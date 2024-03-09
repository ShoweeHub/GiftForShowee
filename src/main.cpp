#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <configLib.h>
#include <bilibiliFans.h>

Config baseConfig = Config("base", "基础", {
        ConfigItem("host_name", "本设备名称", "Showee-PandoraBox", "1~32个字符(字母或数字或_-)", "^[a-zA-Z0-9_\\-]{1,32}$", true, true),
        ConfigItem("wifi_ssid", "WiFi名称", "", "1~32个字符", "^.{1,32}$", true, false),
        ConfigItem("wifi_password", "WiFi密码", "", "空或8~64个英文字符", "^$|^[ -~]{8,64}$", false, false),
        ConfigItem("ap_ssid", "AP名称", "守一Showee的潘多拉魔盒", "1~32个字符", "^.{1,32}$", true, true),
        ConfigItem("ap_password", "AP密码", "LoveShoweeForever", "空或8~64个英文字符", "^$|^[ -~]{8,64}$", false, false)
});

WebServer server(80);
DNSServer dnsServer;

bool dnsServerStarted = false;
bool webServerStarted = false;

void reboot(const String &msg = "") {
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
    ServerHandleConfig(bilibiliFansConfig);
    server.onNotFound(handleNotFound);
    server.begin();
    webServerStarted = true;
    Serial.println("WebServer已启动");
}

void listenStartAPButtonPressed(__attribute__((unused)) void *pVoid) {
    while (true) {
        if (digitalRead(0) == LOW) {
            Serial.println("按键被按下, 正在启动AP模式...");
            startAP();
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(nullptr);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n========设备重启========");
    if (!beginLittleFS()) {
        reboot("LittleFS 故障");
    }
    WiFi.onEvent(onWiFiEvent);
    if (!baseConfig.loadConfig() || !startSTA()) {
        startAP();
    } else {
        xTaskCreate(listenStartAPButtonPressed, "listenStartAPButtonPressed", 4096, nullptr, 1, nullptr);
        xTaskCreate(bilibiliFans, "bilibiliFans", 4096, nullptr, 1, nullptr);
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