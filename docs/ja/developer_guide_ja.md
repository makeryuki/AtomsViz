# Developer Guide (日本語版)

英語版: [developer_guide.md](developer_guide.md)

## 環境構築
1. Windows で開発する場合は Visual Studio 2022 (17.11+) の Desktop development with C++ ワークロードをインストール。
2. macOS では Xcode 15 以降と対応する Command Line Tools を導入。
3. すべてのプラットフォームで CLAP ビルドに必要な CMake 3.20 以上を用意。
4. JUCE 8.0.1 をインストールし、AtmosViz.jucer のモジュールパスを実際の配置場所（例: C:\juce-8.0.1-windows\JUCE、/Applications/JUCE）に合わせて更新。
5. リポジトリをクローンしてサブモジュールを初期化。
   `powershell
   git clone https://github.com/makeryuki/AtomsViz.git
   cd AtmosViz
   git submodule update --init --recursive
   `

## ビルド

### Windows
- **Debug ビルド:** pwsh -File .\build_vst3.ps1
- **Release ビルド:** pwsh -File .\build_vst3.ps1 -Configuration Release
- スクリプトは最初に見つかった amd64 用 MSBuild.exe を利用します。異なるエディションにインストールされている場合は uild_vst3.ps1 を調整してください。
- 主な生成物:
  - Builds/VisualStudio2022/x64/<Config>/VST3/AtmosViz.vst3
  - Builds/VisualStudio2022/x64/<Config>/Standalone Plugin/AtmosViz.exe

### macOS
1. JUCE モジュールパスを変更した後は Xcode プロジェクトを再生成します。
   `ash
   /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave AtmosViz.jucer
   `
2. Release 構成をビルド。
   `ash
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - AU" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - VST3" -configuration Release build
   xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - Standalone Plugin" -configuration Release build
   `
3. 生成物は Builds/MacOSX/build/Release/（.component, .vst3, .app）に出力されます。

### CLAP プラグインのビルド
1. プラットフォームごとの Release ビルド（Windows: Shared Code/AtmosViz.lib、macOS: Builds/MacOSX/build/Release/libAtmosViz.a）が生成済みであることを確認。
2. Projucer プロジェクトをラップする CMake を構成。
   - Windows:
     `powershell
     cmake -B build-clap -S . -G "Visual Studio 17 2022"
     `
   - macOS:
     `ash
     cmake -B build-clap-mac -S . -G "Xcode" -DJUCER_GENERATOR=Xcode -DPATH_TO_JUCE=/Applications/JUCE
     `
3. Release 構成をビルド。
   - Windows: cmake --build build-clap --config Release
   - macOS: cmake --build build-clap-mac --config Release
4. 完成したプラグインバンドルは Windows では uild-clap/AtmosViz_artefacts/Release/AtmosViz.clap、macOS では uild-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap に出力されます。

## デバッグのヒント
- 高速な反復にはスタンドアロン exe を起動し、必要に応じて Visual Studio からデバッガをアタッチ。
- コンポーネントレイアウトの試行錯誤には JUCE Projucer を使えますが、最終的なソースは Source/ に維持してください。
- Debug ビルドで JUCE のアサートを有効にし、カメラ計算の退行を早期検知。
- 一時的な DBG ログは調査後に必ず削除。

## コーディングガイドライン
- C++17 のみを使用し、コンパイラ依存の拡張は避ける。
- juce:: のユーティリティ（Vector、Colour など）を優先し、STL の方が適任な場合のみ切り替え。
- UI カラー定数は PluginEditor.cpp 冒頭付近にまとめて配置。
- コメントやドキュメントは原則 ASCII を保持し、エンコーディング問題（C4819）を防止。

## ドキュメント運用
1. 大幅な改訂前に現行ドキュメントを docs/_archive/<yyyyMMdd>/ へ退避。
2. README.md および関連文書（basic_design, detailed_design, user_manual 等）を更新。
3. 主要な変更は絶対日付付きで docs/work_log.md に記録。
4. 未完了項目は docs/requirements.md の TODO と突き合わせ。

## リリースパッケージング
詳細手順は docs/release_checklist.md を参照してください。主な自動化は以下です。
- uild_vst3.ps1 -Configuration Release
- cmake --build build-clap --config Release
- Compress-Archive で配布用 zip を作成、.clap をコピー
- Git タグ付けと GitHub Release 作成

## テスト戦略（現状）
- **ビジュアル:** 全カメラプリセットとビューモードを手動で確認（1/ ディレクトリのスクリーンショット参照）。
- **オーディオ:** ピンクノイズを Atmos バスに流し、色応答とヒートマップ挙動を検証。
- **回帰:** 射影数式を変更した際はアーカイブ済みスクリーンショットと比較。

## 将来の拡張
- computeInsideProjectionParameters 向けのユニットテストを追加。
- JUCE の Image クラスを使った自動スクリーンショット比較を検討。
- マニフェストヘルパーの x64 対応を調査し、ポストビルド警告を解消。
