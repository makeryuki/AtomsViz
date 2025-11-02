# AtmosViz 基本設計 (日本語版)

英語版: [basic_design.md](basic_design.md)

## 目的
AtmosViz は Dolby Atmos スピーカーレイアウトを可視化し、没入型ミックス監視を支援するツールです。以下の 2 つの視点で空間エネルギーを描画します。
- **Outside ビュー**: 室外からの直交投影。エンジニアリングチェック向け。
- **Inside ビュー**: リファレンスリスニングポジションからの透視投影。ミキサー視点を再現。

## 主なサブシステム
1. **オーディオプロセッサ (AtmosVizAudioProcessor)**
   - スピーカーごとのレベルデータと周波数帯域を受信。
   - 可視化に利用するメトリクス（RMS、ピーク、帯域エネルギー）を集計。
   - ズーム、描画スケール、ビューモード、ヒートマップ密度、色重み付け用パラメータを公開。
2. **エディタ (AtmosVizAudioProcessorEditor)**
   - ビューモード選択、スライダー、Colour Mix Pad ボタン等をホストし、プロセッサーパラメータと同期。
   - レイアウトとコントロールの表示／非表示を管理（例: ヒートマップ密度スライダーは Radiation Heatmap 時のみ表示）。
3. **スピーカービジュアライザ (SpeakerVisualizerComponent)**
   - 室内ワイヤー、スピーカー、トレイル、ヒートマップを描画。
   - カメラ操作（Inside/Outside プリセット、ズーム制限、射影行列）をカプセル化。
   - 色凡例コンテンツを生成し、ズーム変更をエディタへ通知。
4. **色関連コンポーネント**
   - ColourLegendComponent はエネルギーマッピングのグラデーションバーを表示。
   - ColourMixPadComponent はローバンド／ミッドバンド／ハイバンドのバランスを直感的に調整できるコールアウト。

## データフロー
`
Audio Input → AtmosVizAudioProcessor (metrics & bands) → SpeakerVisualizerComponent
          → UI Parameters (ValueTree & slider bindings) → AtmosVizAudioProcessorEditor
`

## カメラと射影の概要
- Outside ビューは固定スケールの直交投影を使用します。
- Inside ビューは computeInsideProjectionParameters() が算出する透視行列を用い、自然な一人称視点を実現します。プリセットの既定値は以下の通りです。
  - Home / User 既定ズーム: 0.60（最小 0.40）
  - Top 既定ズーム: 0.25（最小 0.25）
  - Outside プリセット既定ズーム: 1.0（スライダー範囲 0.10〜2.0）

## 描画レイヤー
1. **室内ワイヤー**: Outside では軸整合の直方体、Inside では近接クリッピングを考慮した透視ライン。
2. **スピーカー**: Dolby Atmos 定義に基づくアイコン。帯域重み付けから色を算出。
3. **オーバーレイ**: モードに応じてトレイル、ヒートマップタイル、アクティビティグリフを表示。
4. **HUD 要素**: 凡例、区切り線、メーターなどをエディタ側で描画。

## 外部依存
- JUCE 8.0.1（GUI とオーディオプラグインフレームワーク）。
- ツールチェーン: MSVC 14.41（Visual Studio 2022）、C++17。
- オプション: x64 ビルドとバンドル組み立てを自動化する PowerShell スクリプト uild_vst3.ps1。

## 永続化と状態管理
- UI 状態はプロセッサーの AudioProcessorValueTreeState に保存され、DAW セッションとともにシリアライズされます。
- ユーザーカメラプリセットはヨー／ピッチ／距離／ズームを格納し、パラメータリコール時に復元されます。

## 現在のテスト戦略
- スタンドアロン実行ファイルによる手動確認。
- VST3 対応 DAW でのスモークテスト（オートメーション、リコール、再スキャン動作）。
- docs/work_log.md に記載したビジュアル回帰チェックリストを用いた将来の自動化準備。
