# M5Stack Clock with ENV Unit

M5Stack Basic/Gray/Go と ENV III Unit を使用した、環境センサ付き時計アプリケーションです。

## 機能

- **NTP時刻同期**: WiFi経由でNTPサーバーから正確な時刻を取得します。
- **デジタル時計表示**: 大きなフォントで現在の時刻 (HH:MM) と日付を表示します。
- **環境データ表示**: 温度、湿度、気圧をリアルタイムで表示します。

## ハードウェア要件

- **M5Stack Core** (Basic, Gray, Go, etc.)
- **M5Stack ENV III Unit** (SHT30 + QMP6988)
    - Port A (I2C) に接続

## ソフトウェア構成

- **Platform**: PlatformIO
- **Framework**: Arduino
- **Libraries**:
    - M5Stack
    - M5Unit-ENV

## セットアップ

1. `include/env.h` ファイルを作成し、Wi-Fi設定を記述してください（リポジトリには含まれていません）。
   ```cpp
   // include/env.h
   #ifndef ENV_H
   #define ENV_H

   const char* ENV_SSID = "YOUR_WIFI_SSID";
   const char* ENV_PASS = "YOUR_WIFI_PASSWORD";

   #endif
   ```
2. M5StackにENV III Unitを接続します。
3. プロジェクトをビルドし、M5Stackに書き込みます。

## ライセンス

[MIT License](LICENSE)