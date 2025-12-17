#include <BlynkSimpleEsp32.h>

BLYNK_WRITE(V2) {
  ServerOffTime = param[0].asInt();
  ServerOnTime = param[1].asInt();
  if (abs(ServerOffTime - ServerOnTime) < 300) {
    Blynk.virtualWrite(V8, "Server Offline Duration cannot be less than 5 minutes");
    Blynk.virtualWrite(V2, LOW);
  } else {
    Blynk.virtualWrite(V8, "Server Offline Time Set");
  }
}
BLYNK_WRITE(V3) {
  if (param.asInt()) {
    if (AutoControl) {
      AutoControl = 0;
      Blynk.virtualWrite(V7, AutoControl);
      Blynk.syncVirtual(V7);
    }
    if (PowerState) {
      PowerCommand("SHUTDOWN");
      MetricsScreenFade = true;
    } else {
      PowerCommand("BOOT");
      MetricsScreenFade = true;
    }
  }
}
BLYNK_WRITE(V5) {
  LEDBrightnessControl(NightLightBrightness, param.asInt());
  NightLightBrightness = param.asInt();
  Blynk.virtualWrite(V8, "Night Light Brightness Set");
}
BLYNK_WRITE(V6) {
  NightModeOnTime = param[0].asInt();
  NightModeOffTime = param[1].asInt();
  Blynk.virtualWrite(V8, "Night Mode Time Set");
}
BLYNK_WRITE(V7) {
  AutoControl = param.asInt();
  if (AutoControl) {
    Blynk.virtualWrite(V8, "AutoControl Enabled");
  } else {
    Blynk.virtualWrite(V8, "AutoControl Disabled");
  }
}
BLYNK_WRITE(V8) {
  Input = param.asString();
  Input.toUpperCase();
  Input.trim();

  if (Input == "FORCE") {
    if (State != 2) {
      if (AutoControl) {
        AutoControl = 0;
        Blynk.virtualWrite(V7, AutoControl);
        Blynk.syncVirtual(V7);
      }
      Blynk.virtualWrite(V8, "Forcing Power Off");
      Serial.println("Forcing Power Off");
      SendPowerSignal(6000);
    } else {
      Blynk.virtualWrite(V8, "Server Already Powered Off");
      Blynk.virtualWrite(V8, "Cannot Force Power Off");
    }
  } else if (Input == "REBOOT") {
    Blynk.virtualWrite(V8, "Rebooting Power Controller");
    delay(2500);
    digitalWrite(Reboot, HIGH);
  } else if (Input == "DEBUG") {
    if (Debug) {
      Blynk.virtualWrite(V8, "Debugging Disabled");
      Debug = false;
    } else {
      Blynk.virtualWrite(V8, "Debugging Enabled");
      Debug = true;
    }
  } else if (Input == "CPU") {
    Blynk.virtualWrite(V8, "Showing Max CPU Usage");
  } else if (Input == "RAM") {
    Blynk.virtualWrite(V8, "Showing Max RAM Usage");
  } else if (Input == "CLEAR") {
  } else if (Input == "RAW") {
    Blynk.virtualWrite(V8, JSONData);
  } else if (Input.substring(0, 3) == "FAN") {
    if (Input.substring(4) == "AUTO" || Input.substring(4) == "") {
      Blynk.virtualWrite(V8, "Fan Auto Control Enabled");
      FanFlag = false;
      return;
    } else {
      if (!FanFlag) {
        Blynk.virtualWrite(V8, "Fan Auto Control Disabled");
        FanFlag = true;
      }
    }
    FanAmplitude = Input.substring(4).toInt();
    if (FanAmplitude > 100) {
      FanAmplitude = 100;
    } else if (FanAmplitude < 0) {
      FanAmplitude = 0;
    }
    Blynk.virtualWrite(V8, "Fan Speed set to " + String(FanAmplitude) + "%");
    ledcWrite(Fan, map(FanAmplitude, 0, 100, 0, 4095));
  } else if (Input == "BOOT") {
    SendPowerSignal(500);
  } else if (Input.substring(0, 8) == "DISPLAY ") {
    DisplayFadeOut();
    DisplayBrightness =  Input.substring(8).toInt());
    DisplayFadeIn();
    Blynk.virtualWrite(V8, "Diplay brightness set to " + Input.substring(8));
  } else {
    Blynk.virtualWrite(V8, "Invalid Input");
  }
}
BLYNK_WRITE(V9) {
  ControllerRebootTime = param[0].asInt();
  Blynk.virtualWrite(V8, "Power Controller Reboot Time Set");
}
