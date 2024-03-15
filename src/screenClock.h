#include <HTTPClient.h>
#include <ctime>

class ScreenClockApplication : public Application {
private:
    uint32_t logoColor = 0xCFFF70;
    uint32_t numColor = 0xFFB570;
    uint32_t backgroundColor = 0x000000;
    uint32_t weekColor[7] = {
            CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Indigo, CRGB::Violet
    };

public:
    ScreenClockApplication() : Application("screenClock", "屏幕时钟", {
            ConfigItem("time_zone", "时区", "+8", "请输入UTC时区,范围:-12~+12", "^(\\+?|-?)(1[0-2]|[0-9])$", true, true)
    }, true) {
        inAutoQueue = true;
    }

    ScreenController::ScreenFrame getOutsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (auto &y: frame.data) {
            for (uint32_t &x: y) {
                x = backgroundColor;
            }
        }
        time_t now = time(nullptr);
        struct tm *localTime = localtime(&now);
        char nowTime[9];
        strftime(nowTime, 9, "%H:%M:%S", localTime);
        uint8_t startX = 4;
        for (uint8_t i = 0; i < 8; i++) {
            if (nowTime[i] == ':') {
                frame.data[3][startX] = numColor;
                frame.data[5][startX] = numColor;
                startX += 2;
            } else {
                uint8_t num = nowTime[i] - '0';
                for (uint8_t y = 0; y < 5; y++) {
                    for (uint8_t x = 0; x < 3; x++) {
                        frame.data[y + 2][startX + x] = screen_nums[num][y][x] ? numColor : backgroundColor;
                    }
                }
                startX += 4;
            }
        }
        if (localTime->tm_hour >= 12) {
            frame.data[2][0] = logoColor;
            frame.data[5][0] = logoColor;
            frame.data[3][1] = logoColor;
            frame.data[6][1] = logoColor;
            frame.data[2][2] = logoColor;
            frame.data[5][2] = logoColor;
        } else {
            frame.data[3][0] = logoColor;
            frame.data[6][0] = logoColor;
            frame.data[2][1] = logoColor;
            frame.data[5][1] = logoColor;
            frame.data[3][2] = logoColor;
            frame.data[6][2] = logoColor;
        }
        return frame;
    }

    ScreenController::ScreenFrame getInsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (auto &y: frame.data) {
            for (uint32_t &x: y) {
                x = backgroundColor;
            }
        }
        time_t now = time(nullptr);
        struct tm *localTime = localtime(&now);
        char nowTime[6];
        strftime(nowTime, 6, "%m.%d", localTime);
        uint8_t startX = 2;
        for (uint8_t i = 0; i < 5; i++) {
            if (nowTime[i] == '.') {
                frame.data[6][startX] = numColor;
                startX += 2;
            } else {
                uint8_t num = nowTime[i] - '0';
                for (uint8_t y = 0; y < 5; y++) {
                    for (uint8_t x = 0; x < 3; x++) {
                        frame.data[y + 2][startX + x] = screen_nums[num][y][x] ? numColor : backgroundColor;
                    }
                }
                startX += 4;
            }
        }
        for (uint8_t x = 21; x < 25; x++) {
            frame.data[4][x] = weekColor[localTime->tm_wday];
        }
        uint8_t week = localTime->tm_wday ? localTime->tm_wday : 7;
        for (uint8_t y = 0; y < 5; y++) {
            for (uint8_t x = 0; x < 3; x++) {
                frame.data[y + 2][x + 27] = screen_nums[week][y][x] ? logoColor : backgroundColor;
            }
        }
        return frame;
    }

    void onLeftButtonPressed() override {
        Serial.printf("%s左键被按下\n", config.alias.c_str());
        ApplicationController::onCenterButtonPressed();
        ApplicationController::onLeftButtonPressed();
    }

    void onRightButtonPressed() override {
        Serial.printf("%s右键被按下\n", config.alias.c_str());
        ApplicationController::onCenterButtonPressed();
        ApplicationController::onRightButtonPressed();
    }

    void enterInsideScreen() override {
        Serial.printf("%s进入内部界面\n", config.alias.c_str());
    }

    void exitInsideScreen() override {
        Serial.printf("%s退出内部界面\n", config.alias.c_str());
    }

    void onSelected() override {
        Serial.printf("%s被选中\n", config.alias.c_str());
    }

    void onUnselected() override {
        Serial.printf("%s被取消选中\n", config.alias.c_str());
    }

    void mainTask() override {
        config.loadConfig();
        configTime(config["time_zone"].toInt() * 3600, 0, "cn.pool.ntp.org", "time.windows.com");
    }
};