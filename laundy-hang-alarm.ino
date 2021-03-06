#include <driver/rtc_io.h>
#include <heltec.h>

#include "Config.hpp"
#include "Fonts.hpp"
#include "Logos.hpp"

#include <AdafruitIO_WiFi.h>
#include <Bounce2.h>
#include <dhtnew.h>
#include <WiFiManager.h>
#include <Ticker.h>

gpio_num_t constexpr BUTTON_PIN = GPIO_NUM_0;
Button btn;

DHTNEW dht(GPIO_NUM_5);
Ticker dhtTicker, ioTicker;

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, nullptr, nullptr);
AdafruitIO_Feed *humidity = nullptr;
AdafruitIO_Feed *temperature = nullptr;

float readHumidity;
float readTemperature;

WiFiManager wm;

void welcome() {
  Heltec.display->clear();
  Heltec.display->setFont(Roboto_16);
  Heltec.display->drawString(64, 32, "Welcome");
  Heltec.display->display();
  delay(500);
}

void onWifiConnected() {
  dhtTicker.attach(5, updateHardware);
  ioTicker.attach(15, sendAdafruit);
}

void networkSetup() {
  WiFi.mode(WIFI_STA);
  wm.setClass("invert");
  wm.setConnectRetries(5);
  wm.setDebugOutput(false);
  wm.setConfigPortalBlocking(false);
  wm.setSaveConfigCallback(onWifiConnected);

  if (wm.autoConnect(AP_SSID, AP_PASS)) {
    onWifiConnected();
  } else {
    Heltec.display->clear();
    Heltec.display->setFont(Roboto_16);
    Heltec.display->drawString(64, 30, "SSID: " + wm.getConfigPortalSSID());
    Heltec.display->setFont(Roboto_Italic_12);
    Heltec.display->drawString(64, 12, "AP mode");
    Heltec.display->drawString(64, 52, WiFi.softAPIP().toString());
    Heltec.display->display();
  }
}

void updateHardware() {
  if (dht.read() == DHTLIB_OK) {
    readHumidity = dht.getHumidity();
    readTemperature = dht.getTemperature();
    // TODO battery
  }

  // TODO OLED
}

void sendAdafruit() {
  if (io.status() >= AIO_CONNECTED && readTemperature && readHumidity) {
    temperature->save(readTemperature);
    humidity->save(readHumidity);
  }
}

void goodBye() {
  Heltec.display->clear();
  Heltec.display->setFont(Roboto_16);
  Heltec.display->drawString(64, 32, "Good bye");
  Heltec.display->display();
  delay(500);
}

void setup() {
  Heltec.begin(true, false, false);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  welcome();

  rtc_gpio_deinit(BUTTON_PIN);
  rtc_gpio_pulldown_en(BUTTON_PIN);
  esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
  btn.attach(BUTTON_PIN, INPUT_PULLUP);
  btn.setPressedState(LOW);

  readHumidity = readTemperature = 0;
  humidity = io.feed("humidity");
  temperature = io.feed("temperature");

  networkSetup();
}

void loop() {
  wm.process();
  btn.update();

  if (btn.pressed()) {
    delete humidity;
    delete temperature;
    goodBye();
    esp_deep_sleep_start();
  }
}