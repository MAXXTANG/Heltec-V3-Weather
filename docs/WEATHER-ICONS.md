# 天氣圖示 / Weather icons

![Native OLED weather icons](images/weather-icons.png)

每個圖示使用 44 × 38 px 單色畫布，依實際可見像素置中，左右與上下留白差最多 1 px。
這是韌體使用的圖形，不是照片或另外繪製的概念稿。標籤不會出現在 OLED 上。

| 圖示 | 天氣代碼與日夜條件 |
| --- | --- |
| 晴天 / Clear | 0，白天 |
| 晴夜 / Clear night | 0，夜晚 |
| 晴時多雲 / Partly cloudy | 1、2，白天 |
| 夜間多雲 / Cloudy night | 1、2，夜晚 |
| 陰天 / Overcast | 3 |
| 霧 / Fog | 45、48 |
| 細雨 / Drizzle | 51、53、55、56、57 |
| 下雨 / Rain | 61、63、65、66、67 |
| 陣雨 / Showers | 80、81、82 |
| 雪 / Snow | 71、73、75、77、85、86 |
| 雷雨 / Thunderstorm | 95、96、99 |

`weather_code` 決定圖示，`is_day` 決定晴天／多雲圖示的日夜版本。
每次取得新資料後更新畫面；無有效資料時顯示問號。
來源：[Open-Meteo API 文件](https://open-meteo.com/en/docs)。

修改 `tools/generate_weather_icons.py` 後執行產生器，再跑 `sh tests/run.sh`，
即可同步更新韌體點陣、文件圖例及邊界檢查。圖形原創程式採 MIT 授權。
