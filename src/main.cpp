#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

String wifi_ssid = "";
String wifi_password = "";
String ap_ssid = "守一Showee的粉丝灯牌";
String ap_password = "LoveShoweeForever";
const unsigned long attemptTimeoutMillis = 10000;

void reboot(const String &msg = "") {
    Serial.printf("%s%s设备3秒后重启...", msg.c_str(), msg.length() > 0 ? "\n" : "");
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

bool saveBaseConfig() {
    Serial.println("正在保存基础配置...");
    JsonDocument doc;
    doc["wifi_ssid"] = wifi_ssid;
    doc["wifi_password"] = wifi_password;
    doc["ap_ssid"] = ap_ssid;
    doc["ap_password"] = ap_password;
    File configFile = LittleFS.open("/baseConfig.json", "w");
    if (!configFile) {
        Serial.println("打开配置文件失败,此错误无法修复");
        return false;
    }
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("基础配置保存成功");
    return true;
}

String JsonStringOr(const JsonVariant &v, const String &def, bool notBlank = false) {
    return v.isNull() || (notBlank && v.as<String>().length() == 0) ? def : v.as<String>();
}

bool loadBaseConfig() {
    Serial.println("正在加载基础配置...");
    File configFile = LittleFS.open("/baseConfig.json", "r");
    if (!configFile) {
        Serial.println("未找到基础配置文件");
        saveBaseConfig();
        return false;
    }
    String config = configFile.readString();
    configFile.close();
    Serial.println("基础配置文件内容: " + config);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config);
    if (error) {
        Serial.println("解析配置文件失败");
        return false;
    }
    wifi_ssid = JsonStringOr(doc["wifi_ssid"], wifi_ssid);
    wifi_password = JsonStringOr(doc["wifi_password"], wifi_password);
    ap_ssid = JsonStringOr(doc["ap_ssid"], ap_ssid, true);
    ap_password = JsonStringOr(doc["ap_password"], ap_password);
    Serial.println("基础配置加载成功");
    Serial.printf("wifi_ssid: %s\n", wifi_ssid.c_str());
    Serial.printf("wifi_password: %s\n", wifi_password.c_str());
    Serial.printf("ap_ssid: %s\n", ap_ssid.c_str());
    Serial.printf("ap_password: %s\n", ap_password.c_str());
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
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("已获取IP: %s\n", WiFi.localIP().toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.printf("AP模式已启动, SSID: %s, 密码: %s\n", ap_ssid.c_str(), ap_password.c_str());
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
    if (wifi_ssid.length() == 0) {
        return false;
    }
    WiFiClass::mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    return true;
}

void startAP() {
    WiFiClass::mode(WIFI_AP);
    if (!WiFi.softAP(ap_ssid.c_str(), ap_password.c_str())) {
        if (!WiFi.softAP("守一Showee的粉丝灯牌", "LoveShoweeForever")) {
            reboot("AP模式启动失败, 此错误无法修复");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n========设备重启========");
    if (!beginLittleFS()) {
        reboot("LittleFS 故障");
    }
    WiFi.onEvent(onWiFiEvent);
    if (!loadBaseConfig() || !startSTA()) {
        startAP();
    }
}

void loop() {
}