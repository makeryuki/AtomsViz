# リリースチェックリスト (日本語版)

英語版: [release_checklist.md](release_checklist.md)

## 準備
- [ ] master が最新であり、テスト／手動確認が完了していることを確認。
- [ ] docs/work_log.md にリリース概要を追記。
- [ ] equirements.md のステータスが最新であることを確認。

## ビルド
- [ ] pwsh -File .\build_vst3.ps1 -Configuration Release を実行。
- [ ] cmake --build build-clap --config Release を実行（未構成なら cmake -B build-clap -S . 後）。
- [ ] macOS では以下を実行。
  - xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - AU" -configuration Release build
  - xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - VST3" -configuration Release build
  - xcodebuild -project Builds/MacOSX/AtmosViz.xcodeproj -scheme "AtmosViz - Standalone Plugin" -configuration Release build
  - cmake --build build-clap-mac --config Release（未構成なら cmake -B build-clap-mac -S . -G "Xcode" -DJUCER_GENERATOR=Xcode -DPATH_TO_JUCE=/Applications/JUCE）
- [ ] 成果物を確認。
  - [ ] Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3
  - [ ] uild-clap/AtmosViz_artefacts/Release/AtmosViz.clap
  - [ ] Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe
  - [ ] Builds/MacOSX/build/Release/AtmosViz.component
  - [ ] Builds/MacOSX/build/Release/AtmosViz.vst3
  - [ ] Builds/MacOSX/build/Release/AtmosViz.app
  - [ ] uild-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap
- [ ] Release ビルドで Heatmap / Temporal Trails / Peak Hold が意図通り描画されることを確認。

## パッケージング
- [ ] 配布バンドルを作成。
  `powershell
  Compress-Archive -Path Builds/VisualStudio2022/x64/Release/VST3/AtmosViz.vst3 -DestinationPath dist/AtmosViz_v<version>_Windows_VST3.zip -Force
  Compress-Archive -Path Builds/VisualStudio2022/x64/Release/Standalone Plugin/AtmosViz.exe -DestinationPath dist/AtmosViz_v<version>_Windows_Standalone.zip -Force
  Copy-Item build-clap/AtmosViz_artefacts/Release/AtmosViz.clap dist/AtmosViz_v<version>_Windows_CLAP.clap
  `
- [ ] macOS では以下を実行。
  `ash
  ditto -c -k --sequesterRsrc --keepParent Builds/MacOSX/build/Release/AtmosViz.component dist/AtmosViz_v<version>_macOS_AU.zip
  ditto -c -k --sequesterRsrc --keepParent Builds/MacOSX/build/Release/AtmosViz.vst3 dist/AtmosViz_v<version>_macOS_VST3.zip
  ditto -c -k --sequesterRsrc --keepParent Builds/MacOSX/build/Release/AtmosViz.app dist/AtmosViz_v<version>_macOS_Standalone.zip
  cp -R build-clap-mac/AtmosViz_artefacts/Release/AtmosViz.clap dist/AtmosViz_v<version>_macOS_CLAP.clap
  `
- [ ] zip と .clap を少なくとも 1 つのホストでスモークテスト。

## バージョニング
- [ ] 必要に応じてバージョンマクロ（JuceLibraryCode/JucePluginDefines.h）を更新。
- [ ] ドキュメント更新と dist 作業を含め、Release v<version> でコミット。
- [ ] タグ付け: git tag v<version>。

## 公開
- [ ] コミットとタグをプッシュ: git push && git push --tags。
- [ ] <version> の GitHub Release を作成し、以下を添付。
  - dist/AtmosViz_v<version>_Windows_VST3.zip
  - dist/AtmosViz_v<version>_Windows_Standalone.zip
  - dist/AtmosViz_v<version>_Windows_CLAP.clap
  - dist/AtmosViz_v<version>_macOS_AU.zip
  - dist/AtmosViz_v<version>_macOS_VST3.zip
  - dist/AtmosViz_v<version>_macOS_Standalone.zip
  - dist/AtmosViz_v<version>_macOS_CLAP.clap
- [ ] リリースノートには docs/work_log.md のハイライトを引用。

## ポストリリース
- [ ] 未解決の TODO を新規 Issue として登録。
- [ ] ビジュアル変更があれば 1/<yyyyMMdd>/ に新しいスクリーンショットを保存。
- [ ] 内部連絡チャネル（#atmosviz-dev）へダウンロードリンクを共有。
