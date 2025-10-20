#include "ColorFormat.h"
#include <WiFi.h>
#include <ESP32Ping.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Variables.h>
#include <Prototypes.h>
#include <BlynkIntegration.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.windows.com", 19800, 60000);
WiFiClient MQTTClient;
PubSubClient client(MQTTClient);
MatrixPanel_I2S_DMA* dma_display = nullptr;

void ControllerReboot() {
  DisplayFadeOut();
  dma_display->clearScreen();
  dma_display->drawRect(0, 0, 96, 48, Blue);
  dma_display->setTextSize(1);
  dma_display->setTextColor(White);
  dma_display->setCursor(21, 20);
  dma_display->print("Rebooting");
  DisplayFadeIn();

  delay(3000);
  digitalWrite(Reboot, HIGH);
}

void DisplayFadeIn() {
  for (int i = 0; i <= DisplayBrightness; i++) {
    dma_display->setBrightness(i);
    delay(3);
  }
}

void DisplayFadeOut() {
  for (int i = DisplayBrightness; i >= 0; i--) {
    dma_display->setBrightness(i);
    delay(3);
  }
}

void FanControl() {
  if (CPUTemp < 65) {
    FanAmplitude = 0;
    ledcWrite(Fan, 0);
    return;
  }
  if (CPUTemp > 75) {
    FanAmplitude = 100;
  } else {
    FanAmplitude = map(CPUTemp, 65, 75, 30, 100);
  }
  ledcWrite(Fan, map(FanAmplitude, 30, 100, 1228, 4095));
}

void SendPowerSignal(int Duration) {
  Blynk.virtualWrite(V8, "Sending Power Command");
  digitalWrite(PowerPin, LOW);
  delay(Duration);
  digitalWrite(PowerPin, HIGH);
}

void PowerCommand(String Command) {
  if (Command == "BOOT") {
    Blynk.virtualWrite(V8, "Booting Up");
    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Blue);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(18, 20);
    dma_display->print("Booting Up");
    DisplayFadeIn();

    SendPowerSignal(500);
    Timeout = 1;
    Blynk.virtualWrite(V8, String("Attempt: ") + Timeout);
    while (!Pinger(true)) {
      Timeout++;
      if (Timeout == 6) {
        Blynk.virtualWrite(V8, "Boot Failed");

        DisplayFadeOut();
        dma_display->clearScreen();
        dma_display->drawRect(0, 0, 96, 48, Red);
        dma_display->setTextSize(1);
        dma_display->setTextColor(White);
        dma_display->setCursor(15, 20);
        dma_display->print("Boot Failed");
        DisplayFadeIn();

        delay(3000);
        return;
      } else {
        Blynk.virtualWrite(V8, String("Attempt: ") + Timeout);
      }
    }
    Blynk.virtualWrite(V8, "Boot Successful");

    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(4, 20);
    dma_display->print("Boot Successful");
    DisplayFadeIn();

    delay(3000);
  } else if (Command == "SHUTDOWN") {
    Blynk.virtualWrite(V8, "Shutting Down");

    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Blue);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(9, 20);
    dma_display->print("Shutting Down");
    DisplayFadeIn();

    SendPowerSignal(500);
    Timeout = 45;
    while (digitalRead(Sense)) {
      delay(1000);
      Timeout--;
      if (Timeout == 0) {
        Blynk.virtualWrite(V8, "Shutdown Unsuccessful");

        DisplayFadeOut();
        dma_display->clearScreen();
        dma_display->drawRect(0, 0, 96, 48, Red);
        dma_display->setTextSize(1);
        dma_display->setTextColor(White);
        dma_display->setCursor(23, 16);
        dma_display->print("Shutdown");
        dma_display->setCursor(12, 25);
        dma_display->print("Unsuccessful");
        DisplayFadeIn();

        delay(3000);
        return;
      }
    }
    Drops = 5;
    Blynk.virtualWrite(V8, "Shutdown Successful");

    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(23, 16);
    dma_display->print("Shutdown");
    dma_display->setCursor(18, 25);
    dma_display->print("Successful");
    DisplayFadeIn();

    delay(3000);
  }
}

bool isOfflineTime() {
  if (ServerOffTime < ServerOnTime) {
    return (SystemTime >= ServerOffTime) && (SystemTime < ServerOnTime);
  } else {
    return (SystemTime >= ServerOffTime) || (SystemTime < ServerOnTime);
  }
}

bool isNightModeTime() {
  if (NightModeOnTime < NightModeOffTime) {
    return (SystemTime >= NightModeOnTime) && (SystemTime < NightModeOffTime);
  } else {
    return (SystemTime >= NightModeOnTime) || (SystemTime < NightModeOffTime);
  }
}

void NightModeRoutine() {
  if (isNightModeTime()) {
    if (!NightMode) {
      DisplayFadeOut();
      dma_display->clearScreen();
      dma_display->drawRect(0, 0, 96, 48, Blue);
      dma_display->setTextSize(1);
      dma_display->setTextColor(White);
      dma_display->setCursor(12, 20);
      dma_display->print("Good Night:)");
      DisplayFadeIn();

      delay(2000);
      DisplayFadeOut();
      DisplayBrightness = 0;

      Duration = 50000;
      Blynk.virtualWrite(V5, 0);
      LEDBrightnessControl(NightLightBrightness, 0);
      NightLightBrightness = 0;
      NightMode = true;
    }
  } else {
    if (NightMode) {
      Duration = 10000;
      Blynk.virtualWrite(V5, 4096);
      NightLightBrightness = 4096;
      LEDBrightnessControl(0, 4096);

      DisplayBrightness = 200;
      DisplayFadeIn();
    }
    NightMode = false;
  }
}

void LEDBrightnessControl(int PreviousBrightness, int CurrentBrightness) {
  int Amplitude = abs(PreviousBrightness - CurrentBrightness) / 2;
  int Offset = (PreviousBrightness + CurrentBrightness) / 2;
  float StartAngle, EndAngle;

  if (PreviousBrightness < CurrentBrightness) {
    StartAngle = 270.0;
    EndAngle = 450.0;
  } else if (PreviousBrightness > CurrentBrightness) {
    StartAngle = 450.0;
    EndAngle = 270.0;
  } else {
    Duration = 250;
    return;
  }

  if (StartAngle < EndAngle) {
    for (float i = StartAngle; i <= EndAngle; i += 0.1) {
      ledcWrite(NightLight, Offset + (Amplitude * sin(radians(i))));
      delayMicroseconds(Duration);
    }
  } else {
    for (float i = StartAngle; i >= EndAngle; i -= 0.1) {
      ledcWrite(NightLight, Offset + (Amplitude * sin(radians(i))));
      delayMicroseconds(Duration);
    }
  }
  Duration = 250;
}

void ControllerRebootRoutine() {
  if (SystemTime == ControllerRebootTime) {
    ControllerReboot();
  }
}

void Debugger() {
  FormattedTime = timeClient.getFormattedTime();
  if (SystemSeconds != Buffer) {
    if (true) {
      Blynk.virtualWrite(V8, String("[") + String(FormattedTime) + String("]") + String(" RSSI: ") + String(WiFi.RSSI()) + String(" | ") + String("Power: ") + String(PowerStatus) + String(" | ") + String("Network: ") + String(NetworkStatus) + String(" | ") + String("Fan: ") + String(FanAmplitude) + String("%"));
    } else {
      Blynk.virtualWrite(V8, String("[") + String(FormattedTime) + String("]") + String(" RSSI: ") + String(WiFi.RSSI()) + String(" | ") + String("Power: ") + String(PowerStatus) + String(" | ") + String("Network: ") + String(NetworkStatus) + String(" | ") + String("Drops: ") + String(Drops));
    }
    Buffer = SystemSeconds;
  }
}

bool Pinger(bool Enforce) {
  if (Enforce) {
    for (Drops = 1; Drops <= Attempts; Drops++) {
      if (Ping.ping("10.1.1.3", 1)) {
        Drops = 0;
        return true;
      }
      if (Debug) Debugger();
    }
    return false;
  } else {
    if (Ping.ping("10.1.1.3", 1)) {
      Drops = 0;
      return true;
    } else {
      Drops++;
      if (Debug) Debugger();
      if (Drops >= 5) return false;
      return true;
    }
  }
}

void WiFiState() {
  if (WiFi.status() == WL_CONNECTED) return;
  Timeout = 120;
  while (WiFi.status() != WL_CONNECTED) {
    if (Timeout == 90) {
    }
    WiFi.reconnect();
    if (Timeout == 0) {
      ControllerReboot();
    }
    delay(1000);
    Timeout--;
  }
}

void StateUpdate() {
  PowerState = digitalRead(Sense);
  NetworkState = Pinger(false);
  WiFiState();

  PowerStatus = PowerState ? "On" : "Off";
  NetworkStatus = NetworkState ? "Online" : "Disconnected";

  if (PowerState && NetworkState) {
    State = 0;
  } else if (PowerState && !NetworkState) {
    State = 1;
  } else if (!PowerState && !NetworkState) {
    State = 2;
  }
}

void AnnounceState(uint8_t State) {
  static uint8_t CurrentState = -1;
  if (State != CurrentState) {
    switch (State) {
      case 0:
        Blynk.setProperty(V1, V2, V3, V5, V6, V7, V8, V9, "color", "#23C48E");
        Blynk.setProperty(V0, V4, "isHidden", false);
        Blynk.virtualWrite(V1, "HomeLab is Online");
        Blynk.virtualWrite(V3, HIGH);
        Blynk.setProperty(V3, "onLabel", "Shutdown");
        Blynk.setProperty(V3, "offLabel", "Shutdown");
        Blynk.virtualWrite(V8, "HomeLab is Online");

        DisplayFadeOut();
        dma_display->clearScreen();
        dma_display->drawRect(0, 0, 96, 48, Green);
        dma_display->setTextColor(Green);
        dma_display->setTextSize(2);
        dma_display->setCursor(7, 12);
        dma_display->print("HomeLab");
        dma_display->setTextSize(1);
        dma_display->setCursor(30, 28);
        dma_display->print("Online");
        DisplayFadeIn();

        DisplayMetrics = true;
        MetricsScreenFade = true;
        break;
      case 1:
        Blynk.setProperty(V1, V2, V3, V5, V6, V7, V8, V9, "color", "#3A88D1");
        Blynk.setProperty(V0, V4, "isHidden", true);
        Blynk.virtualWrite(V1, "HomeLab is Disconnected");
        Blynk.virtualWrite(V4, "Metrics Unavailable");
        Blynk.virtualWrite(V3, HIGH);
        Blynk.setProperty(V3, "onLabel", "Shutdown");
        Blynk.setProperty(V3, "offLabel", "Shutdown");
        Blynk.virtualWrite(V8, "HomeLab is Disconnected");

        DisplayFadeOut();
        dma_display->clearScreen();
        dma_display->drawRect(0, 0, 96, 48, Blue);
        dma_display->setTextColor(Blue);
        dma_display->setTextSize(2);
        dma_display->setCursor(7, 12);
        dma_display->print("HomeLab");
        dma_display->setTextSize(1);
        dma_display->setCursor(12, 28);
        dma_display->print("Disconnected");
        DisplayFadeIn();

        DisplayMetrics = false;
        break;
      case 2:
        Blynk.setProperty(V1, V2, V3, V5, V6, V7, V8, V9, "color", "#F15C6D");
        Blynk.setProperty(V0, V4, "isHidden", true);
        Blynk.virtualWrite(V1, "HomeLab is Powered Off");
        Blynk.virtualWrite(V4, "Metrics Unavailable");
        Blynk.virtualWrite(V3, LOW);
        Blynk.setProperty(V3, "onLabel", "Boot");
        Blynk.setProperty(V3, "offLabel", "Boot");
        Blynk.virtualWrite(V8, "HomeLab is Powered Off");

        DisplayFadeOut();
        dma_display->clearScreen();
        dma_display->drawRect(0, 0, 96, 48, Red);
        dma_display->setTextColor(Red);
        dma_display->setTextSize(2);
        dma_display->setCursor(7, 12);
        dma_display->print("HomeLab");
        dma_display->setTextSize(1);
        dma_display->setCursor(28, 28);
        dma_display->print("Offline");
        DisplayFadeIn();

        DisplayMetrics = false;
        break;
    }
    CurrentState = State;
    delay(3000);
  }
}

void AutoController(int State) {
  if (State == 0) {
    if (isOfflineTime()) {
      Blynk.virtualWrite(V8, "Server Online within Offline Hours");
      DisplayFadeOut();
      dma_display->clearScreen();
      dma_display->drawRect(0, 0, 96, 48, Green);
      dma_display->setTextSize(1);
      dma_display->setTextColor(White);
      dma_display->setCursor(9, 11);
      dma_display->print("Server Online");
      dma_display->setCursor(30, 20);
      dma_display->print("within");
      dma_display->setCursor(9, 29);
      dma_display->print("Offline Hours");
      DisplayFadeIn();

      delay(3000);
      PowerCommand("SHUTDOWN");
      MetricsScreenFade = true;
    }
  } else if (State == 1) {
    Blynk.virtualWrite(V8, "Server Disconnected");
    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->drawRect(0, 0, 96, 48, Blue);
    dma_display->setTextSize(1);
    dma_display->setTextColor(White);
    dma_display->setCursor(18, 16);
    dma_display->print("Unexpected");
    dma_display->setCursor(9, 25);
    dma_display->print("Disconnection");
    DisplayFadeIn();

    delay(3000);
    PowerCommand("SHUTDOWN");
    MetricsScreenFade = true;
  } else if (State == 2) {
    if (!isOfflineTime()) {
      Blynk.virtualWrite(V8, "Server Powered Off within Online Hours");
      DisplayFadeOut();
      dma_display->clearScreen();
      dma_display->drawRect(0, 0, 96, 48, Red);
      dma_display->setTextSize(1);
      dma_display->setTextColor(White);
      dma_display->setCursor(6, 11);
      dma_display->print("Server Offline");
      dma_display->setCursor(30, 20);
      dma_display->print("within");
      dma_display->setCursor(12, 29);
      dma_display->print("Online Hours");
      DisplayFadeIn();

      delay(3000);
      PowerCommand("BOOT");
      MetricsScreenFade = true;
    }
  }
}

void drawIcon(int x, int y, int color, int icon[12][12]) {
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 12; j++) {
      dma_display->drawPixel(x + j, y + i, icon[i][j] * color);
    }
  }
}

void ScreenOne() {
  drawIcon(3, 15, Green, CPUIcon);
  dma_display->setCursor(17, 18);
  dma_display->setTextColor(SoftWhite);
  dma_display->print(String(CPUUsage) + "%");

  dma_display->drawRect(dma_display->getCursorX() + 1, 19, 92 - dma_display->getCursorX(), 4, White);
  dma_display->drawRect(dma_display->getCursorX() + 2, 20, 1 + (CPUUsage / 100.00) * (89 - dma_display->getCursorX()), 2, dma_display->color565((CPUUsage / 100.00) * 255.00, 255.00 - ((CPUUsage / 100.00) * 255.00), 0));

  int xCord;
  if (FanAmplitude < 10) xCord = 15;
  else if (FanAmplitude < 100) xCord = 12;
  else xCord = 9;

  drawIcon(xCord, 30, Green, TempIcon);
  if (CPUTemp > 75) {
    dma_display->fillRect(xCord + 4, 38, 3, 3, dma_display->color565(255, 0, 0));
    dma_display->drawFastVLine(xCord + 5, 34, 4, dma_display->color565(255, 0, 0));
  } else if (CPUTemp < 40) {
    dma_display->fillRect(xCord + 4, 38, 3, 3, dma_display->color565(0, 0, 255));
    dma_display->drawFastVLine(xCord + 5, 34, 4, dma_display->color565(0, 0, 255));
  } else {
    dma_display->fillRect(xCord + 4, 38, 3, 3, dma_display->color565(((CPUTemp - 40) / 35.00) * 255, 0, 255.00 - ((CPUTemp - 40) / 35.00) * 255));
    dma_display->drawFastVLine(xCord + 5, 34, 4, dma_display->color565(((CPUTemp - 40) / 35.00) * 255, 0, 255.00 - ((CPUTemp - 40) / 35.00) * 255));
  }
  dma_display->setCursor(xCord + 11, 33);
  dma_display->print(String(CPUTemp) + "C");

  drawIcon(dma_display->getCursorX() + 8, 30, Green, FanIcon);
  dma_display->setCursor(xCord + 52, 33);
  dma_display->print(String(FanAmplitude) + "%");
}

void ScreenTwo() {
  drawIcon(3, 15, Green, RAMIcon);
  dma_display->setCursor(17, 18);
  dma_display->setTextColor(SoftWhite);
  dma_display->print(String(RAMPercent) + "%");

  dma_display->drawRect(dma_display->getCursorX() + 1, 19, 92 - dma_display->getCursorX(), 4, White);
  dma_display->drawRect(dma_display->getCursorX() + 2, 20, 1 + (RAMPercent / 100.00) * (89 - dma_display->getCursorX()), 2, dma_display->color565((RAMPercent / 100.00) * 255.00, 255.00 - ((RAMPercent / 100.00) * 255.00), 0));

  drawIcon(3, 30, Green, DiskIcon);
  dma_display->setCursor(17, 33);
  dma_display->setTextColor(SoftWhite);
  dma_display->print(String(DiskPercent) + "%");

  dma_display->drawRect(dma_display->getCursorX() + 1, 34, 92 - dma_display->getCursorX(), 4, White);
  dma_display->drawRect(dma_display->getCursorX() + 2, 35, 1 + (DiskPercent / 100.00) * (89 - dma_display->getCursorX()), 2, dma_display->color565((DiskPercent / 100.00) * 255.00, 255.00 - ((DiskPercent / 100.00) * 255.00), 0));
}

void ScreenThree() {
  String spacer = "";

  if (UpSpeed < 10) {
    spacer = "  ";
  } else if (UpSpeed < 100) {
    spacer = " ";
  } else {
    spacer = "";
  }

  dma_display->setCursor(12, 18);
  dma_display->setTextColor(SoftWhite);
  dma_display->print("Up  :" + spacer + String((int)(UpSpeed)) + "Mbps");

  if (DownSpeed < 10) {
    spacer = "  ";
  } else if (DownSpeed < 100) {
    spacer = " ";
  } else {
    spacer = "";
  }

  dma_display->setCursor(12, 33);
  dma_display->setTextColor(SoftWhite);
  dma_display->print("Down:" + spacer + String((int)(DownSpeed)) + "Mbps");
}

void ShowMetrics() {
  if (MetricsScreenFade) {
    DisplayFadeOut();
    dma_display->clearScreen();
    dma_display->setCursor(9, 2);
    dma_display->setTextSize(1);
    dma_display->setTextColor(Green);
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->print("Uptime  " + UpTime);
    dma_display->drawFastHLine(0, 10, 96, Green);
    DisplayFadeIn();
    MetricsScreenFade = false;
  } else {
    dma_display->clearScreen();
    dma_display->setCursor(9, 2);
    dma_display->setTextSize(1);
    dma_display->setTextColor(Green);
    dma_display->drawRect(0, 0, 96, 48, Green);
    dma_display->print("Uptime  " + UpTime);
    dma_display->drawFastHLine(0, 10, 96, Green);
  }

  if (ScreenCounter < 5) ScreenOne();
  else if (ScreenCounter < 7) ScreenTwo();
  else if (ScreenCounter < 9) ScreenThree();

  ScreenCounter++;
  if (ScreenCounter == 9) ScreenCounter = 0;
}

void HandleMQTT() {
  if (!client.connected()) {
    if (client.connect("HomeLab Power Controller")) {
      client.subscribe(MQTTTopic);
    }
  }
}

void TimeUpdate() {
  timeClient.update();
  SystemHours = timeClient.getHours();
  SystemMinutes = timeClient.getMinutes();
  SystemSeconds = timeClient.getSeconds();
  SystemTime = (SystemHours * 3600) + (SystemMinutes * 60) + SystemSeconds;
}

void GetMetrics(char* topic, byte* payload, unsigned int length) {
  JSONData = "";
  for (int i = 0; i < length; i++) {
    JSONData += (char)payload[i];
  }

  StaticJsonDocument<512> doc;
  deserializeJson(doc, JSONData);

  CPUUsage = doc["cpu_usage"].as<int>();
  CPUTemp = doc["cpu_temp"].as<int>();
  RAMUsed = doc["ram_used"].as<float>();
  RAMTotal = doc["ram_total"].as<float>();
  DiskUsed = doc["disk_used"].as<float>();
  DiskTotal = doc["disk_total"].as<float>();
  UpTime = doc["uptime"].as<String>();
  UpSpeed = doc["up_speed"].as<float>();
  DownSpeed = doc["down_speed"].as<float>();
  TopCPUTask = doc["top_cpu_process"]["name"].as<String>();
  TopCPUUsage = doc["top_cpu_process"]["usage"].as<float>();
  TopRAMTask = doc["top_ram_process"]["name"].as<String>();
  TopRAMUsed = doc["top_ram_process"]["usage"].as<int>();
  RAMPercent = (RAMUsed / RAMTotal) * 100.00;
  DiskPercent = (DiskUsed / DiskTotal) * 100.00;

  Blynk.virtualWrite(V0, String("CPU ") + String(CPUUsage) + "% | RAM " + String(RAMPercent) + "% | " + String(CPUTemp) + "℃");
  if (Input == "CPU") {
    Blynk.virtualWrite(V4, TopCPUTask + ": " + String((TopCPUUsage), 1) + "%");
  } else if (Input == "RAM") {
    Blynk.virtualWrite(V4, TopRAMTask + ": " + String((TopRAMUsed / 1024.00), 1) + "GB");
  } else {
    Blynk.virtualWrite(V4, "[" + UpTime + "] | " + ((UpSpeed < 10.00) ? (String(UpSpeed, 1)) : (String(UpSpeed, 0))) + " ▲▼ " + ((DownSpeed < 10.00) ? (String(DownSpeed, 1)) : (String(DownSpeed, 0))) + " | HDD " + String(DiskPercent) + "%");
  }

  if (DisplayMetrics) ShowMetrics();
}
