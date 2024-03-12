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
        if (new_brightness > 0 and new_brightness != brightness) {
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

    static void showFrame(const ScreenFrame &frame) {
        showFrame(frame.data);
    }

    static void showFrame(const uint32_t (&frame)[8][32]) {
        LEDS.clearData();
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 32; x++) {
                setPixel(x, y, frame[y][x]);
            }
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