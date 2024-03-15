#include <screen_state.h>

class Application {
public:
    Config config;
    bool hasInsideScreen;
    UBaseType_t priority = 2;
    bool inAutoQueue = false;

    Application(const String &name, const String &alias, const std::vector<ConfigItem> &items, bool hasInsideScreen, UBaseType_t priority = 2);

    virtual ScreenController::ScreenFrame getOutsideScreenFrame() = 0;

    virtual ScreenController::ScreenFrame getInsideScreenFrame() = 0;

    virtual void mainTask() = 0;

    virtual void onLeftButtonPressed() = 0;

    virtual void onRightButtonPressed() = 0;

    virtual void enterInsideScreen() = 0;

    virtual void exitInsideScreen() = 0;

    virtual void onSelected() = 0;

    virtual void onUnselected() = 0;
};

class ApplicationController {
private:
    static bool inApp;
    static size_t selectedApp;
    static bool autoQueueLocked;
    static uint8_t lock_pos;

    static void RunAppMainTask(void *app) {
        ((Application *) app)->mainTask();
        vTaskDelete(nullptr);
    }

    static ScreenController::ScreenFrame getScreenFrame() {
        if (apps.empty()) {
            ScreenController::ScreenFrame frame{};
            for (auto &y: frame.data) {
                for (unsigned int &x: y) {
                    x = (uint32_t) random(0, 0xFFFFFF);
                }
            }
            return frame;
        }
        if (inApp) {
            return apps[selectedApp]->getInsideScreenFrame();
        } else {
            return apps[selectedApp]->getOutsideScreenFrame();
        }
    }

    [[noreturn]] static void showScreenFrame(__attribute__((unused)) void *pVoid) {
        uint8_t wifi_connecting_frame_index = 0;
        if (!apps.empty()) {
            apps[selectedApp]->onSelected();
        }
        while (true) {
            if (showRebootScreen) {
                ScreenController::showFrame(screen_reboot_logo);
            } else if (otaUpdating) {
                uint32_t temp_screen_ota_logo[8][32];
                memcpy(temp_screen_ota_logo, screen_ota_logo, sizeof(screen_ota_logo));
                for (int i = 0; i < otaProgress; i++) {
                    temp_screen_ota_logo[4][14 + i] = 0x0000FF;
                }
                ScreenController::showFrame(temp_screen_ota_logo);
            } else if (forceDisplayAppsScreen or WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED) {
                ScreenController::ScreenFrame temp_screen_frame = getScreenFrame();
                if (lock_pos > 0) {
                    temp_screen_frame.data[0][lock_pos] = autoQueueLocked ? 0xFF0000 : 0x00FF00;
                    lock_pos = (lock_pos + 1) % 32;
                }
                ScreenController::showFrame(temp_screen_frame, forceDisplayAppsScreen);
                if (!forceDisplayAppsScreen) {
                    frame_index++;
                    if (frame_index >= 50 * 30) {
                        frame_index = 0;
                        if (!apps.empty() and !inApp and !autoQueueLocked) {
                            onRightButtonPressed();
                            if (!apps[selectedApp]->inAutoQueue) {
                                frame_index = 50 * 30;
                            }
                        }
                    }
                }
            } else if (WiFiClass::getMode() == WIFI_AP) {
                ScreenController::showFrame(screen_ap_mode);
            } else if (WiFiClass::getMode() == WIFI_STA and WiFiClass::status() != WL_CONNECTED) {
                uint32_t temp_screen_wifi_logo[8][32];
                memcpy(temp_screen_wifi_logo, screen_wifi_logo, sizeof(screen_wifi_logo));
                for (int i = 0; i < wifi_connecting_frame_index / 25; i++) {
                    temp_screen_wifi_logo[6][17 + i * 3] = 0x4DA6FF;
                }
                wifi_connecting_frame_index = (wifi_connecting_frame_index + 1) % 150;
                ScreenController::showFrame(temp_screen_wifi_logo);
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }

public:
    static std::vector<Application *> apps;
    static bool showRebootScreen;
    static bool forceDisplayAppsScreen;
    static bool otaUpdating;
    static uint8_t otaProgress;
    static uint16_t frame_index;
    static SemaphoreHandle_t http_xMutex;

    static void registerApp(Application *app) {
        apps.push_back(app);
    }

    static void onLeftButtonPressed() {
        if (apps.empty()) return;
        frame_index = 0;
        if (inApp) {
            apps[selectedApp]->onLeftButtonPressed();
        } else {
            apps[selectedApp]->onUnselected();
            selectedApp = (selectedApp - 1 + apps.size()) % apps.size();
            apps[selectedApp]->onSelected();
        }
    }

    static void onCenterButtonPressed() {
        if (apps.empty()) return;
        frame_index = 0;
        if (apps[selectedApp]->hasInsideScreen) {
            inApp = !inApp;
            if (inApp) {
                apps[selectedApp]->enterInsideScreen();
            } else {
                apps[selectedApp]->exitInsideScreen();
            }
        } else {
            inApp = false;
        }
    }

    static void onRightButtonPressed() {
        if (apps.empty()) return;
        frame_index = 0;
        if (inApp) {
            apps[selectedApp]->onRightButtonPressed();
        } else {
            apps[selectedApp]->onUnselected();
            selectedApp = (selectedApp + 1) % apps.size();
            apps[selectedApp]->onSelected();
        }
    }

    static void switchAutoQueueLock() {
        autoQueueLocked = !autoQueueLocked;
        frame_index = 0;
        lock_pos = 1;
    }

    static void start() {
        for (auto app: apps) {
            xTaskCreate(RunAppMainTask, app->config.alias.c_str(), 16384, app, app->priority, nullptr);
        }
        xTaskCreate(showScreenFrame, "showScreenFrame", 16384, nullptr, 9, nullptr);
    }
};

bool ApplicationController::inApp = false;
size_t ApplicationController::selectedApp = 0;
std::vector<Application *> ApplicationController::apps;
bool ApplicationController::showRebootScreen = false;
bool ApplicationController::forceDisplayAppsScreen = false;
bool ApplicationController::otaUpdating = false;
uint8_t ApplicationController::otaProgress = 0;
uint16_t ApplicationController::frame_index = 0;
bool ApplicationController::autoQueueLocked = false;
uint8_t ApplicationController::lock_pos = 0;
SemaphoreHandle_t ApplicationController::http_xMutex = xSemaphoreCreateMutex();

Application::Application(const String &name, const String &alias, const std::vector<ConfigItem> &items, bool hasInsideScreen, UBaseType_t priority) : config(name, alias, items) {
    this->hasInsideScreen = hasInsideScreen;
    this->priority = priority > 8 ? 8 : priority;
    ApplicationController::registerApp(this);
}