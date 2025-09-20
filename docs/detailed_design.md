# 詳細設計書 (AtmosViz)

## 1. クラス構成

### 1.1 `AtmosVizAudioProcessor`
- **役割**: オーディオ解析、メトリクス共有。
- **主要メンバー**
  - `latestMetrics`: `std::array<SpeakerMetrics, 12>`
  - `metricsLock`: `juce::SpinLock`
  - FFT 関連: `juce::dsp::FFT`, `juce::dsp::WindowingFunction`
- **主要メソッド**
  - `processBlock`: RMS/Peak/FFT 計算。
  - `copyLatestMetrics`: GUI ツールからの参照用コピー。
  - `buildSpeakerDefinitions`: Dolby 7.1.4 角度・高さを定義。

### 1.2 `SpeakerVisualizerComponent`
- **役割**: 3D 描画、カメラ制御。
- **主要プロパティ**
  - `CameraPreset enum` / `CameraParameters`
  - `presetColumns`: Inside/Outside それぞれの `CameraParameters` を列キー (Home/User/Front/Back/Left/Right/Top) ごとに保持し、User 列の初期値はいずれもフロントを含む斜めビュー
  - `zoomFactor`, `cameraDistance`, `cameraInside`
  - `speakers`: 可視化対象スピーカー配列
  - `roomVerticesModel`: ルームワイヤーフレームの 8 頂点
  - `VisualizationMode`: 描画モード選択肢と現在値 (`DirectionalLobes` など) を保持。
  - `heatmapPoints`: ヒートマップ描画用にプリ計算した室内グリッド座標。
  - 各 `DisplaySpeaker` に `trail` 履歴を持たせ、Temporal Trails 描画に利用。
- **主要メソッド**
  - `setCameraPreset`: yaw/pitch/distance を反映。
  - `setZoomFactor`: Inside/Outside 用に距離をリマップ。
  - `setVisualizationMode`: プルダウン選択で描画モードを切り替え、UI と再描画を同期。
  - `drawDirectionalLobes` / `drawLayeredLobes` / `drawDirectivityBalloons` / `drawRadiationHeatmap` / `drawTemporalTrails`: 各描画モード専用のレンダリング入口。
  - `mouseDrag`: カメラの yaw/pitch を更新。
  - `mouseWheelMove`: ズーム値を更新。
  - `paint`: ワイヤーフレーム、スピーカー、ローブ、ギズモを描画。
  - 補助関数: `projectPoint`, `applyOrbit`, `drawRoom`, `drawGizmo`。

### 1.3 `AtmosVizAudioProcessorEditor`
- **役割**: プリセットボタンとズームスライダーの管理。
- **主要メソッド**
  - `createCameraButtons`: Outside/Inside ラベル付き 2 行レイアウトで `[Home][User][Front][Back][Left][Right][Top]` ボタンを生成し、列ごとの inside/outside ペアに `onClick` を設定。
  - `setupZoomSlider`: 共通スライダーを初期化し、ラベルを添付。
  - `setupSliderModeSelector`: Zoom / Draw Scale を切り替えるコンボボックスを構成し、選択に応じてスライダー機能を更新。
  - `setupVisualizationSelector`: View Mode プルダウンを構成し、モードとの同期・トリガーを管理。
  - `sliderValueChanged`: ズーム倍率を更新し、Home=100% を基準に Outside/Inside の距離・FOV を再計算。
  - `setCameraPreset`: GUI 側から視点切り替え。

## 2. 描画計算
- `projectPoint`: カメラ yaw/pitch に基づく手書きの投影計算。`cameraDistance` を透視投影距離として使用。
- ローブ描画: `juce::Path` を用いて三角形 + 外周ストロークを描画。
- ワイヤーフレーム: `roomEdges` の各辺を前面/背面に分類して線種を切り替える。

- 可視化モードごとの描画:
  - `Directional Lobes`: 単一ローブで指向性を示し、塗りとストロークで強度差を演出。
  - `Layered Lobes`: 3 層の扇形をスケール・アルファ差で重ね、低〜高域の広がりを視覚的に表現。
  - `Directivity Balloon`: 指向性に沿って楕円シェルを複数段描画し、等レベル面を擬似的に表示。LFE は正円の同心シェル。
  - `Radiation Heatmap`: 事前生成した室内グリッド点に対し、RMS と指向性を掛け合わせた逆二乗減衰でレベルを推定し、半透明粒子を着色。
  - `Temporal Trails`: 各スピーカ位置の履歴をポリラインで描画し、最新方向を細線で強調。

## 3. カメラ制御
- プリセットは列キー (Home/User/Front/Back/Left/Right/Top) と Inside/Outside フラグの組み合わせで管理し、`presetColumns` を通じて `SpeakerVisualizerComponent` に提供。
- Outside プリセット適用時は `cameraInside` を false、Inside プリセット適用時は true に設定し、距離/原点固定の挙動を切り替える。
- Outside 側は距離パラメータをそのまま使用し、Home は部屋全体が収まる固定距離（目安 40 m）にロック。Front/Back/Left/Right は各面の外側から法線方向を向くよう yaw/pitch を設定し、Top は天井の外側から真下を向いて天井面とトップスピーカーを俯瞰する。その他の方位も 10%〜200% の倍率を距離換算に用いる。
- Inside 側は距離を固定し、カメラ位置を常に原点（リスニング位置）に保ったまま yaw/pitch のみ更新して原点オービットを維持し、Home は原点から正面を向く室内ホームビュー、Front/Back/Left/Right は向きを +Z/-Z/-X/+X に設定、Top 時は原点で真上（+Y）を向いて天井面とトップスピーカーを俯瞰する。
- User 列は現在視点を一時保存して呼び戻すスロット（永続化は未実装）で、Inside/Outside それぞれ独立して保持する。Inside 側は原点固定のままユーザー調整した yaw/pitch/ズームを保持し、初期値はいずれもフロントを含む斜めビュー。
- ズーム倍率は Home=100% を基準に 10%〜200% で管理。Outside は `cameraDistance = outsideHomeDistance * (100 / zoomPercent)`、Inside は FOV/スケールを `insideHomeScale * (100 / zoomPercent)` として原点固定のまま近接/遠景を表現。
- `mouseDrag`: yawAnchor / pitchAnchor と `e.position` の差から角度を更新。
## 4. メトリクス共有と描画同期
- Audio スレッド → GUI スレッド間は `copyLatestMetrics` と `metricsLock` でスレッドセーフに共有。
- GUI 側は `timerCallback` で 1/30 秒毎に最新メトリクスを取得し、滑らかに更新。

## 5. インタラクション
- プリセットボタン: Outside/Inside それぞれの行で `[Home][User][Front][Back][Left][Right][Top]` の `TextButton` を並べ、クリックで対応する視点に切り替えつつズームも同期する。
- ズームスライダー: Inside/Outside 共通のズーム値に対応し、マウスホイール操作とも連携。
- マウスドラッグ: Inside/Outside どちらでも回転操作を提供。Inside は視点が原点に固定される。

## 6. 拡張ポイント
- `presetMap` に項目を追加すれば新しい視点を容易に追加可能。
- ズームのリマッピング式を `setZoomFactor` 内で集中管理しているため、レンジ変更や距離関数を簡単に調整できる。
- `colourForBands` をカスタマイズすることで帯域ごとの色味を変更できる。
