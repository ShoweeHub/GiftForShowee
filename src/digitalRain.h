class DigitalRainApplication : public Application {
private:
    uint8_t rainPos[32][4]{};
    uint8_t reset_count = 0;

public:
    DigitalRainApplication() : Application("digitalRain", "数字雨", {}, true) {
        inAutoQueue = true;
        for (auto &i: rainPos) {
            i[0] = random(16, 24);
            i[1] = random(0, i[0]);
            i[2] = random(1, 5);
            i[3] = 0;
        }
    }

    ScreenController::ScreenFrame getOutsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (uint8_t x = 0; x < 32; x++) {
            for (uint8_t y = 0; y < 8; y++) {
                uint8_t temp_pos = rainPos[x][1];
                frame.data[y][x] = y > temp_pos or temp_pos - y >= 8 ? 0 : (255 - (temp_pos - y) * 32) / 5 * 0x010501;
            }
            rainPos[x][3]++;
            if (rainPos[x][3] >= rainPos[x][2]) {
                rainPos[x][3] = 0;
                rainPos[x][1] = rainPos[x][1] + 1 >= rainPos[x][0] ? 0 : rainPos[x][1] + 1;
            }
        }
        return frame;
    }

    ScreenController::ScreenFrame getInsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        for (uint8_t x = 0; x < 32; x++) {
            for (uint8_t y = 0; y < 8; y++) {
                frame.data[y][x] = screen_reset_logo[y][x];
            }
        }
        uint8_t startX = 22;
        for (uint8_t x = 0; x < reset_count; x++) {
            for (uint8_t y = 0; y < 5; y++) {
                frame.data[y + 2][startX + x * 2] = frame.data[y + 2][startX + x * 2] == 0 ? 0 : 0x00FF00;
            }
        }
        return frame;
    }

    void onLeftButtonPressed() override {
        Serial.printf("%s左键被按下\n", config.alias.c_str());
        if (reset_count < 5 and reset_count > 0) {
            reset_count--;
        }
    }

    void onRightButtonPressed() override {
        Serial.printf("%s右键被按下\n", config.alias.c_str());
        if (reset_count + 1 <= 5) {
            reset_count++;
            if (reset_count == 5) {
                Serial.println("LittleFS 正在格式化...");
                LittleFS.format();
                reboot();
            }
        }
    }

    void enterInsideScreen() override {
        Serial.printf("%s进入内部界面\n", config.alias.c_str());
        reset_count = 0;
    }

    void exitInsideScreen() override {
        Serial.printf("%s退出内部界面\n", config.alias.c_str());
        reset_count = 0;
    }

    void onSelected() override {
        Serial.printf("%s被选中\n", config.alias.c_str());
    }

    void onUnselected() override {
        Serial.printf("%s被取消选中\n", config.alias.c_str());
    }

    void mainTask() override {
    }
};