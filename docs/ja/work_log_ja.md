# AtmosViz 作業ログ (日本語版)

英語版: [work_log.md](work_log.md)

## 2025-11-01
- v0.6.0 をリリース（モノラル／ステレオ〜Dolby Atmos 9.x）し、Windows 形式の配布パッケージを生成。
- JUCE レイアウト全般への対応を文書化し、バージョン番号を更新。
- 今後の macOS 成果物向けにパッケージングフローを調整（詳細はリリースチェックリスト参照）。

## 2025-10-29
- docs/channel_layout_expansion.md にチャンネルレイアウト拡張ロードマップを記載。
- AudioProcessor / Visualizer を再構築し、モノラル／ステレオ〜Dolby Atmos 9.1.6（Atmos / ITU 変種）までの JUCE レイアウトを自動認識するよう対応。
- README、ユーザーマニュアル、要件ドキュメントを拡張レイアウトサポート内容に合わせて更新。

## 2025-10-28
- JUCE チャンネルタイプのバインドにより 7.1.4 のチャンネルマッピングを修正（L/R の入れ替わりを解消）。
- バージョンメタデータを 0.5.0 に引き上げ、README とユーザーマニュアルの記述を更新。
- Windows Release 成果物（VST3, Standalone, CLAP）を再ビルドし、GitHub v0.5.0 リリース準備完了。
- クロスプラットフォーム化のための macOS リリース手順を共有。

## 2025-10-27
- Visualization Gain コントロールを追加し、既存の色凡例を整理しつつヘッダーレイアウトを再調整。
- Heatmap と Frequency Emphasis のキャプションを削除して UI を簡素化しつつ色手掛かりを維持。
- Windows Release 成果物（VST3, Standalone, CLAP）をビルドし、v0.4.0 配布準備。
- clap 用 CMake 設定を 0.4.0 向けに更新。
- macOS Release（AU, VST3, Standalone, CLAP）のビルドを整備し、GitHub Release v0.4.0 に全成果物を添付。

## 2025-10-19
- macOS Release ビルド（AU, VST3, Standalone）を Xcode で生成し、dist/ に保存。
- /Applications/JUCE ツールチェーンで macOS CLAP バンドルをビルドし、パッケージング手順を追記。
- README、Developer Guide、Checklist、Requirements、User Manual を macOS インストール／ビルド手順で更新。
- clap-juce-extensions をサブモジュールとして導入し、CMake で Release CLAP を生成。
- スタンドアロン／CLAP の配布バンドルを dist/ に追加し、インストールステップを記載。
- Developer / Release ガイドに CLAP ビルド手順を追記。
- GPLv3 ライセンスを追加し、旧ドキュメントを docs/_archive/20251012/ に退避。

## 2025-10-12
- Inside のズーム制約を正規化（Home / User 既定 60%、Top 既定 25%、最小 40% / 25%）。
- Home / User の向きを調整（ヨー -95°、ピッチ -5°）し、原点から前壁を向くよう修正。
- Release 成果物を再ビルドし、dist/AtmosViz_v0.3.0_Windows_VST3.zip を公開。
- README、設計ドキュメント、マニュアルを刷新し、旧ノートを docs/_archive/20251012 にアーカイブ。
- Release スタンドアロンビルドの起動を確認。
- フォローアップ: Inside Top の向き調整、Radiation Heatmap の列カバレッジ検証、マニフェストヘルパー失敗の評価。

## 2025-10-11
- inside.html 参照に一致する行列ベースの Inside 射影を書き換え完了。
- リファクタ後の Outside ビュー安定性を復元。
- Colour Mix Pad コールアウト位置とヘッダーレイアウト（ヒートマップスライダーの右寄せ）を調整。

## 2025-10-07
- Band Colour Weights スライダーを追加し、Colour Mix Pad との同期を実装。
- ヒートマップ密度スライダーの表示切替と凡例更新を追加。
- Debug / Release 双方に対応した PowerShell ビルドスクリプト uild_vst3.ps1 を作成。

## 2025-09-30
- ドキュメント再開メモと初期ズームスライダー調整を記録。
- VST3 マニフェストヘルパーの失敗（MSB3073）を調査。

## 2025-09-20
- 初期設計ドキュメントを草案（現行版により置き換え済み）。
