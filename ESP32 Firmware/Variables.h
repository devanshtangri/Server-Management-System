#include <Icons.h>

// Blynk Credentials
#define BLYNK_TEMPLATE_ID "Your-Template-ID"
#define BLYNK_TEMPLATE_NAME "HomeLab Power Controller"
#define BLYNK_AUTH_TOKEN "Your-Blynk-Token"

// Physical Pin Definitions
#define Reboot 2
#define Sense 34
#define NightLight 21
#define PowerPin 32
#define Fan 19

// Display Configuration
#define PANEL_RES_X 96
#define PANEL_RES_Y 48
#define PANEL_CHAIN 1

// Network Credentials
char ssid[] = "YourSSID";
char pass[] = "YourPassword";
char MQTTServer[] = "Static-IP-of-the-server";
char MQTTTopic[] = "Metrics";
int MQTTPort = 11111; // Change this to your MQTT Port
byte bssid[] = { 0x6A, 0x42, 0xA1, 0xAD, 0x53, 0xE8 };

IPAddress local_IP(10, 1, 1, 4); // Change this to set local IP of the ESP
IPAddress gateway(10, 1, 1, 1); // Change this to your Gateway Address
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1);

// Program Variables
uint16_t White, SoftWhite, Red, Green, Blue, Duration = 250;

uint8_t Timeout, DisplayBrightness = 200, Buffer = 0, State, Drops = 5, ScreenCounter = 0;
int SystemHours, SystemMinutes, SystemSeconds, SystemTime, ServerOffTime, ServerOnTime, ControllerRebootTime, NightModeOnTime, NightModeOffTime;
int OTADuration, FanAmplitude, SketchSize, NightLightBrightness = 0;
String FormattedTime, PowerStatus, NetworkStatus, Input;
const uint8_t Attempts = 30;
bool NetworkState = false, PowerState = false, AutoControl = false, MetricsScreenFade = true;
bool NightMode = false, Debug = true, FanFlag = false, DisplayMetrics = false;

float RAMUsed, RAMTotal, DiskUsed, DiskTotal, TopCPUUsage, TopRAMUsed, UpSpeed, DownSpeed;
int CPUUsage, CPUTemp, RAMPercent, DiskPercent;
String JSONData, TopCPUTask, TopRAMTask, UpTime;
