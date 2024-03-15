class ScreenWeatherApplication : public Application {
private:
    int8_t temperature = 0;
    uint8_t humidity = 0;
    bool selected = false;
    const String baseUrl = "https://weatherapi.market.xiaomi.com/wtr-v3/weather/current?appKey=weather20151024&sign=zUFJoAR2ZVrDy1vF3D07&isGlobal=False&locale=zh_cn";
public:
    ScreenWeatherApplication() : Application("screenWeather", "天气APP", {
            ConfigItem("location", "城市ID", "101280604", "请输入城市ID", "^\\d{9}$", true, true),
            ConfigItem("delay", "刷新周期(分钟)", "1", "范围1~5分钟", "^[1-5]$", true, true)
    }, false) {
        inAutoQueue = true;
    }

    ScreenController::ScreenFrame getOutsideScreenFrame() override {
        ScreenController::ScreenFrame frame{};
        memcpy(frame.data, screen_weather_logo, sizeof(screen_weather_logo));
        String humidityStr = String(humidity / 10) + String(humidity % 10);
        String temperatureStr = String(abs8(temperature) / 10) + String(abs8(temperature) % 10);
        uint8_t startX = 0;
        for (uint8_t i = 0; i < 2; i++) {
            uint8_t num = humidityStr[i] - '0';
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 3; x++) {
                    frame.data[y][x + startX] = screen_nums[num][y][x] ? 0x4DA6FF : 0x000000;
                }
            }
            startX += 4;
        }
        if (temperature < 0) {
            frame.data[5][14] = 0xFF0000;
            frame.data[5][15] = 0xFF0000;
        }
        startX = 17;
        for (uint8_t i = 0; i < 2; i++) {
            uint8_t num = temperatureStr[i] - '0';
            for (uint8_t y = 0; y < 5; y++) {
                for (uint8_t x = 0; x < 3; x++) {
                    frame.data[y + 3][x + startX] = screen_nums[num][y][x] ? 0xFF0000 : 0x000000;
                }
            }
            startX += 4;
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
                http.begin(baseUrl + "&locationKey=weathercn:" + config["location"]);
                Serial.println("正在请求天气API...");
                int httpCode = http.GET();
                if (httpCode == 200) {
                    String payload = http.getString();
                    JsonDocument doc;
                    deserializeJson(doc, payload);
                    temperature = doc["temperature"]["value"].as<int8_t>();
                    humidity = doc["humidity"]["value"].as<uint8_t>();
                    Serial.printf("请求成功, 温度:%d, 湿度:%d\n", temperature, humidity);
                } else {
                    Serial.printf("请求失败:%s\n", HTTPClient::errorToString(httpCode).c_str());
                }
                http.end();
                xSemaphoreGive(ApplicationController::http_xMutex);
                vTaskDelay(config["delay"].toInt() * 60000 / portTICK_PERIOD_MS);
            } else {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    }
};
