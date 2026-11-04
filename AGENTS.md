# Heltec V3 Weather 共用協作規則

> 最後更新：2026/11/04

## 開始工作

- 先讀 `README.md`、`STATUS.md`、`docs/BUILD-VALIDATION.json`、`platformio.ini` 與 `handoffs/README.md` 指向的交接；確認分支、HEAD 與未提交變更。
- 本專案是 Heltec WiFi LoRa 32 V3 的 Wi-Fi 天氣時鐘；不把 V3、Wireless Paper 或其他板型視為可互刷。
- 使用者當次要求決定範圍；文件的未來工作與歷史提示不構成執行授權。

## 實作與驗證

- 沿用已鎖定的 PlatformIO 與函式庫版本；build／test 完整入口見 README。
- 繪圖、排程、電池或倒數邏輯改動後執行適用的 `sh tests/run.sh`；需要韌體編譯時依 README 使用 `python tools/build.py`。記錄實際命令、結果與未測項。
- Host／原生測試、ESP32-S3 編譯與硬體驗證分開標示。原字型 v1.1.10 的歷史實測不能當成公開字型 v1.2.4 已刷機的證據。
- 無量測就不宣稱續航、平均電流、實際閃爍率或正在充電；原板充電狀態無 MCU 訊號，不用高電壓推定充電。
- 不提交 Wi-Fi 憑證、私有位置、Flash／NVS 備份、未授權字型或產生的二進位。沿用 MIT／OFL 與第三方標示。
- 燒錄、清除 NVS 或變更實機需符合當次授權與明確裝置識別；一般文件／交接工作不觸發硬體操作。

## 進度與跨 AI 交接

- 當前工程進度與下一步以 `STATUS.md` 為準；`docs/BUILD-VALIDATION.json` 保留版本化驗證證據，不因補協作文件就改寫測試結果。
- 收工新增一份 `handoffs/` 記錄，包含基線、修改檔案、實測／未測項、未提交變更與同步結果；同一工作目錄一次一位執行者寫入。
- 專案方向／知識總覽（本機可選）：`~/Obsidian/Mas-Vault/10 Projects/Heltec-V3-Weather/專案總覽.md`。不可讀時可依 repo 繼續，標記待補摘要；公開 repo 不複製私人筆記。
- Vault 的工程摘要附來源版本與查核日期；歷史交接不取代最新狀態，檔案不代表聊天與私人記憶已共用。
