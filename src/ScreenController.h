#include <FastLED.h>

class ScreenController {
private:
    static CRGB leds[256];
    static bool screenOpened;
    static Config screenConfig;
public:
    static void setup() {
        LEDS.addLeds<WS2812B, 21, GRB>(leds, 256);
        setBrightness(1);
    }

    static void update() {
        LEDS.setBrightness(screenConfig["brightness"].toInt() * 8 * screenOpened);
        LEDS.show();
    }

    static void loadConfig() {
        screenConfig.loadConfig();
        update();
    }

    static void setBrightness(uint8_t brightness) {
        if (brightness >= 1 and brightness <= 4 and screenConfig["brightness"].toInt() != brightness) {
            screenConfig["brightness"] = String(brightness);
            screenConfig.saveConfig();
        }
        update();
    }

    static uint8_t getBrightness() {
        return screenConfig["brightness"].toInt();
    }

    static void closeScreen() {
        screenOpened = false;
        update();
    }

    static void openScreen() {
        screenOpened = true;
        update();
    }

    static void switchScreen() {
        if (screenOpened) {
            closeScreen();
        } else {
            openScreen();
        }
    }

    static void showFrame(uint32_t frame[8][32]) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 32; x++) {
                setPixel(x, y, frame[y][x]);
            }
        }
        update();
    }

    static void setPixel(int x, int y, uint32_t color, bool updateScreen = false) {
        leds[y * 32 + (y % 2 == 0 ? x : 31 - x)] = color;
        if (updateScreen) {
            update();
        }
    }
};

CRGB ScreenController::leds[256];
bool ScreenController::screenOpened = true;
Config ScreenController::screenConfig = Config("screen", "屏幕", {
        ConfigItem("brightness", "亮度", "1", "范围1~4级", "^[1-4]$", true, true)
});