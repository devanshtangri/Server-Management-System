#include <ArduinoOTA.h>
#include <Methods.h>

void setup() {
  pinMode(NightLight, OUTPUT);
  pinMode(Fan, OUTPUT);
  digitalWrite(NightLight, LOW);
  digitalWrite(Fan, LOW);
  
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN);

  mxconfig.gpio.e = 33;
  mxconfig.clkphase = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness(0);
  dma_display->clearScreen();

  White = dma_display->color565(155, 155, 155);
  SoftWhite = dma_display->color565(125, 125, 125);
  Red = dma_display->color565(255, 0, 0);
  Green = dma_display->color565(0, 185, 70);
  Blue = dma_display->color565(0, 0, 255);

  dma_display->drawRect(0, 0, 96, 48, Green);
  dma_display->setTextSize(2);
  dma_display->setTextColor(White);
  dma_display->setCursor(7, 16);
  dma_display->print("HomeLab");

  DisplayFadeIn();

  delay(3000);

  dma_display->drawRect(3, 33, 90, 4, White);

  pinMode(Sense, INPUT);
  pinMode(PowerPin, OUTPUT);
  pinMode(Reboot, OUTPUT);
  digitalWrite(PowerPin, HIGH);
  digitalWrite(Reboot, LOW);

  dma_display->drawRect(4, 34, 8, 2, Green);

  WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.begin(ssid, pass, 0, bssid);

  dma_display->drawRect(4, 34, 23, 2, Green);

  Timeout = 10;
  while (WiFi.status() != WL_CONNECTED) {
    if (Timeout == 0) {
      ControllerReboot();
    } else if (Timeout == 5)
      delay(1000);
    Timeout--;
  }

  Blynk.config(BLYNK_AUTH_TOKEN);
  if (!Blynk.connect(10000)) {
    ControllerReboot();
  }

  dma_display->drawRect(4, 34, 34, 2, Green);

  ArduinoOTA.setPort(11111);
  ArduinoOTA.setHostname("HomeLab-Controller");
  ArduinoOTA.setRebootOnSuccess(false);
  ArduinoOTA.onStart([]() {
    OTADuration = micros();
    Blynk.virtualWrite(V8, "OTA Update Starting");
    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(18, 17);
    dma_display->print("OTA Update");
    dma_display->drawRect(14, 28, 67, 4, White);
    DisplayFadeIn();
  });
  ArduinoOTA.onProgress([](float Progress, float Total) {
    SketchSize = Total;
    static bool Written = false;
    static int PreviousPercent;

    int ProgressPercent = (Progress / Total) * 100.00;
    if (ProgressPercent != PreviousPercent) {
      Written = false;
    }
    if (ProgressPercent % 2 == 0 && !Written) {
      Blynk.virtualWrite(V8, "Progress: " + String(ProgressPercent) + "%");
      dma_display->drawRect(15, 29, 65 * (ProgressPercent / 100.00), 2, Green);
      Written = true;
    }
    PreviousPercent = ProgressPercent;
  });
  ArduinoOTA.onError([](ota_error_t Error) {
    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Red);
    dma_display->setTextSize(1);
    dma_display->setTextColor(Red);
    dma_display->setCursor(12, 20);
    dma_display->print("Update Error");
    DisplayFadeIn();

    delay(3000);
    ControllerReboot();
  });
  ArduinoOTA.onEnd([]() {
    OTADuration = micros() - OTADuration;
    Blynk.virtualWrite(V8, "OTA Update Completed in " + String(OTADuration / 1000000.00, 1) + " seconds");
    Blynk.virtualWrite(V8, "Effective Upload Speed " + String((SketchSize / 1024.00) / (OTADuration / 1000000.00), 1) + " KB/s");

    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->setTextSize(1);
    dma_display->setTextColor(Green);
    dma_display->setCursor(30, 16);
    dma_display->print("Update");
    dma_display->setCursor(24, 25);
    dma_display->print("Complete");
    DisplayFadeIn();

    delay(3000);
    ControllerReboot();
  });
  ArduinoOTA.begin();

  dma_display->drawRect(4, 34, 46, 2, Green);

  client.setServer(MQTTServer, MQTTPort);
  client.setCallback(GetMetrics);
  client.setBufferSize(512);

  dma_display->drawRect(4, 34, 54, 2, Green);

  Timeout = 10;
  while (!timeClient.update()) {
    delay(1000);
    if (Timeout == 10) {
      Blynk.virtualWrite(V8, "Connecting to NTP Server");
    } else if (Timeout == 5) {
      Blynk.virtualWrite(V8, "Power Controller will Reboot in 5 Seconds");
    }
    Timeout--;
    if (Timeout == 0) {
      Blynk.virtualWrite(V8, "Connecting to NTP Server Failed");
      delay(3000);
      ControllerReboot();
    }
  }

  dma_display->drawRect(4, 34, 65, 2, Green);

  ledcAttach(NightLight, 5000, 12);
  ledcAttach(Fan, 19000, 12);
  ledcWrite(NightLight, 0);
  ledcWrite(Fan, 0);

  dma_display->drawRect(4, 34, 73, 2, Green);

  Blynk.virtualWrite(V8, "Synchronizing Settings");

  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V7);
  Blynk.syncVirtual(V9);

  dma_display->drawRect(4, 34, 88, 2, Green);
  delay(1000);
  TimeUpdate();
}
void loop() {
  Blynk.run();
  TimeUpdate();
  NightModeRoutine();
  ControllerRebootRoutine();
  StateUpdate();
  AnnounceState(State);
  if (AutoControl) AutoController(State);
  if (Debug && (Drops == 0)) Debugger();
  if (State == 0) {
    HandleMQTT();
    client.loop();
    if (!FanFlag) { FanControl(); }
  }
  ArduinoOTA.handle();
}
