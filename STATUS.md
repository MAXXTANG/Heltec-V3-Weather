# Heltec V3 Weather — 工作狀態

> 最後更新：2026/11/10
> 正式工程進度入口。首次整理基線：main `c16f2401a7993745c6f2b62de37a814396607d5d`。本次協作文件建置不改韌體行為。

## 目前狀態

原始碼版本 v1.2.4；既有天氣／時鐘手動切頁、倒數與省電行為見 [README](README.md)。詳細歷史驗證保留在 [BUILD-VALIDATION](docs/BUILD-VALIDATION.json)。

## 來源記載已驗證（本輪未重跑）

- v1.2.4 原生測試（ASan／UBSan）與 ESP32-S3 編譯：報告記載 passed。
- 2026-10-29 原字型 v1.1.10：相同倒數與控制邏輯的 75 秒裝置驗證通過；按鍵由 USB DTR 模擬，未按機械按鍵。

## 待驗證與限制

- 公開字型 v1.2.4 二進位尚未刷入實機。
- 閃爍有原生邊界與像素測試，但沒有相機量測。
- 平均電流、實際續航、充電狀態偵測沒有新增量測或硬體支援。
- Codex → Claude → Codex：2026-11-10 已完成 Claude 唯讀接手（見 [handoff](handoffs/2026-11-10-2309-claude-readonly-validation.md)），待 Codex 核對原始輸出後才算通過。

## 本輪工作與下一步

本輪建立 AGENTS／CLAUDE、進度入口與 [交接目錄](handoffs/README.md)；沒有重跑工程測試或修改歷史驗證數值。

1. 由 Codex 核對 2026-11-10 Claude 唯讀接手的原始輸出，通過後在新 handoff 標示結果。
2. 後續若使用者要求硬體驗證，先核對板型、目標裝置與備份，再驗證公開版；未執行前保持待驗證。

提交與遠端同步結果以 Git 歷史與實際 push 結果為準；跨工具驗收證據寫入新的 handoff。
