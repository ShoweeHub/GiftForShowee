void reboot(const String &msg = "") {
    ApplicationController::showRebootScreen = true;
    Serial.printf("%s%s设备3秒后重启...\n", msg.c_str(), msg.length() > 0 ? "\n" : "");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP.restart();
}