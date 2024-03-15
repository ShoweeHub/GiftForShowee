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
    ApplicationController::otaUpdating = true;
}

void update_finished() {
    otaConfig["md5"] = md5;
    otaConfig.saveConfig();
    Serial.println("OTA升级已结束");
    ApplicationController::otaUpdating = false;
}

void update_progress(int cur, int total) {
    Serial.printf("OTA升级进度 %d/%d\n", cur, total);
    ApplicationController::otaProgress = cur * 16 / total;
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
    http.GET();
    String location = http.header("Location");
    http.end();
    return location;
}

String get_md5(const String &url) {
    HTTPClient http;
    http.begin(url);
    const char *headerKeys[] = {"Etag"};
    size_t headerKeysSize = sizeof(headerKeys) / sizeof(char *);
    http.collectHeaders(headerKeys, headerKeysSize);
    http.GET();
    String etag = http.header("Etag");
    http.end();
    return etag;
}

[[noreturn]] void checkAndUpdate(__attribute__((unused)) void *pVoid) {
    httpUpdate.onStart(update_started);
    httpUpdate.onEnd(update_finished);
    httpUpdate.onProgress(update_progress);
    httpUpdate.onError(update_error);
    otaConfig.loadConfig();
    while (true) {
        if (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED and xSemaphoreTake(ApplicationController::http_xMutex, (TickType_t) 1000) == pdTRUE) {
            String actual_firmware_url = get_actual_firmware_url();
            if (actual_firmware_url.length() > 0) {
                md5 = get_md5(actual_firmware_url);
                Serial.printf("当前固件的md5:%s,新固件的md5:%s\n", otaConfig["md5"].c_str(), md5.c_str());
                if (md5.length() > 0 and otaConfig["md5"] != md5) {
                    Serial.println("发现新固件,3秒后开始OTA更新");
                    vTaskDelay(3000 / portTICK_PERIOD_MS);
                    WiFiClientSecure wiFiClient;
                    wiFiClient.setInsecure();
                    httpUpdate.update(wiFiClient, actual_firmware_url);
                }
            }
            xSemaphoreGive(ApplicationController::http_xMutex);
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}