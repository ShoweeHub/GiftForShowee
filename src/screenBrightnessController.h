class ScreenBrightnessControllerApplication : public Application {
private:
    static const bool brightness_logo[7][7];
public:
    ScreenBrightnessControllerApplication() : Application("screenBrightnessController", "屏幕亮度控制器", {
            ConfigItem("brightness", "亮度", "8", "2~32", "^[2-9]|[1-2][0-9]|3[0-2]$", true, true)
    }, true) {}

    ScreenController::ScreenFrame getOutsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (auto &y: frame.data) {
            for (uint32_t &x: y) {
                x = 0x000000;
            }
        }
        for (uint8_t y = 0; y < 7; y++) {
            for (uint8_t x = 0; x < 7; x++) {
                frame.data[y + 1][x + 1] = brightness_logo[y][x] ? 0xFFFF00 : 0x000000;
            }
        }
        String brightnessStr = String(ScreenController::getBrightness());
        uint8_t brightnessStrLength = brightnessStr.length();
        uint8_t startX = 21 - brightnessStrLength * 2;
        for (uint8_t i = 0; i < brightnessStrLength; i++) {
            uint8_t num = brightnessStr[i] - '0';
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 3; x++) {
                    frame.data[y + 2][startX + i * 4 + x] = screen_nums[num][y][x] ? 0xFFFFFF : 0x000000;
                }
            }
        }
        return frame;
    }

    ScreenController::ScreenFrame getInsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (auto &y: frame.data) {
            for (uint32_t &x: y) {
                x = 0x000000;
            }
        }
        for (uint8_t y = 0; y < 7; y++) {
            for (uint8_t x = 0; x < 7; x++) {
                frame.data[y + 1][x + 1] = brightness_logo[y][x] ? 0xFFFF00 : 0x000000;
            }
        }
        String brightnessStr = String(ScreenController::getBrightness());
        uint8_t brightnessStrLength = brightnessStr.length();
        uint8_t startX = 21 - brightnessStrLength * 2;
        for (uint8_t i = 0; i < brightnessStrLength; i++) {
            uint8_t num = brightnessStr[i] - '0';
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 3; x++) {
                    frame.data[y + 2][startX + i * 4 + x] = screen_nums[num][y][x] ? 0xFFFFFF : 0x000000;
                }
            }
        }
        for (uint8_t i = 0; i < 3; i++) {
            frame.data[4 + i][10 + i] = 0x00FF00;
            frame.data[4 - i][10 + i] = 0x00FF00;
            frame.data[4 + i][30 - i] = 0x00FF00;
            frame.data[4 - i][30 - i] = 0x00FF00;
        }
        return frame;
    }

    void onLeftButtonPressed() override {
        Serial.printf("%s左键被按下\n", config.alias.c_str());
        ScreenController::setBrightness(ScreenController::getBrightness() - 1);
        config["brightness"] = String(ScreenController::getBrightness());
        config.saveConfig();
    }

    void onRightButtonPressed() override {
        Serial.printf("%s右键被按下\n", config.alias.c_str());
        ScreenController::setBrightness(ScreenController::getBrightness() + 1);
        config["brightness"] = String(ScreenController::getBrightness());
        config.saveConfig();
    }

    void mainTask() override {
        config.loadConfig();
        ScreenController::setBrightness(config["brightness"].toInt());
    }
};

const bool ScreenBrightnessControllerApplication::brightness_logo[7][7] = {
        {false, false, false, true,  false, false, false},
        {false, true,  false, false, false, true,  false},
        {false, false, false, true,  false, false, false},
        {true,  false, true,  true,  true,  false, true},
        {false, false, false, true,  false, false, false},
        {false, true,  false, false, false, true,  false},
        {false, false, false, true,  false, false, false}
};