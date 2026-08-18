# GSSquared WebAssembly (Web) 開發與架構技術手冊 (DevLog.md)

本文件完整記錄 GSSquared WebAssembly (WASM) 版本的架構設計、開發工作流程、CI/CD 自動化部署、以及關鍵除錯與根本原因分析（Root Cause Analysis），供開發者與後續 AI Agent 參考遵循。

---

## 1. 核心架構與技術棧

### 1.1 WebAssembly & C++ 核心
* **編譯工具鏈**：Emscripten SDK (emsdk)。
* **多執行緒與並行**：`-pthread` 與 `-sPTHREAD_POOL_SIZE=8`，依賴瀏覽器 `SharedArrayBuffer`。
* **指令集優化**：支援 `GS2_WASM_SIMD` CMake 選項（`-msimd128`）。
* **圖形與視窗**：SDL3 WebGL 後端，搭配 `SDL_LOGICAL_PRESENTATION_LETTERBOX` 進行固定比例（1288x928 設計解析度）自適應縮放。
* **字型與選單**：`SDL_ttf` 搭配 Dear ImGui。
* **資料持久化**：使用 Emscripten `IDBFS` (IndexedDB File System)，掛載於 `/persistent/`（供 NVRAM/PRAM 與存檔長久留存）。

### 1.2 Web 前端 (Shell)
* **主頁面**：[`assets/web/shell.html`](file:///c:/dev/gssquared/assets/web/shell.html)。
* **設計風格**：Modern Dark Glassmorphism（深色毛玻璃質感、CSS 變數、自適應響應式佈局）。
* **UI 元件**：
  * 頂部功能列：`GSSquared Web` 品牌標題、磁碟讀寫 LED 狀態指示燈、Drives / Hotkeys / Mute / Fullscreen 控制按鈕、GitHub Repo 捷徑。
  * 磁碟掛載視窗（Drives Modal）：支援 `Slot 6 Drive 1/2` 與 `Slot 7 HD1` 檔案選取與拖放（`.woz`, `.2mg`, `.dsk`, `.po`, `.hdv`）。
  * 快捷鍵說明視窗（Hotkeys Modal）：Apple II / IIgs 專用按鍵對應（Control-Reset、Open/Closed Apple、F8-F12）。
  * 啟動遮罩（Start Overlay）：倒數 5 秒自動啟動或點擊手動啟動（滿足瀏覽器 WebAudio 自動播放策略）。
* **PWA 與離線支援**：`manifest.json` 與 `sw.js`。

---

## 2. 本地開發與編譯流程

專案根目錄已提供一鍵批次檔：

### 2.1 一鍵編譯 Web 版
```cmd
buildweb.bat
```
* 自動調用本地 Emscripten 環境變數 (`C:\dev\MameWasm\emsdk\emsdk_env.bat`)。
* 執行 `emcmake cmake -B build-web -DGS2_WASM_SIMD=ON`。
* 執行 `cmake --build build-web --target GSSquared -j %NUMBER_OF_PROCESSORS%`。

### 2.2 一鍵啟動本地測試伺服器
```cmd
runweb.bat
```
* 執行 `python assets/web/serve.py 8000 build-web`。
* `serve.py` 會自動注入 `Cross-Origin-Opener-Policy: same-origin` 與 `Cross-Origin-Embedder-Policy: require-corp` HTTP Headers，確保 `SharedArrayBuffer` 正常運作。
* 瀏覽器開啟：`http://localhost:8000/GSSquared.html` 或 `http://localhost:8000/`。

---

## 3. GitHub Actions CI/CD 與 GitHub Pages 部署

* **Workflow 檔案**：[`.github/workflows/deploy.yml`](file:///c:/dev/gssquared/.github/workflows/deploy.yml)
* **觸發條件**：推送到 `main` 分支時自動執行。
* **部署站點**：`https://anomixer.github.io/gssquared/`
* **建置步驟**：
  1. `actions/checkout@v4` (包含 submodules)。
  2. `actions/cache@v4` 快取 Emscripten SDK。
  3. 直接透過 `git clone --depth 1 https://github.com/emscripten-core/emsdk.git` 安裝與啟用 `3.1.73`（避開第三方 Action 的 HTTP Zip 503 與 Node 20 棄用問題）。
  4. `sudo apt-get install ninja-build` + `emcmake cmake -B build-web -DGS2_WASM_SIMD=ON`。
  5. `cmake --build build-web --target GSSquared`。
  6. 部署產物至 GitHub Pages。

---

## 4. 關鍵問題排查與根本原因分析 (Root Cause Post-Mortems)

在 Web 移植與維護過程中，我們排查並徹底解決了數個深層次技術問題：

### 4.1 GitHub Pages 卡在 `Loading assets... (67/68)`
* **現象**：本地 `serve.py` 正常，但部署至 GitHub Pages 後資源載入卡死在最後一步。
* **根本原因**：
  * GSSquared 使用了 Emscripten `-pthread` 多執行緒，需要 `SharedArrayBuffer`。
  * 瀏覽器安全性規定：唯有伺服器回傳 `COOP` (Cross-Origin-Opener-Policy) 與 `COEP` (Cross-Origin-Embedder-Policy) Headers 時才能開啟 `SharedArrayBuffer`。
  * GitHub Pages 是純靜態空間，無法自訂 HTTP 回傳標頭。
* **解決方案**：
  * 引入 [`assets/web/coi-serviceworker.js`](file:///c:/dev/gssquared/assets/web/coi-serviceworker.js)。
  * 在 `shell.html` `<head>` 最前端註冊 Service Worker，在瀏覽器端動態攔截並為所有靜態資源注入 COOP/COEP Headers。

---

### 4.2 WebAudio 15~20 秒音訊延遲
* **現象**：模擬器開機或恢復分頁時，音效嚴重滯後十幾秒。
* **根本原因**：
  * 當瀏覽器因未進行使用者手勢互動而暫停（Suspend）WebAudio 時，C++ 端音訊產生器（`soundglu.cpp`, `mb.cpp`, `mb2.cpp`）仍持續往 SDL Audio Stream 填充緩衝區。
  * 當使用者點擊畫面恢復音訊時，SDL 必須先播放完積壓的龐大緩衝區。
* **解決方案**：
  * 在音訊處理器中監控隊列佇列長度，若超出安全臨界值（代表音訊曾被暫停），自動清空（Purge）積壓的陳舊音訊封包，確保即時低延遲播放。

---

### 4.3 初始載入與 High-DPI 螢幕 1/4 畫面裁切
* **現象**：開啟網頁時，畫面只有左上角 1/4 顯示，必須全螢幕切換一次才正常；或者點擊選單返回時畫面被裁切。
* **根本原因**：
  1. `<canvas>` 標籤若硬編碼了 `width="1288" height="928"`，在高 DPI（如 Windows 縮放 125%/150%/200% 或 Retina）下，HTML 屬性尺寸與 CSS 物理像素（`clientWidth * devicePixelRatio`）發生衝突。
  2. 若在 JS 中手動指派 `canvas.width = targetW; canvas.height = targetH;`，WebGL 規範會直接銷毀並重建 WebGL Context（觸發 `webglcontextlost`），造成畫面無限重新載入的死循環。
  3. `SelectSystem::ensure_logical_presentation()` 必須正確更新 `SDL_SetRenderLogicalPresentation(..., LETTERBOX)`。
  4. **（2026-08-14 追加）** 在 WASM 環境下，canvas 在初始化時的實際尺寸可能與 SDL 的邏輯呈現（logical presentation）設定不同步，導致 `LETTERBOX` 模式的計算基於錯誤的 canvas 尺寸，造成畫面裁切。
* **解決方案**：
  * 移除 `<canvas>` 標籤上硬編碼的 `width` 與 `height` 屬性，交由 SDL3 與 Emscripten 根據 CSS 容器自適應初始化。
  * 嚴禁在 JS 執行期間強制修改 `canvas.width` / `canvas.height`，僅透過 `ResizeObserver` 發送標準 `window.dispatchEvent(new Event('resize'))`。
  * 在 `SelectSystem.cpp` 與 `EditSystem.cpp` 中以 `ensure_logical_presentation()` 確保 Letterbox 縮放隨時與當前 Render Backbuffer 同步。
  * **（2026-08-14 追加）** 修改 `videosystem.cpp` 的 window 創建邏輯：在 WASM 環境下，`SDL_CreateWindow` 不指定寬高（傳入 0, 0），讓 canvas 自適應 HTML 容器尺寸，並跳過 `SDL_SetWindowMinimumSize` 和 `SDL_SetWindowAspectRatio` 的設定，避免強制調整 canvas 尺寸造成與容器衝突。原生平台維持原有行為（指定 1288x928 並設定尺寸限制）。
  * **（2026-08-14 追加）** 在 `SelectSystem::ensure_logical_presentation()` 和 `EditSystem::ensure_logical_presentation()` 函數中加入 WASM 專用邏輯：每次設置 logical presentation 前，先讀取 renderer 的實際輸出尺寸並調用 `calculate_target_rect()` 更新內部狀態（透過 `vs->update_target_from_output()`）。此函數在 `event()` 和 `render()` 中都會被調用，確保在任何狀態（初始啟動、overlay 顯示中、關閉 emulation 後）下都能正確同步 canvas 尺寸與 SDL 邏輯呈現。
  * **（2026-08-14 追加）** 在 `SelectSystem` 和 `EditSystem` 建構子中，設置 logical presentation 前先執行一次 canvas 尺寸同步（透過上述 `ensure_logical_presentation()` 的邏輯）。
  * **（2026-08-14 追加）** 修改 `shell.html` 的 `Module.setStatus`：移除 runtime ready 時的 resize 觸發，避免在 overlay 還顯示時以錯誤尺寸渲染背景的 select 畫面。
  * **（2026-08-14 追加）** 在 `shell.html` 的 `startEmulator()` 函數中，overlay 消失後立即觸發多次 resize 事件（0ms, 16ms, 100ms, 200ms），確保 SDL 和 canvas 完全同步後才開始渲染 select 畫面。
  * **（2026-08-14 Late 追加 - Debug Session）** 問題仍持續發生。新增了詳細的 debug logging 到 `SelectSystem.cpp`、`EditSystem.cpp` 和 `videosystem.cpp`，追蹤 canvas 尺寸在各個關鍵時間點的讀取情況：
    - `SelectSystem` 建構子：在 `update_target_from_output()` 前後記錄 canvas 尺寸和 target rect。
    - `SelectSystem::ensure_logical_presentation()`：記錄每次調用時的 canvas 尺寸變化。
    - `videosystem_t::update_target_from_output()`：記錄 `SDL_GetCurrentRenderOutputSize()` 返回的實際像素尺寸。
    - 目的：確認 canvas 尺寸在初始化時是否為 0x0 或其他錯誤值，以及 resize 事件是否正確更新 canvas 尺寸。
    - 測試方法：開啟瀏覽器 DevTools console (F12)，觀察初始載入、overlay 消失、關閉 emulation 時的 console 輸出。
    - 下一步：根據 debug log 結果判斷是否需要延遲 logical presentation 設定、改變 resize 事件觸發時機，或者 SDL 的 canvas 尺寸查詢方式。
* **技術細節**：
  * **根本修復**：在 WASM 環境下，`SDL_CreateWindow` 不再強制指定視窗尺寸（傳入 0, 0 讓 SDL 查詢 canvas 容器大小），並跳過 `SDL_SetWindowMinimumSize` 和 `SDL_SetWindowAspectRatio` 設定，完全由 HTML/CSS 控制 canvas 佈局。這消除了 C++ 端強制設定與瀏覽器容器尺寸的衝突。
  * **每次渲染前同步**：`SelectSystem::ensure_logical_presentation()` 和 `EditSystem::ensure_logical_presentation()` 在每次 `event()` 和 `render()` 調用時，都會先讀取 renderer 的實際輸出尺寸（`SDL_GetCurrentRenderOutputSize`）並更新 `video_system_t` 的 target rect（`calculate_target_rect`），然後才設置 `SDL_SetRenderLogicalPresentation(..., LETTERBOX)`。
  * 此方法確保無論 canvas 何時改變尺寸（高 DPI 環境、瀏覽器 resize、fullscreen 切換、狀態轉換），SDL 的邏輯呈現都能基於正確的 canvas 尺寸計算，避免畫面被裁切。
  * **JS 端配合**：延遲 resize 觸發至 overlay 消失後，避免在錯誤的尺寸下進行首次渲染；多次 resize（16ms, 100ms, 200ms 間隔）確保瀏覽器佈局穩定後 SDL 完全同步。
  * 條件編譯 `#ifdef __EMSCRIPTEN__` / `#ifndef __EMSCRIPTEN__` 確保這些修復僅影響 WASM 版本，原生 C++ 版本維持固定視窗尺寸和長寬比限制的原有行為。
  * **Debug Logging (2026-08-14 Late)**：新增 `printf` debug 輸出追蹤 canvas 尺寸檢測和同步時機，協助診斷裁切問題的真正根源。Log 輸出格式：`[SelectSystem]` / `[SelectSystem::ensure]` / `[videosystem]` 前綴，顯示 canvas 像素尺寸、target rect 位置與大小。測試時需開啟瀏覽器 console 觀察輸出。

---

### 4.4 滑鼠游標與機器卡片 Highlight 偏移、選單文字破字
* **現象**：滑鼠懸停於機器卡片時，高亮框位置與滑鼠位置不一致；頂部選單文字鋸齒模糊或破字。
* **根本原因**：
  * 在 `videosystem.cpp` 渲染迴圈中呼叫 `SDL_SetRenderViewport(renderer, NULL)` 破壞了 SDL3 `SDL_ttf` 引擎的文字座標矩陣。
  * 在 `SelectSystem::render()` 內層重複調用 `SDL_SetRenderLogicalPresentation`，但 `SelectSystem::event()`（滑鼠座標轉換 `SDL_ConvertEventToRenderCoordinates`）執行於 `render()` 之前，兩者矩陣版本不同步。
* **解決方案**：
  * 移除 `videosystem.cpp` 中對 Viewport 的多餘覆寫。
  * 將 Presentation 矩陣的更新集中在 `WINDOW_RESIZED` / `PIXEL_SIZE_CHANGED` 事件以及 `ensure_logical_presentation()` 中。

---

### 4.5 `File -> Close Emulation` / `Quit` 拋出 JS Console Exception
* **現象**：在模擬過程中點擊選單 `File -> Close Emulation` 或 `Quit`，左下角直接彈出 `Exception thrown, see JS console`。
* **根本原因**：
  * `PHASE_EMULATION` 中對 `SDL_EVENT_QUIT` 的處理曾被註解，導致 CPU 無法停機。
  * 在 Emscripten 主迴圈中直接回傳 `SDL_APP_SUCCESS` 會使 SDL 呼叫 `exit(0)`，在 WASM 中會被包裝為 C++ Exception 拋至瀏覽器控制台。
* **解決方案**：
  * 在 `src/gs2.cpp` 的 `SDL_AppEvent` 中恢復 `SDL_EVENT_QUIT` 處理：`cpu->halt = HLT_USER;`。
  * 在 `SDL_AppIterate` 裡，針對 `__EMSCRIPTEN__` 環境攔截結束事件，改為直接執行 `transition_to_shutdown(state)` 平滑返回主選單 `PHASE_SYSTEM_SELECT`，避免 Process 退出拋出異常。

### 4.6 WASM 啟動 Overlay 二次重繪與手動提前啟動

* **現象**：WASM 載入期間 overlay 背景會出現第二次 shell 重繪；手動按 `Start Emulator` 提前進入時，canvas 尚未完成 layout 同步，曾造成 selector 裁切，必須調整瀏覽器視窗後才恢復。
* **原因**：
  * `Module.setStatus('')` 在 runtime ready 時修改 overlay 內容（隱藏 spinner、替換提示文字），造成 overlay layout 變更與額外重繪。
  * 手動啟動會在 SDL/Emscripten canvas 尺寸穩定前進入 selector。
  * `runweb.bat` 服務 `build-web/GSSquared.html`；Emscripten shell 是 link-time 輸入，若只修改 `assets/web/shell.html` 而沒有重新 link，產物會保留舊 UI。
* **解決方案**：
  * 移除 `Start Emulator` 按鈕與 overlay click-to-start，只保留 runtime ready 後五秒自動啟動。
  * runtime ready 時維持 overlay DOM 與視覺狀態不變，避免第二次 shell redraw；overlay 消失後才派送 0ms、16ms、100ms、200ms resize 事件。
  * `transition_to_shutdown()` 重建 renderer/selector 後，透過 Emscripten `EM_ASM` 再次派送相同的 resize 序列，修復 Close Emulation 後的 canvas 同步時序。
  * `CMakeLists.txt` 追蹤 `assets/web/shell.html`，`buildweb.bat` 每次重新 configure、clean 並 relink `GSSquared.html`，避免 `build-web` 產物殘留舊 shell。

### 4.7 WASM `Machine -> Pause / Resume` freeze

* **現象**：WASM 執行 `Machine -> Pause / Resume` 後整個瀏覽器畫面 freeze。
* **根本原因**：`run_one_frame()` 在 `EXEC_PAUSED` 直接返回，沒有呼叫 `SDL_RenderPresent()`；Emscripten 的 `requestAnimationFrame`/vsync loop 因此沒有下一個 frame，Resume 事件也無法正常更新畫面。
* **解決方案**：paused 狀態停止 CPU/device emulation，但 WASM 仍呼叫 `frame_video_update()` 呈現不變的 frame，讓 menu 與 Resume 持續可用；native 版本則使用 16ms delay 避免 busy loop。

### 4.8 WASM `File -> Drives` mount 失敗

* **現象**：原生 C++ 的 `File -> Drives` 正常，但 WASM 點擊 `S7D1`、`S6D1`、`S5D2` 等 drive 後顯示 `Failed to mount media`；OSD 左側 drive drawer 正常。
* **根本原因**：`storage_key_t` 是 64-bit，slot 位於高 16 bits。`MenuInterface::diskToggle()` 將完整 key 直接轉成 `SDL_Event.user.data1` pointer；wasm32 pointer 會截斷高位，造成 menu event 送出錯誤 drive key。左側 drawer 直接傳 `storage_key_t`，所以不受影響。
* **解決方案**：menu event 改以 32-bit `slot/drive` packing 傳遞，接收端再還原 `storage_key_t`，避免依賴 pointer 保存 64-bit 整數。

### 4.9 GitHub Pages refresh 卡在 `Loading assets... (67/68)`

### 4.10 GitHub Pages menu alignment and official GS2 branding

* **WASM mouse capture 修正**：`Machine -> Capture Mouse` 不再讓上方 File/Edit/Machine menu 消失；`F1` 是唯一的 Capture / Release toggle，host 不攔截 `Esc`，確保 `Ctrl + Open Apple (Alt) + Esc` 能送進 Apple IIgs built-in control panel。由於瀏覽器要求 Pointer Lock 必須來自 user gesture，若 menu command 的第一次 request 被拒絕，shell 會在下一次點擊 emulator canvas 時自動重試，並顯示 F1 release 提示。

* **現象**：GitHub Pages 上 selector/editor 的滑鼠游標與選取物件位置偏移，頂部選單後段文字（例如 Settings、Display、Docs）出現裁切；Web shell 與 favicon 使用的圖示也不是官方 GS2 正方形 mark。
* **根本原因**：selector/editor 使用固定 `1288x928` 的 SDL `LETTERBOX` logical presentation，但 Dear ImGui menu overlay 使用 SDL window point coordinates，兩者套用了不同的座標轉換矩陣。
* **解決方案**：新增 `render_menu_overlay_over_logical_ui()`，繪製選單前暫時停用 logical presentation，使用實際 window output coordinates render menu，再恢復 selector/editor 的 `1288x928 LETTERBOX` 設定，讓滑鼠 hit-test、文字與畫面使用同一個 transform。
* **官方圖示**：加入 `assets/img/gssquared-mark.png`（1024x1024），Web header、startup overlay、favicon 與 PWA manifest 統一改用此正方形 GS2 mark；CMake Web POST_BUILD 也會將它複製到 `build-web/img/`。
* **子選單文字裁切修正**：保留目前正確的 window-point 滑鼠輸入座標，menu render 暫停 logical presentation 時，依 `SDL_GetCurrentRenderOutputSize()` 與 SDL window size 計算並套用 renderer scale，讓 ImGui 文字頂點正確映射到瀏覽器 pixel output，避免主選單右側及子選單文字被裁切。

* **現象**：本地 `runweb.bat` 正常，但 GitHub Pages refresh 後卡在最後一個 Emscripten preload asset。
* **根本原因**：頁面同時註冊 COI service worker 與 PWA cache service worker；後者可能在 deployment/refresh 期間混用舊版 `.wasm` 與 `.data`。
* **解決方案**：保留 `coi-serviceworker.js` 提供 COOP/COEP，停止註冊 `sw.js` 快取 emulator assets，並在 shell 中自動解除既有 `sw.js` registration 後 reload 一次。

---

### 4.11 WASM Close Emulation canvas listener 與 resize menu clipping

* **Close Emulation 例外**：回到 Select Machine 時，舊 SDL canvas 的 Emscripten pointer listener 可能在 `SDL_WindowData` 釋放後仍被觸發，造成 `querySelector('')`、`querySelector('0')` 以及 `memory access out of bounds`。
* **修正**：在 `transition_to_shutdown()` 拆除舊 SDL window 前，先由 web shell 替換 `#canvas` DOM node，讓舊 listener 隨舊 node 一起移除；新 SDL window 綁定新的 canvas。WASM 頁面內切換不呼叫 `SDL_Quit()`，並在 Close Emulation 前解除 mouse capture。
* **Resize menu clipping**：canvas resize 後，selector/editor 的 ImGui menu 背景仍在但文字被黑色邊界覆蓋。原因是 menu 使用 window-sized `STRETCH` 後，在 `SDL_RenderPresent()` 前立即切回 `LETTERBOX`，SDL 的 letterbox border 可能覆蓋 menu。
* **最終修正**：menu overlay 以實際 window size 的 `STRETCH` presentation render，先 `vs->present()`，再恢復 `1288x928 LETTERBOX` 供下一個 selector/editor frame 使用。Resize event 同時 invalidate SelectSystem/EditSystem，確保 canvas 清除後整個 UI 會重畫。
* **驗證**：WASM Ninja build 成功；在 `1189x856`、接近 `1288/928` aspect ratio 的 browser canvas 上反覆 resize，File/Edit/Machine 等 menu 文字保持正常顯示。

### 4.12 Web shell header 狀態資訊整理

* 移除 web shell header 中沒有實際作用的 `S6D1`、`S6D2`、`HD1` LED 指示，以及無法正確掛載 media 的 `Drives` 按鈕/操作區。
* 將 `Ready`、`Mounted ...` 等 `status-toast` 訊息移到 header 正中央，保留長檔名的省略顯示，避免狀態訊息佔用 canvas 左下角。

### 4.13 Second Sight Text 畫面比例修正

* **現象**：啟用 `Display -> Second Sight Text` 後，Apple IIgs 文字畫面被拉伸到整個 `1288x928` logical target，導致上方的 `Apple IIgs` 標題接近甚至超出畫面邊界。
* **修正**：Second Sight 的 `720x400` VGA text texture 現在維持原始寬高比，依目前 target 寬度計算高度並垂直置中，再交給 renderer 顯示，避免文字畫面被非等比例放大或裁切。

### 4.14 Web Hotkeys 與本地開發更新

* **Hotkeys 面板**：依實際快捷鍵順序更新 Apple II Hotkeys 顯示，並加入 `(IIgs) Enter Control Panel` 按鈕。
* **IIgs Control Panel**：按鈕透過 WASM bridge 排入 `Ctrl + Left Alt + Esc` 的 SDL/ADB 按鍵序列，避開瀏覽器 `Ctrl + Esc` 開啟 Windows Start menu 的衝突。
### 4.15 GitHub Actions Emscripten Setup 503 錯誤與 Node 20 棄用修復

* **現象**：GitHub Actions CI/CD 在執行 `mymindstorm/setup-emsdk@v14` 時報錯 `HTTP Error 503: Service Unavailable`（下載 `https://github.com/emscripten-core/emsdk/archive/HEAD.zip` 失敗），並伴隨 Node 20 runner 棄用強制升級 Node 24 的相容性警告。
* **根本原因**：第三方 action 透過 HTTP 下載 zip archive 容易受到 GitHub archive 服務限流或 503 影響，且舊版 action 依賴已棄用的 Node.js 20 執行環境。
* **解決方案**：在 `.github/workflows/deploy.yml` 中改用原生 `git clone --depth 1 https://github.com/emscripten-core/emsdk.git` 搭配 `actions/cache@v4` 快取，直接安裝並啟用 `3.1.73`，完全擺脫第三方 Action 依賴，並大幅加速後續 Actions 建置時間。

### 4.16 Web Select/Edit 滑鼠座標與 Second Sight Text 比例修正

* **Select / Edit 畫面滑鼠不對齊，且不顯示 tool tip。** 根因：原本 `SelectSystem::event()` 使用 `SDL_ConvertEventToRenderCoordinates()`，在 Emscripten canvas 上對 DPR/內部 `dst` 矩形的映射有誤，導致設計座標被「壓縮」——上方機器卡片可 hover，但下方 `+ New` / `Launch` / `Edit` 按鈕（設計 Y 較大）永遠對不到，連帶 `SystemButton` 的機器說明 tool tip 與底部按鈕 hint 都不顯示。
* **修正（與先前錯誤方向不同）**：最初嘗試用 `vs->target`（renderer output pixels）做反向 Letterbox，但那等價於 `SDL_ConvertEventToRenderCoordinates` 本身，因此毫無效果。正確做法是**在 window points 空間**自行反轉 LETTERBOX：直接用 `SDL_GetWindowSize()` 取得視窗點數（滑鼠事件本就在 points 空間），把 1288×928 設計空間依視窗長寬比 letterbox 進去，再 `design = (point − letterbox.xy) * (design / letterbox.wh)`。此映射與 DPR 無關、且與 SDL 的 logical presentation 完全互逆。`SelectSystem::event()` 與 `EditSystem::event()` 現在都改用此方式（原生平台仍走 `SDL_ConvertEventToRenderCoordinates`）。
* **驗證狀態（2026-08-18）**：已確認 `＋ New` / `Launch` / `Edit` 三個底部按鈕 hover 會 highlight —— 代表設計空間下半部（較大設計 Y）的反轉映射已正確。機器卡片 hover tool tip、與 Second Sight Text 比例是否完全正常仍待進一步確認。
* **Second Sight Text 文字被撐開（比例失真）。** 根因：`secondsight.hpp` 的 `render_vga_text_frame()` 雖有 720×400 等比例 letterbox 修正，但只作用在 `a2_overlay`（Apple II 文字同步）路徑；而真正的 Second Sight VGA 文字/圖形模式走 `vga_render_8/16/24bpp()`，這些函式內部以 `dstadj = nullptr` 呼叫 `render_frame()`，把 720×400 直接**拉伸**填入整個 1288×928 target。
* **修正（方向修正：非 letterbox）**：使用者確認開啟 Second Sight Text 時應**即時切換顯示解析度**到文字原生比例（720×400），而非把文字 letterbox 進 1288×928。實作：
  - `video_system_t` 新增 `forced_target_aspect`（>0 時覆寫預設 target 長寬比）與 `set_target_aspect()`；`calculate_target_rect()` 在有覆寫時改用該比例，使 `vs->target` 對齊新解析度。
  - `MenuInterface::toggleSsTextMode()` 切換 SS 文字模式時呼叫 `syncSsTextCanvasAspect()`：WASM 下用 `EM_ASM` 把 canvas CSS `aspect-ratio` 攪成 `720 / 400`（關閉時還原 `1288 / 928`）並設定 `vs->target_aspect`；`shell.html` 的 ResizeObserver 轉成 window resize 讓 SDL 重新測量 canvas。
  - **`transition_to_emulation()` 進入模擬時若 SS 文字已啟用（例如來自設定檔），也會呼叫 `getMenuInterface()->syncSsTextCanvasAspect()`，避免必須先關再開 SS 才會生效。**
  - `transition_to_shutdown()`（回到選單）時把 canvas 還原為 `1288 / 928`，避免選單沿用 720×400。
  - `vga_render_8/16/24bpp()` 仍保留 `dstadj` 參數作為安全網，但 canvas 已切到 720×400、`vs->target` 即滿版時不會產生黑邊。
  - **（2026-08-18 修正）** canvas aspect 切換改為「寬度驅動」模型：`width: min(100vw, 1288px); height: auto; aspect-ratio: 720/400`，關閉時還原 `aspect-ratio: 1288/928; width/height: ''`。這樣一來：(1) 720:400 Aspect 永遠與 canvas 同步，高度由寬度與 Aspect 推導，不會因 `height:100%` + `max-width:100%` 在窄螢幕產生扭曲；(2) 瀏覽器視窗大於設計寬 (1288px) 時，canvas 寬度上限鎖在 1288px，Second Sight 文字即為 1:1 並不會「撑开」 balloon 到整個大視窗 —— 其餘viewport 顯示 letterbox 條細。`syncSsTextCanvasAspect()` 會在切換後連續派送 `resize` (0ms / 50ms) 確保 SDL3 重新量測 canvas。
  * **驗證狀態（2026-08-18）**：底部按鈕 hover 已確認；Second Sight Text canvas 切換改為寬度驅動並鎖定最大 1288px，防止大視窗 balloon 的行為待另台機器實測。

### 4.17 Second Sight 最終顯示模型與 menu bar inset

* **需求釐清**：SS 模式不是把 browser canvas 改成 `720x400`；WASM 應和 exe 一樣維持最大 `1288x928` 顯示 surface。`720x400` 是 Second Sight VGA 內容的原生解析度，只應在這個 surface 內等比例放大。
* **修正過程**：排除了固定寬度但保留 `height:100%` 造成的垂直拉伸、以 `devicePixelRatio` 除 CSS layout 尺寸造成 canvas 變瘦，以及 browser 大視窗超過 1288 後持續 balloon 的問題。最終由 Web shell 將 SS canvas 以 `1288x928` logical 尺寸限制，視窗不足時才整體等比例縮小。
* **Renderer 修正**：Second Sight text renderer 改用明確 destination rectangle，將 canvas 內 File/Edit/Machine menu bar 約 26 window points 的高度扣除，SS 內容從 menu bar 下方開始 render；剩餘區域再依 `720x400` 等比例計算，避免 `Apple IIgs` 第一列被 menu bar 覆蓋。
* **相關文件重點**：Second Sight mode 03h 是 `80x25`、`9x16` text，原生顯示區為 `720x400`；這是內容 source aspect，不是整個 GSSquared canvas 的 layout aspect。
* **驗證**：WASM Ninja build 成功；localhost 大視窗、SS canvas 上限、menu bar 下方起始位置與關閉 SS 後恢復 selector layout 均納入測試流程。

---

## 5. 後續維護與開發守則 (Rules for Agents & Contributors)

1. **嚴禁修改 `vendored/` 目錄**：不得改動 `vendored/SDL/`、`vendored/SDL_image/`、`vendored/SDL_ttf/` 等第三方庫原始碼。
2. **遵守 WebGL 生命週期**：前端 JS 切勿直接變更執行中 Canvas 的 `width` / `height` 屬性，避免 WebGL Context 丟失。
3. **保持 COOP/COEP 支援**：新增資源或改動 Web 架構時，確保 `coi-serviceworker.js` 正常載入，以維持 `SharedArrayBuffer` 與多執行緒支援。
4. **提交前驗證**：修改 C++ 或 Web 程式碼後，請執行 `buildweb.bat` 並使用 `runweb.bat` 驗證全解析度、縮放、音訊低延遲與 `Close Emulation` 流程。
