# 公開前隱私檢查 / Publication privacy review

Review date: 2026-10-26. Scope: the files and new Git history published in this
repository. The earlier private development repository remains private and its
history was not imported.

## 檢查結果

- 原始碼沒有預填使用者的 Wi-Fi SSID、家用 Wi-Fi 密碼、API token 或私人金鑰。
- Wi-Fi 設定只透過裝置配網頁面寫入該裝置的 NVS；配網 AP 名稱／密碼是在裝置上
  執行時產生的，不包含某一台實機的識別碼常數。
- 公開檔案中未發現本機絕對路徑、已知私人裝置識別碼或已知本機使用者名稱。
- 新 Git 歷史不包含舊版二進位、序列紀錄、完整 Flash／NVS 備份、裝置設定檔、
  相依套件快取或本機工具。提交使用公開 GitHub 帳號和 GitHub noreply 郵件。
- 兩張實機照片已轉為新的 JPEG，未攜帶 EXIF、GPS、XMP 或原始 HEIC metadata。
  可見內容為使用者授權公開的裝置、手部與鍵盤，沒有可見 Wi-Fi 名稱或密碼。
- 圖片、封面與預覽均已檢查 metadata；封面標示為 AI 輔助插圖，與實拍分開。
- Gitleaks 8.30.1 使用預設規則、完整遮蔽輸出進行掃描，未發現秘密資料。

## 有意保留的公開範例

- `JIAOXI` 和其鎮中心座標是公開示範設定，不是照片 GPS 或裝置定位資料。
- `192.168.4.1` 是裝置配網 AP 的預設本機位址，不是使用者家中網路資訊。
- Open-Meteo URL、公開 NTP 主機與 ISRG Root X1 公開憑證是服務設定與信任根，
  不是帳號憑證或私人金鑰。
- 範例照片中的時刻、日期、溫度及濕度是照片畫面的一部分，已由提供者授權公開。

## 檢查方法與界線

檢查將 Git 暫存區中的檔案匯出至乾淨目錄，再執行秘密資料掃描、私人路徑與裝置
識別資訊比對、照片 metadata 檢查，以及原始碼人工檢視。發布時另外掃描新提交的
Git 歷史。未採用會忽略疑似秘密資料的自訂 Gitleaks allowlist。

編譯與測試產物保留在忽略的本機目錄；本次公開的是原始碼，沒有附上可能嵌入
本機建置路徑的韌體二進位。舊私有歷史中找到過本機路徑／裝置識別資訊，因此
本 repo 採用乾淨的新歷史。

這是對本次發布內容的檢查結果，不是對未來修改的保證。若之後加入自己的座標、
日誌、畫面、設定檔或二進位，發布前應再次檢查；尤其不要提交含 NVS 的完整 Flash 備份。
