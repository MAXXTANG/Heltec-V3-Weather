# 宜蘭災情版 v1.4.1 刷機與還原

適用 **Heltec WiFi LoRa 32 V3、ESP32-S3、8 MB Flash**，不適用 Wireless Paper、V2 或其他板型。Release 標籤：`v1.4.1-yilan`；韌體內部版本：`1.4.1-usb-demo`。解壓縮 Release 的 firmware ZIP 後，在該目錄開啟終端機。

## 選擇還原方式

| 用途 | 檔案與位置 | Wi-Fi 設定 |
| --- | --- | --- |
| 已使用本專案分割表，重刷災情版 | `app.bin` → `0x10000` | 保留，但須先通過下方 preflight |
| 空白板／其他韌體改裝為災情版 | `clean-install.bin` → `0x0`，先完整備份再清空 | 清除，需重新配網 |
| 回到某台裝置先前的完整狀態 | 自己保存的 8 MB 完整備份 → `0x0` | 還原到備份當時 |

公開套件不包含任何人的 Flash／NVS 備份。`clean-install.bin` 是由建置產物組裝的通用安裝映像，不是原裝機備份，也不是 8 MB 的完整讀回檔。

## 1. 準備工具與連線

安裝 Python 3.10+，建立虛擬環境後安裝固定版本工具：

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install esptool==4.5.1
python -m serial.tools.list_ports
```

Windows 用 `py -m venv .venv`，PowerShell 以 `.venv\Scripts\Activate.ps1` 啟用。以下所有 `PORT` 都需替換成實際連接埠，例如 macOS `/dev/cu.usbserial-0001`、Windows `COM5` 或 Linux `/dev/ttyUSB0`；不要照抄範例埠號。

關閉序列監視器。無法連線時，按住 BOOT，短按 RESET，再放開 BOOT，進入下載模式。使用可傳資料的 USB 線。

```sh
python -m esptool --chip esp32s3 --port PORT --baud 115200 chip_id
python -m esptool --chip esp32s3 --port PORT --baud 115200 flash_id
python verify_package.py
```

確認確實是 ESP32-S3、8 MB。若板型、容量、校驗碼不符，停止。不要使用 `--force` 跳過 Secure Boot／Flash Encryption 或晶片不相容錯誤。

## 2. 先完整備份（每次寫入前）

將 `my-v3-before.bin` 換成未使用過的日期檔名，避免覆蓋舊備份：

```sh
python -m esptool --chip esp32s3 --port PORT --baud 115200 read_flash 0x0 0x800000 my-v3-before.bin
python -m esptool --chip esp32s3 --port PORT --baud 115200 verify_flash 0x0 my-v3-before.bin
python -c "import hashlib,pathlib; p=pathlib.Path('my-v3-before.bin'); assert p.stat().st_size==8388608; print(hashlib.sha256(p.read_bytes()).hexdigest())"
```

另存 SHA-256 與板型／裝置識別資料。完整備份含 Wi-Fi 等私人設定，應離線保存，不上傳公開 repo。若原裝置已損壞或 Flash 已清空，此時的備份不能取代以前保存的完整備份。

## 3A. 保留設定，只重刷程式（原有災情版優先使用）

先檢查備份中的分割表與啟動槽。此保守檢查只接受預設 OTA metadata 或全空白的 OTA metadata；其他配置需人工檢查，不會自動重設：

```sh
python preflight.py my-v3-before.bin
python -m esptool --chip esp32s3 --port PORT --baud 115200 write_flash 0x10000 app.bin
python -m esptool --chip esp32s3 --port PORT --baud 115200 verify_flash 0x10000 app.bin
```

只有 preflight 成功才執行後兩行。不要執行 erase_flash，也不要在這個流程寫入 clean-install.bin 或 boot_app0.bin。寫入只涵蓋 app0，保留 NVS、開機程式、分割表與 OTA metadata。驗證成功後按 RESET；如仍在下載模式，放開 BOOT 再重啟。

## 3B. 空白板或完全重裝（會清除全部設定）

先確認步驟 2 的完整備份已保存並校驗。下面的 erase_flash 會清除所有分割區，包括 Wi-Fi、另一個應用槽與儲存資料。

```sh
python -m esptool --chip esp32s3 --port PORT --baud 115200 erase_flash
python -m esptool --chip esp32s3 --port PORT --baud 115200 write_flash 0x0 clean-install.bin
python -m esptool --chip esp32s3 --port PORT --baud 115200 verify_flash 0x0 clean-install.bin
```

映像組成：bootloader `0x0`、partitions `0x8000`、預設 boot_app0／OTA metadata `0xe000`、app `0x10000`；中間空隙為 `0xff`。app0 容量 `0x330000`。無需另刷四個單檔。

重啟後，手機連上 OLED 顯示的 Heltec-V3 Wi-Fi，使用畫面上的密碼，開啟 `http://192.168.4.1` 輸入家中 2.4 GHz Wi-Fi。長按 BOOT 約 3 秒可重新配網。

## 3C. 還原以前的整台裝置狀態

使用同一台裝置、相同 8 MB 容量的完整備份，先核對以前保存的 SHA-256。不要把 app.bin 當成完整備份。此操作會覆寫目前所有設定，Wi-Fi 會回到備份時狀態：

```sh
python -m esptool --chip esp32s3 --port PORT --baud 115200 write_flash 0x0 my-v3-before.bin
python -m esptool --chip esp32s3 --port PORT --baud 115200 verify_flash 0x0 my-v3-before.bin
```

完整讀回檔不包含晶片 eFuse；加密或 Secure Boot 裝置不能套用本指南直接跨裝置還原。

## 4. 螢幕驗收與操作

確認開機可見時鐘，天氣與災情能更新。短按 BOOT：時鐘 → 天氣 → 災情 1/N → 2/N → … → 時鐘。三連按啟動倒數；長按約 3 秒重新配網。

首次成功取得災情只建立基準；之後的新案自動閃爍 3 秒並停留，短按 BOOT 確認後回時鐘。資料約每 60 秒輪詢，網路及倒數可能延長間隔。USB 單次模擬工具在原始碼的 tools/demo_new_case.py；模擬只由明確指令觸發，畫面標示非真實災情。

實測 app 已完成寫入校驗、官方資料讀取、模擬新案停留與使用者親按 BOOT 回時鐘。clean-install.bin 只完成離線結構與組件校驗，本次未清空裝置實測；真實官方新案到通知、相機閃爍量測與續航仍未測。寫入成功不代表 OLED／Wi-Fi 必定驗收通過。

命令依據：[Espressif esptool v4 官方說明](https://docs.espressif.com/projects/esptool/en/release-v4/esp32s3/esptool/basic-commands.html)。
