# 宜蘭災情版 Release

日期：2026-11-06；repo MAXXTANG/Heltec-V3-Weather；分支 宜蘭災情版；基線 5e2280040678df97980e70a90af3c5c5416bff02。正式狀態：STATUS.md。

使用者明確要求發布韌體與還原說明，因此二進位只以 Release 附件提供，不加入 Git 歷史。未改動韌體來源；變更為 README、STATUS、第三方標示、FLASH-RESTORE 指南與交接。

實際檢查：app SHA-256 與已實測版本一致，esptool 4.5.1 映像校驗通過；安裝映像組件與位址、空白 NVS、SHA-256 及 preflight 成功／拒絕路徑離線檢查。無再次刷機，完整安裝路徑未經實機驗收，真實新案與光學閃爍未測。原先原生／硬體測試結果保留在 docs/VALIDATION.md。

產物包括 firmware ZIP、完整專案 source ZIP、對應 Arduino 框架來源 ZIP、獨立 app、刷機指南、SHA256SUMS。授權文字保留在 firmware ZIP；不含裝置備份、NVS、私人憑證或原始日誌。

本記錄在發布前寫入；同步結果以 v1.4.1-yilan 標籤與 GitHub Release 為準，發布流程最後需重新下載附件並比對 SHA-256。原 main 與先前提交不改寫。後續若改韌體，另發新版本，不覆蓋此版附件。
