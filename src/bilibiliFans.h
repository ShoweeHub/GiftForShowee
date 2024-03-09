#include <HTTPClient.h>

Config bilibiliFansConfig = Config("bilibiliFans", "B站粉丝计数器", {
        ConfigItem("uid", "B站UID", "3493110847900630", "请输入B站UID", "^[1-9][0-9]{0,15}$", true, true),
        ConfigItem("delay", "刷新周期(秒)", "5", "范围3~10S", "^[3-9]$|^10$", true, true)
});
long bilibiliFansCount = 0;

[[noreturn]] void bilibiliFans(__attribute__((unused)) void *pVoid) {
    bilibiliFansConfig.loadConfig();
    while (true) {
        if (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin("https://api.bilibili.com/x/relation/stat?vmid=" + bilibiliFansConfig["uid"]);
            int httpCode = http.GET();
            if (httpCode == 200) {
                String payload = http.getString();
                JsonDocument doc;
                deserializeJson(doc, payload);
                bilibiliFansCount = doc["data"]["follower"].as<long>();
            } else {
                Serial.printf("请求失败:%s\n", HTTPClient::errorToString(httpCode).c_str());
            }
            http.end();
            vTaskDelay(bilibiliFansConfig["delay"].toInt() * 1000 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}