#include <HTTPUpdate.h>

const String firmware_url = "https://g-lirb8132-generic.pkg.coding.net/gift_for_showee/esp32/firmware.bin";
Config otaConfig = Config("ota", "OTA", {
        ConfigItem("md5", "当前固件的md5", "", "", "", false, false)
});
String md5 = "";

void reboot(const String &msg = "") {
    ApplicationController::showRebootScreen = true;
    Serial.printf("%s%s设备3秒后重启...\n", msg.c_str(), msg.length() > 0 ? "\n" : "");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP.restart();
}

void update_started() {
    Serial.println("开始OTA更新");
    //TODO: OTA界面
}

void update_finished() {
    otaConfig["md5"] = md5;
    otaConfig.saveConfig();
    Serial.println("OTA升级已结束");
}

void update_progress(int cur, int total) {
    Serial.printf("OTA升级进度 %d/%d\n", cur, total);
    //TODO: 百分比进度
}

void update_error(int err) {
    Serial.printf("OTA升级失败,错误代码:%d", err);
    reboot();
}

String get_actual_firmware_url() {
    HTTPClient http;
    http.begin(firmware_url);
    const char *headerKeys[] = {"Location"};
    size_t headerKeysSize = sizeof(headerKeys) / sizeof(char *);
    http.collectHeaders(headerKeys, headerKeysSize);
    if (http.GET() == 307) {
        return http.header("Location");
    }
    return "";
}

String get_md5(const String &url) {
    HTTPClient http;
    http.begin(url);
    const char *headerKeys[] = {"Etag"};
    size_t headerKeysSize = sizeof(headerKeys) / sizeof(char *);
    http.collectHeaders(headerKeys, headerKeysSize);
    if (http.GET() == 200) {
        return http.header("Etag");
    }
    return "";
}

[[noreturn]] void checkAndUpdate(__attribute__((unused)) void *pVoid) {
    httpUpdate.onStart(update_started);
    httpUpdate.onEnd(update_finished);
    httpUpdate.onProgress(update_progress);
    httpUpdate.onError(update_error);
    otaConfig.loadConfig();
    while (true) {
        if (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED) {
            String actual_firmware_url = get_actual_firmware_url();
            if (actual_firmware_url.length() > 0) {
                md5 = get_md5(actual_firmware_url);
                Serial.printf("当前固件的md5:%s,新固件的md5:%s\n", otaConfig["md5"].c_str(), md5.c_str());
                if (md5.length() > 0 and otaConfig["md5"] != md5) {
                    WiFiClientSecure wiFiClient;
                    wiFiClient.setInsecure();
                    httpUpdate.update(wiFiClient, actual_firmware_url);
                }
            }
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}