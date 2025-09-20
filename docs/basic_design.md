# 基本設計書 (AtmosViz)

## 1. システム構成
- ベースフレームワーク: JUCE 8.0.1
- プロジェクト構成
  - `PluginProcessor` : オーディオ処理・解析を担当。
  - `PluginEditor`    : 3D 可視化 UI、カメラ制御、ユーザーインタラクション。
  - `docs/`           : 各種ドキュメント格納先。

## 2. データフロー
1. DAW から 7.1.4 オーディオバッファを受け取る。
2. `PluginProcessor::processBlock` で各チャンネルの RMS / Peak / FFT を計算し、最新メトリクスを共有メモリ (`latestMetrics`) に格納。
3. `PluginEditor` から定期的にメトリクスを取得し、GUI スレッドで可視化を更新。
4. ユーザー操作 (カメラボタン / ズームスライダー / マウスドラッグ) は `SpeakerVisualizerComponent` 内で処理し、描画パラメータに反映。

## 3. UI 構成
- 上部コントロールバー
  - 2 行構成のカメラプリセットパネル（最左列ラベル `Outside` / `Inside`、ラベル右に `[Home][User][Front][Back][Left][Right][Top]` を横並びで配置。各列は inside/outside ペアで揃え、User は両行で独立したカスタム視点スロットを持つ）
  - ズームスライダー (10%〜200%、テキストボックス付き)
- メインビュー
  - ルームワイヤーフレーム
  - スピーカー球体・指向性ローブ
  - XYZ ギズモ
## 4. カメラ設計
- カメラプリセットは `CameraParameters` で yaw/pitch/inside/距離を定義し、列キー (Home/User/Front/Back/Left/Right/Top) ごとに inside/outside ペアを保持。
- Outside 行: 距離パラメータを直接利用し、Home は部屋全体が必ず収まる固定距離（目安 40 m）で俯瞰。Front/Back/Left/Right は各面の外側から法線方向を向き箱状の部屋を映し、Top は天井の外側から真下に向けて天井面とトップスピーカーを俯瞰する。その他の方位も 10%〜200% の倍率を距離換算に用いる。
- Inside 行: カメラ位置は原点（リスニング位置）固定のオービットビューで、Home では室内ホームビュー（原点から正面を向く）。Front/Back/Left/Right は向きを +Z/-Z/-X/+X に切り替え、Top は原点で真上（+Y）を向いて天井面とトップスピーカーを俯瞰する。
- Outside 行は常に `cameraInside = false` で距離パラメータを使用し、Inside 行は常に `cameraInside = true` で原点固定のオービットを維持。
- User 列は現在の視点を一時保存・呼び戻すスロット（永続化は別途検討）で、Inside/Outside 個別に記録する。Inside 側は原点固定のままユーザー調整した yaw/pitch/ズームを保持し、初期値はいずれもフロントを含む斜めビューに設定する。
- マウスホイール・ズームスライダーは Home 視点を 100% とする倍率を更新し、Outside は基準距離（約 40 m）を倍率換算、Inside は原点固定のまま FOV 相当を倍率換算する。
## 5. スピーカー可視化
- Dolby 推奨角度・高さを `SpeakerDefinitions` として定義。
- スピーカーごとに `SpeakerMetrics` (rms, peak, frequency bands) を保持。
- ローブは RMS 値に応じて長さ (main) と広がり (spread) を変化させ、色は低/中/高帯域の比率でグラデーション。

## 6. 今後の拡張に備えたポイント
- `CameraPreset` の列挙を追加するだけで新しい視点を増やせる構造。
- ズーム・距離マッピングは共通関数化しており、レンジ調整が容易。
- `latestMetrics` は固定サイズ配列のため、7.1.4 以外のレイアウト対応時は再生成の必要あり。
