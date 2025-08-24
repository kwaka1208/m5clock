#include <M5Stack.h>
#include <WiFi.h>
#include "time.h"
#include "env.h" // Wi-FiのSSIDとパスワードをenv.hに分離

// --- NTPサーバー設定 ---
const char* ntpServer = "ntp.nict.jp";
// タイムゾーン設定 (日本標準時 JST)
// UTCからのオフセット秒 (9時間 * 60分 * 60秒)
const long gmtOffset_sec = 9 * 3600; 
// 夏時間（デイライトセービング）のオフセット秒 (日本は0)
const int daylightOffset_sec = 0;

// 曜日を日本語で表示するための配列
const char* weekDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// 時刻情報を画面に表示する関数
void printLocalTime() {
  struct tm timeinfo;
  // getLocalTimeが失敗した場合はエラーメッセージを表示
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to obtain time");
    return;
  }

  // 画面を黒でクリア
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);

  // --- 日付の表示 ---
  // YYYY-MM-DD (曜日) の形式で文字列を生成
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d (%s)", 
            timeinfo.tm_year + 1900, 
            timeinfo.tm_mon + 1, 
            timeinfo.tm_mday,
            weekDay[timeinfo.tm_wday]);

  M5.Lcd.setCursor(25, 40);
  M5.Lcd.setTextSize(3); // 文字サイズを3に設定
  M5.Lcd.print(dateStr);

  // --- 時刻の表示 ---
  // HH:MM:SS の形式で文字列を生成
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  M5.Lcd.setCursor(45, 120);
  M5.Lcd.setTextSize(5); // 文字サイズを6に設定
  M5.Lcd.print(timeStr);
}

void setup() {
  // M5Stackの初期化
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setBrightness(100); // 画面の明るさを設定

  // 画面に接続中メッセージを表示
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("Connecting to %s\n", SSID);

  // Wi-Fiに接続
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nWiFi connected.");

  // NTPサーバーから時刻を取得して設定
  M5.Lcd.println("Getting time from NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // 最初の時刻表示
  printLocalTime();

  // 時刻取得後はWi-FiをOFFにして省電力化
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  M5.Lcd.println("WiFi disconnected.");
  delay(1000); // メッセージを1秒表示
}

void loop() {
  // 1秒ごとに時刻を更新して表示
  printLocalTime();
  delay(1000);
}
