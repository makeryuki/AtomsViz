# AtmosViz ユーザーマニュアル (日本語版)

英語版: [user_manual.md](user_manual.md)

**制限事項:** AtmosViz は現在、JUCE 標準レイアウトのうちモノラル／ステレオから Dolby Atmos 9.1.6（Atmos / ITU 変種）までをサポートしています。これ以外のチャンネル設定では動作しません。

## 1. インストール
**Windows (VST3 + Standalone)**
1. dist/AtmosViz_v0.6.0_Windows_VST3.zip をダウンロード。
2. 展開すると AtmosViz.vst3（バンドルフォルダ）が得られます。
3. AtmosViz.vst3 を C:\Program Files\Common Files\VST3\ にコピー。
4. DAW を起動し、プラグインスキャンを再実行します（多くのホストでは *Effects* カテゴリに表示）。
5. dist/AtmosViz_v0.6.0_Windows_Standalone.zip を展開し、AtmosViz.exe を起動するとスタンドアロンツールとして動作します。

**macOS (AU / VST3 / Standalone / CLAP)**
1. 公開中の macOS 向けアーカイブ（dist/AtmosViz_v0.6.0_macOS_AU.zip、dist/AtmosViz_v0.6.0_macOS_VST3.zip、dist/AtmosViz_v0.6.0_macOS_Standalone.zip）および CLAP バンドル（dist/AtmosViz_v0.6.0_macOS_CLAP.clap）をダウンロード。
2. 必要なアーカイブを展開すると、それぞれバンドル (AtmosViz.component, AtmosViz.vst3, AtmosViz.clap) が得られます。
3. 各バンドルを標準ディレクトリにコピー。
   - AU: /Library/Audio/Plug-Ins/Components/
   - VST3: /Library/Audio/Plug-Ins/VST3/
   - CLAP: /Library/Audio/Plug-Ins/CLAP/
4. スタンドアロンの場合は AtmosViz.app を /Applications/ へドラッグ（または展開フォルダから直接起動）。初回起動時は Gatekeeper の許可が必要になる場合があります（システム設定 > プライバシーとセキュリティ）。
5. コピー後にホストでプラグインスキャンを実行します。Logic Pro や MainStage など一部ホストは Audio Unit 追加時に再起動が必要です。

## 2. クイックスタート
1. AtmosViz を Atmos 対応セッションで開きます。
2. ドロップダウンから **View Mode** を選択。
   - *Radiation Heatmap*
   - *Temporal Trails*
   - *Peak Hold*
3. **Perspective**（Inside / Outside）とカメラプリセットボタン（Home, Front, Back, Left, Right, Top, User）を選択。
4. **Zoom スライダー**（10%〜200%）を使用。Inside プリセットでは最小 40%（Top は 25%）が強制されます。
5. **Heatmap Density**（表示されている場合）で空間サンプリングを調整。
6. **Band Colour Weights** や **Colour Mix Pad** を使って Low / Mid / High の色バランスを再設定。
7. **Visualization Gain** スライダーでカメラズームとは独立にローブのサイズを調整。

## 3. コントロール一覧
| コントロール | 位置 | 説明 |
|--------------|------|------|
| View Mode コンボ | ヘッダー左側 | 可視化アルゴリズムを切り替え。Heatmap 選択時に密度スライダーが表示。 |
| Slider Mode コンボ | ヘッダー | 水平スライダーが Zoom か Draw Scale どちらを操作するかを選択。 |
| Zoom / Draw Scale スライダー | ヘッダー | Zoom: 10〜200%。Inside では下限を厳守。Draw Scale はカメラ位置と無関係にグリフ到達距離を調整。 |
| Visualization Gain スライダー | ヘッダー | 基準値に対して -50% 〜 +100% の範囲でローブ／トレイルの大きさを変更。 |
| Heatmap Density スライダー | ヘッダー（Heatmap 時） | レベル 1〜5（Coarse → Ultra）でサンプリンググリッド解像度を変更。 |
| Band Colour スライダー | ヘッダー | Low / Mid / High の線形スライダーで色の寄与率を設定。 |
| Colour Mix Pad ボタン | ヘッダー | 三角パッドを開き、ノードをドラッグして重みを視覚的に調整。 |
| カメラプリセットボタン | ヘッダー行 | Inside / Outside 各プリセットを即座に切り替え。User は最後に保存した手動姿勢を保持。 |
| Reset to User | コンテキストメニュー | ビジュアライザを右クリックして User 状態へリセット。 |

## 4. カメラ挙動
- **Inside Home / User**: カメラは部屋の原点に固定。既定ヨー -95°、ピッチ -5°、ズーム 60%。ホイール操作でも 40%以上を維持。
- **Inside Top**: 原点から下向きに俯瞰。既定ズーム 25%、下限 25%。
- **Outside**: 部屋の境界ボックスを中心とした直交軌道。Inside の制限とは独立したズーム範囲。

## 5. 色混合
- 色は正規化した帯域エネルギー（Low / Mid / High）とユーザー指定の重みから決定。
- 既定重みは Low=0.33, Mid=0.33, High=0.34 でバランス型。
- Colour Mix Pad の三角形は、左下が青 (Low)、右下が緑 (Mid)、頂点が赤 (High)。内部の位置は中間色になります。

## 6. ヒートマップの読み方
- ヒートマップ強度は現在フレームのピークに合わせて正規化し、ちらつきを抑えるために平滑化しています。
- 密度レベルごとのグリッド解像度（奥行 x 横 x 高さ）は以下の通り。
  - 1: 5 x 5 x 3
  - 2: 7 x 7 x 5
  - 3: 9 x 9 x 7
  - 4: 11 x 11 x 9
  - 5: 13 x 13 x 11
- 色は青 → 緑 → アンバー → 赤の勾配で、そのフレーム内のエネルギーレンジを示します。

## 7. トラブルシュート
| 症状 | 対処 |
|------|------|
| DAW にプラグインが表示されない | \\VST3\\（Windows）や /Library/Audio/Plug-Ins/{VST3, Components, CLAP}（macOS）に配置されているか確認し、ホストを閉じた状態で再スキャン。 |
| macOS AU が検証に失敗する | システム設定 > プライバシーとセキュリティ から AtmosViz を許可し、ホストを再起動して AU の検証を再実行。 |
| カメラが意図せず移動する | Slider Mode が Zoom になっているか確認。右クリックで User 状態にリセット。 |
| Colour Mix Pad が開かない | エディタウィンドウにフォーカスがあるか確認。ボタンを押すとフローティングコールアウトが開きます。 |
| Heatmap スライダーが見えない | View Mode を *Radiation Heatmap* に切り替えてください。 |
| Outside ビューが歪む | 最新の master を使用しているか確認し、Outside グループのプリセットボタンを選択。 |

## 8. サポートチャネル
- GitHub Issues にホスト情報・サンプルレート・スクリーンショット（1/ フォルダ形式）を添えて報告してください。

## 9. 今後のロードマップ（スナップショット）
- Inside Top の向きを改善し、前壁をビューボトムに配置。
- カメラスイープを用いたビジュアル回帰の自動化。
- ルームチューニングのための 3D アセットオーバーレイ検討。
