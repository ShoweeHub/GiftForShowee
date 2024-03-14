#include <FastLED.h>

class ScreenController {
private:
    static CRGB leds[256];
    static bool screenOpened;
    static uint8_t brightness;
public:
    typedef struct {
        uint32_t data[8][32];
    } ScreenFrame;

    static void setup() {
        LEDS.addLeds<WS2812B, 21, GRB>(leds, 256);
        setBrightness(8);
    }

    static void update() {
        LEDS.setBrightness(brightness * screenOpened);
        LEDS.show();
    }

    static void setBrightness(uint8_t new_brightness) {
        if (new_brightness >= 2 and new_brightness <= 32 and new_brightness != brightness) {
            brightness = new_brightness;
            update();
        }
    }

    static uint8_t getBrightness() {
        return LEDS.getBrightness();
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
        screenOpened = !screenOpened;
        update();
    }

    static void showFrame(const ScreenFrame &frame, bool forceDisplayAppsScreen = false) {
        showFrame(frame.data, forceDisplayAppsScreen);
    }

    static void showFrame(const uint32_t (&frame)[8][32], bool forceDisplayAppsScreen = false) {
        LEDS.clearData();
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 32; x++) {
                setPixel(x, y, frame[y][x]);
            }
        }
        if (forceDisplayAppsScreen) {
            setPixel(0, 0, 0xFF0000);
            setPixel(1, 0, 0x00FF00);
            setPixel(2, 0, 0x0000FF);
            setPixel(3, 0, 0xFFFF00);
            setPixel(4, 0, 0xFF00FF);
            setPixel(5, 0, 0x00FFFF);
            setPixel(6, 0, 0xFFFFFF);
        }
        update();
    }

    static bool isScreenOpened() {
        return screenOpened;
    }

    static void setPixel(uint8_t x, uint8_t y, CRGB color, bool updateScreen = false) {
        if (x > 31 or y > 7) {
            return;
        }
        color.g = uint8_t(color.g * 0.8);
        color.b = uint8_t(color.b * 0.5);
        leds[y * 32 + (y % 2 == 0 ? x : 31 - x)] = color;
        if (updateScreen) {
            update();
        }
    }
};

CRGB ScreenController::leds[256];
bool ScreenController::screenOpened = true;
uint8_t ScreenController::brightness = 8;