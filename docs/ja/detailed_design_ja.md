# AtmosViz 詳細設計 (日本語版)

英語版: [detailed_design.md](detailed_design.md)

## UI 構成
- **ヘッダーストリップ**
  - isualizationCombo: Radiation Heatmap、Temporal Trails、Peak Hold などを選択。
  - sliderModeCombo: スライダーが Zoom を扱うか Draw Scale を扱うかを切り替え。
  - heatmapDensitySlider: Radiation Heatmap モードのときのみ表示（レベル 1〜5、Coarse → Ultra）。
  - andWeightTitleLabel と 3 つのロータリースライダー（Low / Mid / High）は colourMixWeights パラメータにマップ。
  - colourPadButton: ColourMixPadComponent を収めた juce::CallOutBox を開き、視覚的に重みを調整。
  - colourLegend: 現行の可視化モードに応じたキャプション付きグラデーションバー。
- **コンテンツ領域**
  - SpeakerVisualizerComponent がエディタ残領域を占有し、描画とマウス操作を担当。

## パラメータバインディング
エディタは全スライダーに対して AudioProcessorValueTreeState::SliderAttachment を生成します。コンボボックスは変更イベントを捕捉し、プロセッサーのセッターを明示的に呼び出します。Colour Mix Pad は同じ 3 パラメータを更新し、スライダーとの整合を保ちます。

## カメラ処理
- カメラプリセットは SpeakerVisualizerComponent::CameraPreset に列挙され、エディタはセグメント化ボタン群として表示します。
- pplyZoomFactorToCamera() は getCurrentMinZoom() を通じて Inside / Outside で異なるズーム下限を適用します。
- setCameraPreset() はプリセットごとのヨー、ピッチ、距離を保存し、射影行列を再計算します。
- Inside の射影は computeInsideProjectionParameters() で算出した水平・垂直 FOV、ニアプレーン、焦点距離を利用します。

## レンダリングパイプライン
1. **データ準備**
   - プロセッサーから取得したメトリクスを毎フレーム speakers 配列へコピー。
   - ヒートマップのグリッドポイントは密度変更時に Lazy 再生成。
   - トレイルは減衰付きアルファを持つリングバッファで管理。
2. **射影**
   - projectPoint() が Inside（透視）と Outside（直交）を切り替え。
   - Inside ビューではニアプレーンを基準化し、depth < insideNearPlane の点は破棄。
3. **描画**
   - 室内ラインは drawRoomOutside() もしくは drawRoomInside() が担当。
   - スピーカーグリフは juce::Path を用い、スピーカーごとの色で描画。
   - ヒートマップセルは正規化エネルギーに比例したアルファで合成。
   - 凡例やラベルは、過去に発生した C2661 エラーを避けるためにサイズを制限しつつ Graphics::drawFittedText を使用。

## カラーシステム
- 周波数帯（Low / Mid / High）は andColourWeights に保存された重みで合成。
- Colour Mix Pad では三角形 UI の重心座標を重みトリプレットへ変換。
- ヒートマップグラデーションは colourForHeatmapRatio() で取得し、凡例と同期。

## インタラクションモデル
- マウスホイール: ズームを調整（下限／上限はモードに依存）。Draw Scale モード時は描画スケールに切替。
- 左ドラッグ（Inside ビュー）: ヨー／ピッチを回転。ジンバルロックを避けるために範囲を制限。
- 右クリック: User プリセットのデフォルトにリセット。
- コールアウトパッド: CallOutBox::launchAsynchronously で Colour Mix Pad ボタンにアンカー。

## 永続化
- getStateInformation は ValueTree をシリアライズし、最新のカメラプリセット ID と色重みも保存します。
- setStateInformation はパラメータを復元した後に pplyZoomFactorToCamera() を呼び、ビューを再同期。

## エラー処理とロギング
- デバッグビルドではプリセット遷移や射影の異常値をアサートで監視。
- UI コードは isualizer ポインタが有効か確認してからアクセスし、ヌル参照を防止。

## 既知の制限
- VST3 マニフェストヘルパーは無効化されたままで、moduleinfo メタデータは含まれません。
- ヒートマップ計算は直方体の部屋を前提としており、不規則な形状へ対応するにはグリッド再設計が必要です。
- 自動テストは未実装のため、カメラ／射影変更後は手動回帰テストが必須です。
