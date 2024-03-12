#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <configLib.h>
#include <ScreenController.h>
#include <Application.h>
#include <bilibiliFans.h>

#define LEFT_BUTTON 26
#define CENTER_BUTTON 16
#define RIGHT_BUTTON 5
Config baseConfig = Config("base", "基础", {
        ConfigItem("host_name", "本设备名称", "Showee-PandoraBox", "1~32个字符(字母或数字或_-)", "^[a-zA-Z0-9_\\-]{1,32}$", true, true),
        ConfigItem("wifi_ssid", "WiFi名称", "", "1~32个字符", "^.{1,32}$", true, false),
        ConfigItem("wifi_password", "WiFi密码", "", "空或8~64个英文字符", "^$|^[ -~]{8,64}$", false, false),
        ConfigItem("ap_ssid", "AP名称", "守一Showee的潘多拉魔盒", "1~32个字符", "^.{1,32}$", true, true),
        ConfigItem("ap_password", "AP密码", "LoveShoweeForever", "空或8~64个英文字符", "^$|^[ -~]{8,64}$", false, false)
});
BilibiliFansApplication bilibiliFansApplication;
WebServer server(80);
DNSServer dnsServer;
bool dnsServerStarted = false;
bool webServerStarted = false;

void reboot(const String &msg = "") {
    ApplicationController::showRebootScreen = true;
    Serial.printf("%s%s设备3秒后重启...\n", msg.c_str(), msg.length() > 0 ? "\n" : "");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP.restart();
}

bool beginLittleFS() {
    Serial.println("正在初始化 LittleFS...");
    if (!LittleFS.begin()) {
        Serial.println("LittleFS 初始化失败, 正在尝试格式化...");
        if (LittleFS.format()) {
            Serial.println("LittleFS 格式化成功, 正在重新初始化...");
            if (!LittleFS.begin()) {
                Serial.println("LittleFS 初始化失败, 此错误无法修复");
                return false;
            }
        } else {
            Serial.println("LittleFS 格式化失败, 此错误无法修复");
            return false;
        }
    }
    Serial.println("LittleFS 初始化成功");
    return true;
}

void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("STA模式已启动");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("STA模式已停止");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.printf("已连接到: %s\n", WiFi.SSID().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("未连接到WIFI");
            WiFi.reconnect();
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("已获取IP: %s\n", WiFi.localIP().toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.printf("AP模式已启动, SSID: %s, 密码: %s\n", baseConfig["ap_ssid"].c_str(), baseConfig["ap_password"].c_str());
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("AP模式已停止");
            break;
        default:
            Serial.println("WIFI事件: " + String(event));
            break;
    }
}

bool startSTA() {
    if (baseConfig["wifi_ssid"].length() == 0) {
        return false;
    }
    WiFiClass::hostname(baseConfig["host_name"]);
    WiFiClass::mode(WIFI_STA);
    WiFi.begin(baseConfig["wifi_ssid"], baseConfig["wifi_password"]);
    return true;
}

void startDNSServer() {
    if (dnsServer.start(53, "*", WiFi.softAPIP())) {
        dnsServerStarted = true;
        Serial.println("DNSServer启动成功");
    } else {
        reboot("DNSServer启动失败");
    }
}

void startAP() {
    WiFiClass::mode(WIFI_AP);
    if (!WiFi.softAP(baseConfig["ap_ssid"], baseConfig["ap_password"])) {
        if (!WiFi.softAP("守一Showee的粉丝灯牌", "LoveShoweeForever")) {
            reboot("AP模式启动失败, 此错误无法修复");
        }
    }
    startDNSServer();
}

void handleNotFound() {
    Serial.println("未找到: " + server.uri());
    server.send(200, "application/json", R"({"code":-1,"msg":"Not Found"})");
}

void ServerHandleConfig(Config &config) {
    server.on("/" + config.name + "Config", HTTP_GET, [&config]() {
        server.send(200, "text/html", config.getConfigPageHtml());
    });
    server.on("/" + config.name + "Config", HTTP_POST, [&config]() {
        config.loadConfigFromServerArgs(server);
        config.saveConfig();
        server.send(200, "application/json", R"({"code":0,"msg":"保存成功"})");
        reboot(config.name + "配置已保存");
    });
}

void startWebServer() {
    ServerHandleConfig(baseConfig);
    for (auto app: ApplicationController::apps) {
        ServerHandleConfig(app->config);
    }
    server.onNotFound(handleNotFound);
    server.begin();
    webServerStarted = true;
    Serial.println("WebServer已启动");
}

[[noreturn]] void listenButtonsPressed(__attribute__((unused)) void *pVoid) {
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(CENTER_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    uint8_t leftButtonPressedCount = 0;
    uint8_t centerButtonPressedCount = 0;
    uint8_t rightButtonPressedCount = 0;
    const uint8_t longPressCount = 10;
    bool canPressAppButton;
    while (true) {
        canPressAppButton = (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED and ScreenController::isScreenOpened());
        if (digitalRead(LEFT_BUTTON) == HIGH) {
            if (leftButtonPressedCount >= longPressCount) {
                if (leftButtonPressedCount == longPressCount) {
                    Serial.println("左键被长按");
                    if (rightButtonPressedCount > longPressCount) {
                        Serial.println("左右键同时长按");
                        if (ScreenController::isScreenOpened()) {
                            if (WiFiClass::getMode() != WIFI_AP) {
                                startAP();
                            } else {
                                reboot("主动重启");
                            }
                        }
                    }
                }
                leftButtonPressedCount = longPressCount + 1;
            } else {
                leftButtonPressedCount++;
            }
        } else {
            if (leftButtonPressedCount >= longPressCount) {
                Serial.println("左键被长按松开");
            } else if (leftButtonPressedCount > 0) {
                Serial.println("左键被短按松开");
                if (canPressAppButton) {
                    ApplicationController::onLeftButtonPressed();
                }
            }
            leftButtonPressedCount = 0;
        }
        if (digitalRead(CENTER_BUTTON) == LOW) {
            if (centerButtonPressedCount >= longPressCount) {
                if (centerButtonPressedCount == longPressCount) {
                    Serial.println("中键被长按");
                    ScreenController::switchScreen();
                }
                centerButtonPressedCount = longPressCount + 1;
            } else {
                centerButtonPressedCount++;
            }
        } else {
            if (centerButtonPressedCount >= longPressCount) {
                Serial.println("中键被长按松开");
            } else if (centerButtonPressedCount > 0) {
                Serial.println("中键被短按松开");
                if (canPressAppButton) {
                    ApplicationController::onCenterButtonPressed();
                }
            }
            centerButtonPressedCount = 0;
        }
        if (digitalRead(RIGHT_BUTTON) == HIGH) {
            if (rightButtonPressedCount >= longPressCount) {
                if (rightButtonPressedCount == longPressCount) {
                    Serial.println("右键被长按");
                    if (leftButtonPressedCount > longPressCount) {
                        Serial.println("左右键同时长按");
                        if (ScreenController::isScreenOpened()) {
                            if (WiFiClass::getMode() != WIFI_AP) {
                                startAP();
                            } else {
                                reboot("主动重启");
                            }
                        }
                    }
                }
                rightButtonPressedCount = longPressCount + 1;
            } else {
                rightButtonPressedCount++;
            }
        } else {
            if (rightButtonPressedCount >= longPressCount) {
                Serial.println("右键被长按松开");
            } else if (rightButtonPressedCount > 0) {
                Serial.println("右键被短按松开");
                if (canPressAppButton) {
                    ApplicationController::onRightButtonPressed();
                }
            }
            rightButtonPressedCount = 0;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n========设备重启========");
    ScreenController::setup();
    if (!beginLittleFS()) {
        reboot("LittleFS 故障");
    }
    ApplicationController::start();
    xTaskCreate(listenButtonsPressed, "listenButtonsPressed", 4096, nullptr, 1, nullptr);
    WiFi.onEvent(onWiFiEvent);
    if (!baseConfig.loadConfig() || !startSTA()) {
        startAP();
    }
    startWebServer();
}

void loop() {
    if (webServerStarted) {
        server.handleClient();
    }
    if (dnsServerStarted) {
        dnsServer.processNextRequest();
    }
}