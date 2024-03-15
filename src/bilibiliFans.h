#include <HTTPClient.h>
#include <screen_nums.h>

class BilibiliFansApplication : public Application {
private:
    uint32_t logoColor = 0x40C5F1;
    uint32_t numColor = 0x40C5F1;
    uint32_t backgroundColor = 0x000000;
    long bilibiliFansCount = 0;
    bool selected = false;
    static const bool logo[8][8];
public:
    BilibiliFansApplication() : Application("bilibiliFans", "B站粉丝计数器", {
            ConfigItem("uid", "B站UID", "3493110847900630", "请输入B站UID", "^[1-9][0-9]{0,15}$", true, true),
            ConfigItem("delay", "刷新周期(秒)", "5", "范围3~10S", "^[3-9]$|^10$", true, true)
    }, false) {
        inAutoQueue = true;
    }

    ScreenController::ScreenFrame getOutsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (auto &y: frame.data) {
            for (uint32_t &x: y) {
                x = backgroundColor;
            }
        }
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                frame.data[y][x] = logo[y][x] ? logoColor : backgroundColor;
            }
        }
        String bilibiliFansCountStr = String(bilibiliFansCount);
        uint8_t bilibiliFansCountStrLength = bilibiliFansCountStr.length();
        if (bilibiliFansCountStrLength <= 6) {
            uint8_t startX = 21 - bilibiliFansCountStrLength * 2;
            for (auto i: bilibiliFansCountStr) {
                uint8_t num = i - '0';
                for (uint8_t y = 0; y < 5; y++) {
                    for (uint8_t x = 0; x < 3; x++) {
                        frame.data[y + 2][startX + x] = screen_nums[num][y][x] ? numColor : backgroundColor;
                    }
                }
                startX += 4;
            }
        } else if (bilibiliFansCountStrLength == 7) {
            uint8_t startX = 9;
            for (uint8_t i = 0; i < 4; i++) {
                uint8_t num = bilibiliFansCountStr[i] - '0';
                for (uint8_t y = 0; y < 5; y++) {
                    for (uint8_t x = 0; x < 3; x++) {
                        frame.data[y + 2][startX + x] = screen_nums[num][y][x] ? numColor : backgroundColor;
                    }
                }
                startX += 4;
                if (i == 2) {
                    frame.data[6][startX] = numColor;
                    startX += 2;
                }
            }
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 5; x++) {
                    frame.data[y + 2][startX + x] = screen_w[y][x] ? numColor : backgroundColor;
                }
            }
        } else if (bilibiliFansCountStrLength == 8) {
            uint8_t startX = 10;
            for (uint8_t i = 0; i < 4; i++) {
                uint8_t num = bilibiliFansCountStr[i] - '0';
                for (uint8_t y = 0; y < 5; y++) {
                    for (uint8_t x = 0; x < 3; x++) {
                        frame.data[y + 2][startX + x] = screen_nums[num][y][x] ? numColor : backgroundColor;
                    }
                }
                startX += 4;
            }
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 5; x++) {
                    frame.data[y + 2][startX + x] = screen_w[y][x] ? numColor : backgroundColor;
                }
            }
        }
        return frame;
    }

    ScreenController::ScreenFrame getInsideScreenFrame() override {
        return getOutsideScreenFrame();
    }

    void onLeftButtonPressed() override {
        Serial.printf("%s左键被按下\n", config.alias.c_str());
    }

    void onRightButtonPressed() override {
        Serial.printf("%s右键被按下\n", config.alias.c_str());
    }

    void enterInsideScreen() override {
        Serial.printf("%s进入内部界面\n", config.alias.c_str());
    }

    void exitInsideScreen() override {
        Serial.printf("%s退出内部界面\n", config.alias.c_str());
    }

    void onSelected() override {
        selected = true;
        Serial.printf("%s被选中\n", config.alias.c_str());
    }

    void onUnselected() override {
        selected = false;
        Serial.printf("%s被取消选中\n", config.alias.c_str());
    }

    [[noreturn]] void mainTask() override {
        config.loadConfig();
        while (true) {
            if (selected and WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED and xSemaphoreTake(ApplicationController::http_xMutex, (TickType_t) 1000) == pdTRUE) {
                HTTPClient http;
                http.begin("https://api.bilibili.com/x/relation/stat?vmid=" + config["uid"]);
                Serial.println("正在请求B站API...");
                int httpCode = http.GET();
                if (httpCode == 200) {
                    String payload = http.getString();
                    JsonDocument doc;
                    deserializeJson(doc, payload);
                    bilibiliFansCount = doc["data"]["follower"].as<long>();
                    Serial.printf("请求成功, 粉丝数:%ld\n", bilibiliFansCount);
                } else {
                    Serial.printf("请求失败:%s\n", HTTPClient::errorToString(httpCode).c_str());
                }
                http.end();
                xSemaphoreGive(ApplicationController::http_xMutex);
                vTaskDelay(config["delay"].toInt() * 1000 / portTICK_PERIOD_MS);
            } else {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    }
};

constexpr const bool BilibiliFansApplication::logo[8][8]{
        {false, true,  false, false, false, false, true,  false},
        {false, false, true,  false, false, true,  false, false},
        {false, true,  true,  true,  true,  true,  true,  false},
        {true,  false, false, false, false, false, false, true},
        {true,  false, true,  false, false, true,  false, true},
        {true,  false, true,  false, false, true,  false, true},
        {true,  false, false, false, false, false, false, true},
        {false, true,  true,  true,  true,  true,  true,  false}
};