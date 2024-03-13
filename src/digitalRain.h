class DigitalRainApplication : public Application {
private:
    uint8_t rainPos[32][4]{};

public:
    DigitalRainApplication() : Application("digitalRain", "数字雨", {}, false) {
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
        return getOutsideScreenFrame();
    }

    void onLeftButtonPressed() override {
        Serial.printf("%s左键被按下\n", config.alias.c_str());
    }

    void onRightButtonPressed() override {
        Serial.printf("%s右键被按下\n", config.alias.c_str());
    }

    void mainTask() override {
    }
};