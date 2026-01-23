#include "M5UnitENV.h"
#include "env.h" // Wi-FiのSSIDとパスワードをenv.hに分離
#include "time.h"
#include <M5Stack.h>
#include <WiFi.h>

// --- NTPサーバー設定 ---
const char *ntpServer = "ntp.nict.jp";
// タイムゾーン設定 (日本標準時 JST)
// UTCからのオフセット秒 (9時間 * 60分 * 60秒)
const long gmtOffset_sec = 9 * 3600;
// 夏時間（デイライトセービング）のオフセット秒 (日本は0)
const int daylightOffset_sec = 0;

// 曜日を日本語で表示するための配列
const char *weekDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

SHT3X sht30;
QMP6988 qmp6988;

// 時刻情報を画面に表示する関数
void printLocalTime() {
  struct tm timeinfo;
  // getLocalTimeが失敗した場合はエラーメッセージを表示
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to obtain time");
    return;
  }

  // 画面を黒でクリア (削除)
  // M5.Lcd.fillScreen(BLACK);
  // 文字色を白、背景を黒に設定して上書き描画するように変更
  M5.Lcd.setTextColor(WHITE, BLACK);

  // --- 日付の表示 ---
  // YYYY-MM-DD (曜日) の形式で文字列を生成
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d (%s)", timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1, timeinfo.tm_mday, weekDay[timeinfo.tm_wday]);

  M5.Lcd.setCursor(25, 40);
  M5.Lcd.setTextSize(3); // 文字サイズを3に設定
  M5.Lcd.print(dateStr);

  // --- 時刻の表示 ---
  // HH:MM の形式で文字列を生成
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  M5.Lcd.setCursor(85, 100); // センタリング (HH:MM 5文字 * font5(約30px) =
                             // 150px, (320-150)/2 = 85)
  M5.Lcd.setTextSize(5);
  M5.Lcd.print(timeStr);

  // --- センサー情報の表示 ---
  if (sht30.update()) {
    float temp = sht30.cTemp;
    float hum = sht30.humidity;
    float pressure = qmp6988.calcPressure() / 100.0f; // Pa -> hPa

    M5.Lcd.setCursor(20, 170);
    M5.Lcd.setTextSize(2);
    // 桁数が変わっても残像が残らないように幅指定を追加
    M5.Lcd.printf("Temp: %5.1fC Hum: %5.1f%%", temp, hum);

    M5.Lcd.setCursor(20, 200);
    M5.Lcd.printf("Pressure: %6.1f hPa", pressure);
  }
}

void setup() {
  // M5Stackの初期化
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setBrightness(100); // 画面の明るさを設定
  M5.Lcd.fillScreen(BLACK);  // 初回のみ画面をクリア

  // センサー初期化
  if (!qmp6988.begin(&Wire, 0x70, 21, 22, 400000)) {
    M5.Lcd.println("QMP6988 Init Fail");
  }
  if (!sht30.begin(&Wire, 0x44, 21, 22, 400000)) {
    M5.Lcd.println("SHT30 Init Fail");
  }

  // 画面に接続中メッセージを表示
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("Connecting to %s\n", ENV_SSID);

  // Wi-Fiに接続
  WiFi.begin(ENV_SSID, ENV_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nWiFi connected.");

  // NTPサーバーから時刻を取得して設定
  M5.Lcd.println("Getting time from NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // 最初の時刻表示
  M5.Lcd.fillScreen(BLACK);
  printLocalTime();

  // 時刻取得後はWi-FiをOFFにして省電力化
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop() {
  // 1秒ごとに時刻を更新して表示
  printLocalTime();
  delay(500);
}
