#include <HTTPClient.h>

typedef struct {
    String uid;
    long fansCount;
} bilibiliFansConfigStruct;

bilibiliFansConfigStruct bilibiliFansConfig = {"3493110847900630", 0};

[[noreturn]] void bilibiliFans(__attribute__((unused)) void *pVoid) {
    while (true) {
        if (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin("https://api.bilibili.com/x/relation/stat?vmid=" + bilibiliFansConfig.uid);
            int httpCode = http.GET();
            if (httpCode == 200) {
                String payload = http.getString();
                JsonDocument doc;
                deserializeJson(doc, payload);
                bilibiliFansConfig.fansCount = doc["data"]["follower"].as<long>();
            } else {
                Serial.printf("请求失败:%s\n", HTTPClient::errorToString(httpCode).c_str());
            }
            http.end();
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
//TODO 添加uid的读取(还没想好是每个模块自己读取还是统一读取)

