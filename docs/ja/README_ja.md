# AtmosViz (日本語版)

英語版: [README.md](README.md)

AtmosViz は、JUCE をベースにした Dolby Atmos 可視化プラグインおよびスタンドアロンアプリケーションです。室内視点と外部視点の双方でスピーカーのアクティビティやヒートマップのエネルギー分布を描画し、没入型ミックスを監視するエンジニア向けに豊富な UI コントロールを提供します。

**制限事項:** AtmosViz は現在、JUCE 標準レイアウトのうちモノラル／ステレオから Dolby Atmos 9.1.6（Atmos / ITU 変種を含む）までをサポートしています。その他のバス構成には未対応です。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## ハイライト
- **Inside / Outside 視点スイート**
  - それぞれの視点に対し 8 種類のカメラプリセット（Home / Front / Back / Left / Right / Top / User）を用意し、適切なヨー・ピッチ・距離を初期設定。
  - Inside プリセットはリスニングポジションにカメラを固定し、ズームの下限を Home・User・Front・Back・Left・Right では 40%、Top では 25% に制限します。
  - Outside プリセットは直交投影の立方体を維持し、スピーカー配置を歪みなく俯瞰できます。
- **Radiation Heatmap ツールキット**
  - 密度スライダー（Coarse 〜 Ultra）がサンプリンググリッドを 5x5x3 から 13x13x11 セルまで変更。
  - 色は青 → 緑 → アンバー → 赤のランプでフレーム内のエネルギー範囲を表現します（詳細は docs/visualization_reference.md 参照）。
  - エネルギースムージングで静かなパッセージでもちらつきを抑えつつ応答性を確保。
- **色混合とスケーリング制御**
  - Low / Mid / High スライダーで周波数帯域の重み付けを調整。
  - Colour Mix Pad ボタンで三角パッド UI を開き、視覚的にパレットバランスを変更。
  - 新設の *Visualization Gain* スライダーはカメラズームと独立してローブやグリフの到達距離を制御し、従来の凡例枠を置き換えます。
- **時間軸系解析スイート**
  - Temporal Trails が移動するオブジェクトの軌跡を減衰付きで描画。
  - Peak Hold が各スピーカーの最新ピークを強調し、未使用チャンネルのトラブルシュートを支援。
  - Draw Scale モードを併用することで、ズームとは独立にグリフサイズを最適化。
- **チャンネルマッピングの安全策**
  - JUCE のチャンネルタイプを利用して各メーターを正しいスピーカーに結び付けます。
  - JUCE のスピーカーレイアウト（モノラル、ステレオ、クアッド、5.x、6.x、7.x SDDS/Atmos、9.x Atmos/ITU）に自動追従し、ステージ表示を再配置。
  - 未知のチャンネルは無視して描画を崩さず、追加ステムが入っても視覚化が破綻しません。
- **オートメーションとホスト統合**
  - 主なコントロールすべてを AudioProcessorValueTreeState パラメータ化し、DAW オートメーションとリコールに対応。
  - スタンドアロン実行ファイルもプラグインと同一 UI を備え、ホストを起動せずに動作確認が可能です。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## リポジトリ構成
- Source/ - プラグイン本体およびエディタの実装。
- JuceLibraryCode/ - JUCE により自動生成されたラッパコード。
- Builds/VisualStudio2022/ - Visual Studio プロジェクト群（VST3、Standalone、ヘルパー）。
- Builds/MacOSX/ - Xcode プロジェクトとビルド成果物（AU、VST3、Standalone）。
- dist/AtmosViz_v0.6.0_Windows_VST3.zip - 配布用 VST3 バンドル。
- dist/AtmosViz_v0.6.0_Windows_Standalone.zip - ポータブルなスタンドアロン実行ファイル。
- dist/AtmosViz_v0.6.0_Windows_CLAP.clap - clap-juce-extensions でビルドした CLAP プラグイン。
- dist/AtmosViz_v0.6.0_macOS_VST3.zip - macOS VST3 バンドル。
- dist/AtmosViz_v0.6.0_macOS_AU.zip - macOS Audio Unit コンポーネント。
- dist/AtmosViz_v0.6.0_macOS_Standalone.zip - macOS スタンドアロン .app バンドル。
- dist/AtmosViz_v0.6.0_macOS_CLAP.clap - macOS CLAP プラグインバンドル。
- docs/ - 最新の設計資料およびユーザー／開発者ドキュメント（下記参照）。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## ビルド方法

### ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## Windows
1. **Visual Studio 2022**（Desktop development with C++ ワークロード）をインストール。
2. JUCE 8.0.1 をインストールし、プロジェクト設定の JUCE パス（例: C:\\juce-8.0.1-windows\\JUCE）を確認。
3. リポジトリルートで以下を実行します。
   `powershell
   pwsh -File .\build_vst3.ps1 -Configuration Release
   `
   生成物:
   - VST3 バンドル: Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3
   - スタンドアロン実行ファイル: Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe

> **メモ:** JUCE の VST3 Manifest Helper は現在 MSB3073 を返しますが、プラグイン自体は正しく生成されます。マニフェストが不要であればポストビルド処理を無効化できます。

### ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## macOS
1. **Xcode 15**（および対応する Command Line Tools）をインストール。
2. JUCE 8.0.1 と Projucer を導入し、AtmosViz.jucer のモジュール検索パスを実環境に合わせて更新。
3. Xcode プロジェクトを再生成。
   `ash
   /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave AtmosViz.jucer
   `
4. プラグイン形式をビルド。
   `ash
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - AU" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - VST3" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - Standalone Plugin" -configuration Release build
   `
   生成物は Builds/MacOSX/build/Release/ に出力されます。
5. clap-juce-extensions を用いた CLAP バンドルをビルド。
   `ash
   cmake -B build-clap-mac -S . -G "Xcode" -DJUCER_GENERATOR=Xcode -DPATH_TO_JUCE=/Applications/JUCE
   cmake --build build-clap-mac --config Release
   `
   完成したバンドルは uild-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap に配置されます。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## プラグインのインストール
- **Windows (VST3):** Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3（または dist/AtmosViz_v0.6.0_Windows_VST3.zip 内のバンドル）を C:\Program Files\Common Files\VST3 へコピー。
- **macOS (VST3):** Builds/MacOSX/build/Release/AtmosViz.vst3（または dist/AtmosViz_v0.6.0_macOS_VST3.zip）を /Library/Audio/Plug-Ins/VST3/ へコピー。
- **macOS (AU):** Builds/MacOSX/build/Release/AtmosViz.component（または dist/AtmosViz_v0.6.0_macOS_AU.zip）を /Library/Audio/Plug-Ins/Components/ へコピー。
- コピー後にホスト DAW でプラグインスキャンを再実行してください。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## スタンドアロンの実行
- **Windows:** dist/AtmosViz_v0.6.0_Windows_Standalone.zip を展開し、AtmosViz.exe を起動。
- **macOS:** dist/AtmosViz_v0.6.0_macOS_Standalone.zip を展開し、AtmosViz.app を /Applications へ移動またはそのまま起動。初回起動時に Gatekeeper の許可が必要な場合があります。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## CLAP プラグインのインストール
- **Windows:** dist/AtmosViz_v0.6.0_Windows_CLAP.clap（または uild-clap の成果物）を C:/Program Files/Common Files/CLAP へ配置。
- **macOS:** dist/AtmosViz_v0.6.0_macOS_CLAP.clap（または uild-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap）を /Library/Audio/Plug-Ins/CLAP/ へ配置。
- カスタムフォルダを使用するホストでは適切なディレクトリへコピーし、プラグインスキャンを実行してください。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## ドキュメント
- [ユーザーマニュアル](docs/user_manual.md)
- ドキュメントセット（英語版）は以下のファイルで構成されます。
  - docs/basic_design.md
  - docs/detailed_design.md
  - docs/developer_guide.md
  - docs/requirements.md
  - docs/user_manual.md
  - docs/work_log.md
  - docs/visualization_reference.md
  - docs/channel_layout_expansion.md
  - docs/release_checklist.md
- 日本語版はそれぞれ _ja.md 付きファイルとして同ディレクトリに配置しています。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## リリース手順
1. Release ビルドスクリプト実行（前述参照）。
2. 生成された VST3 バンドルを dist/AtmosViz_v<version>_Windows_VST3.zip として圧縮。
3. コミットにタグを付与（git tag v<version>）し、git push && git push --tags。
4. GitHub Release を作成し、docs/work_log.md のハイライトを掲載しながら成果物を添付。
5. 公開前に少なくとも 1 つのホストでスモークテストを実施。

## ビデオデモ

[![機能デモ](https://img.youtube.com/vi/ScN5kZFw42c/hqdefault.jpg)](https://youtu.be/ScN5kZFw42c)

[![AtmosViz Demo: From a 7.1.2-Channel Bed to Rendered Dolby Atmos Channels](https://img.youtube.com/vi/sDN6ZurD0Lc/hqdefault.jpg)](https://youtu.be/sDN6ZurD0Lc)

## サポートと今後の課題
- カメラ数式を変更した際は Inside プリセットを必ず再確認し、Outside が直交を維持していることを確認。
- 直近のフォローアップ: Inside Top の向き調整、Radiation Heatmap の列カバレッジ検証。
- 問題・要望は GitHub Issues へスクリーンショットとホスト情報を添えて報告してください。

---
本ソフトウェアは Codex（GPT-5）とのペアプログラミングにより作成されています。

