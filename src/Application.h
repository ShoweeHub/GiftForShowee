#include <screen_state.h>

class Application {
public:
    Config config;
    bool hasInsideScreen;
    UBaseType_t priority = 2;

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
            } else if (forceDisplayAppsScreen or WiFiClass::getMode() == WIFI_STA and WiFiClass::status() == WL_CONNECTED) {
                ScreenController::showFrame(getScreenFrame(), forceDisplayAppsScreen);
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

    static void registerApp(Application *app) {
        apps.push_back(app);
    }

    static void onLeftButtonPressed() {
        if (apps.empty()) return;
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
        if (inApp) {
            apps[selectedApp]->onRightButtonPressed();
        } else {
            apps[selectedApp]->onUnselected();
            selectedApp = (selectedApp + 1) % apps.size();
            apps[selectedApp]->onSelected();
        }
    }

    static void start() {
        for (auto app: apps) {
            xTaskCreate(RunAppMainTask, app->config.alias.c_str(), 4096, app, app->priority, nullptr);
        }
        xTaskCreate(showScreenFrame, "showScreenFrame", 4096, nullptr, 9, nullptr);
    }
};

bool ApplicationController::inApp = false;
size_t ApplicationController::selectedApp = 0;
std::vector<Application *> ApplicationController::apps;
bool ApplicationController::showRebootScreen = false;
bool ApplicationController::forceDisplayAppsScreen = false;

Application::Application(const String &name, const String &alias, const std::vector<ConfigItem> &items, bool hasInsideScreen, UBaseType_t priority) : config(name, alias, items) {
    this->hasInsideScreen = hasInsideScreen;
    this->priority = priority > 8 ? 8 : priority;
    ApplicationController::registerApp(this);
}