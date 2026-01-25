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

// Wi-Fi接続ステータス
bool isOffline = false;

// 自動減光制御用
unsigned long lastActivityTime = 0;
const int BRIGHTNESS_HIGH = 100;
const int BRIGHTNESS_LOW = 10;
const unsigned long DIM_DELAY_MS = 5000;
int currentBrightness = BRIGHTNESS_HIGH;
unsigned long lastUpdateDisplayTime = 0;

// 時刻・センサー情報を画面に表示する関数
void updateDisplay() {
  struct tm timeinfo;
  bool timeAvailable = false;

  // オフラインでなければ時刻取得を試みる
  if (!isOffline) {
    if (getLocalTime(&timeinfo)) {
      timeAvailable = true;
    }
  }

  M5.Lcd.setTextColor(WHITE, BLACK);

  // --- 日付・時刻の表示 ---
  if (timeAvailable) {
    // YYYY-MM-DD (曜日)
    char dateStr[20];
    sprintf(dateStr, "%04d-%02d-%02d (%s)", timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1, timeinfo.tm_mday, weekDay[timeinfo.tm_wday]);

    M5.Lcd.setCursor(25, 40);
    M5.Lcd.setTextSize(3);
    M5.Lcd.print(dateStr);

    // HH:MM
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

    M5.Lcd.setCursor(85, 100);
    M5.Lcd.setTextSize(5);
    M5.Lcd.print(timeStr);
  } else {
    // 時刻が表示できない場合のステータス表示
    M5.Lcd.fillRect(0, 40, 320, 120, BLACK);
    M5.Lcd.setCursor(20, 80);
    M5.Lcd.setTextSize(3);
    if (isOffline) {
      M5.Lcd.print("   OFFLINE   ");
    } else {
      M5.Lcd.print(" Syncing Time.. ");
    }
  }

  // --- センサー情報の表示 ---
  // update()がfalseでも前回の値を表示するか、あるいはエラーを表示するのが親切だが
  // ここではupdateに失敗しても描画更新を行わない（前回値が残る）挙動はそのままに、
  // デバッグ用に明示的に呼び出す。
  bool sensorUpdated = sht30.update();

  if (sensorUpdated) {
    float temp = sht30.cTemp;
    float hum = sht30.humidity;
    float pressure = qmp6988.calcPressure() / 100.0f; // Pa -> hPa

    M5.Lcd.setCursor(20, 170);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("Temp: %5.1fC Hum: %5.1f%%", temp, hum);

    M5.Lcd.setCursor(20, 200);
    M5.Lcd.printf("Pressure: %6.1f hPa", pressure);
  } else {
    // センサー更新失敗時
    // (長時間更新されない場合の対策は別途必要だが、とりあえず何もしない)
    // 必要であれば "Sensor Error" 表示を追加
  }
}

void setup() {
  // M5Stackの初期化
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setBrightness(BRIGHTNESS_HIGH); // 画面の明るさを設定
  lastActivityTime = millis();
  M5.Lcd.fillScreen(BLACK); // 初回のみ画面をクリア

  // センサー初期化
  if (!qmp6988.begin(&Wire, 0x70, 21, 22, 400000)) {
    M5.Lcd.println("QMP6988 Init Fail");
  }
  if (!sht30.begin(&Wire, 0x44, 21, 22, 400000)) {
    M5.Lcd.println("SHT30 Init Fail");
  }

  // Wi-Fi接続試行
  bool connected = false;
  M5.Lcd.setTextSize(2);

  for (int i = 0; i < wifiConfigCount; i++) {
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.fillRect(0, 10, 320, 30, BLACK); // メッセージエリアクリア
    M5.Lcd.printf("Connecting to %s ...", wifiConfigs[i].ssid);

    WiFi.begin(wifiConfigs[i].ssid, wifiConfigs[i].password);

    // 接続待ち (タイムアウト設定: 10秒程度)
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 5) {
      delay(500);
      M5.Lcd.print(".");
      retryCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      M5.Lcd.println("\nWiFi connected.");
      break;
    } else {
      M5.Lcd.println("\nFailed.");
    }
  }

  if (connected) {
    // NTPサーバーから時刻を取得して設定
    M5.Lcd.println("Getting time from NTP server...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // 時刻同期待ち
    struct tm timeinfo;
    // getLocalTimeで時刻が取得できるまで待機 (最大20秒程度)
    // configTime後、すぐに切断すると同期できないため
    int retrySync = 0;
    while (!getLocalTime(&timeinfo, 1000) && retrySync < 20) {
      M5.Lcd.print(".");
      retrySync++;
    }

    if (retrySync < 20) {
      M5.Lcd.println("\nTime Synced!");
    } else {
      M5.Lcd.println("\nTime Sync Failed.");
      // 同期失敗してもとりあえず進むが、次回からオフライン扱いにするか？
      // ここではそのまま進む（時刻はずれている可能性がある）
    }

    // 時刻取得後はWi-FiをOFFにして省電力化
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } else {
    isOffline = true;
    M5.Lcd.println("WiFi connection failed.");
    M5.Lcd.println("Starting in Offline Mode...");
    delay(2000);
  }

  // 画面クリアしてメイン表示へ
  M5.Lcd.fillScreen(BLACK);
  updateDisplay();
}

void loop() {
  M5.update(); // ボタン状態更新

  // ボタンが押されたかチェック
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    lastActivityTime = millis();
    if (currentBrightness != BRIGHTNESS_HIGH) {
      currentBrightness = BRIGHTNESS_HIGH;
      M5.Lcd.setBrightness(currentBrightness);
    }
  }

  // 減光判定
  if (millis() - lastActivityTime > DIM_DELAY_MS) {
    if (currentBrightness != BRIGHTNESS_LOW) {
      currentBrightness = BRIGHTNESS_LOW;
      M5.Lcd.setBrightness(currentBrightness);
    }
  }

  // 1秒ごとに時刻を更新して表示 (非ブロッキング)
  if (millis() - lastUpdateDisplayTime > 1000) {
    updateDisplay();
    lastUpdateDisplayTime = millis();
  }

  delay(10); // 短いウェイト
}
