# ビジュアライゼーションリファレンス (日本語版)

英語版: [visualization_reference.md](visualization_reference.md)

## View Mode 一覧
| モード | 概要 | 備考 |
|--------|------|------|
| Directional Lobes | 各スピーカーから放射されるカバレッジローブをワイヤーフレームで描画。 | 静的ベッドの指向性チェックに有用。色は帯域重み付けに基づく。 |
| Layered Lobes | 周波数帯ごとのローブを半透明で積層し、Low / Mid / High のバランスを強調。 | レイヤーの不透明度は現在の帯域スライダー値を反映。 |
| Directivity Balloon | 各スピーカーの周囲に合成バルーンを描画し、総和エネルギーを表示。 | 密なシーンでは Draw Scale モードで重なりを軽減。 |
| Radiation Heatmap | 室内を 3D グリッドでサンプリングし、色付きのクアッドとして表示。 | 密度スライダー（1〜5）が解像度を変更。色は青 → 緑 → アンバー → 赤のランプ。 |
| Temporal Trails | 移動するスピーカー／オブジェクトの軌跡を減衰付きで描画し、帯域色で区別。 | トレイル長は 	railBufferCapacity に基づき、約 2 秒でフェード。 |

## Visualization Gain
- ヘッダーにある専用スライダーで、カメラズームとは独立にローブ／トレイルの到達距離を調整。
- 範囲: -50%（小さく）〜 +100%（大きく）。
- Draw Scale モードと併用し、ズームはフレーミング用、Visualization Gain は密度調整用として使い分け。

## ヒートマップ密度対応表
| レベル | ラベル | グリッド（奥行 x 横 x 高さ） |
|--------|--------|---------------------------|
| 1 | Coarse | 5 x 5 x 3 |
| 2 | Low | 7 x 7 x 5 |
| 3 | Medium | 9 x 9 x 7 |
| 4 | High | 11 x 11 x 9 |
| 5 | Ultra | 13 x 13 x 11 |

## ヒートマップカラーランプ
| 位置 | 色 | 意味 |
|------|----|------|
| 0.00 | Blue (#1E3A8A) | 最小エネルギー |
| 0.25 | Teal (#0EA5E9) | 低エネルギー |
| 0.50 | Green (#22C55E) | 中程度 |
| 0.75 | Amber (#F59E0B) | 高エネルギー |
| 1.00 | Red (#F87171) | 最大エネルギー |

*凡例フレームは UI から削除済み。色の解釈は本表を参照してください。*

## 帯域カラー重み
- 既定値: Low 0.33、Mid 0.33、High 0.34。
- スライダー範囲: 各帯域 0.0〜1.5。最終的な色計算時に自動正規化。
- Colour Mix Pad の頂点:
  - **Low (青)**: 左下ノード
  - **Mid (緑)**: 右下ノード
  - **High (赤)**: 上部ノード
- 任意のノードを移動すると他ノードを再調整し、重み合計 1.0 を維持。

## カメラプリセット
| グループ | プリセット | ヨー | ピッチ | 距離 | 既定ズーム |
|----------|------------|------|--------|------|--------------|
| Inside | Home | -95° | -5° | insideHomeBaseDistance | 0.60 |
| Inside | User | 保存済み状態 | 可変 | stored | 0.60 |
| Inside | Top | 180° | -90° | insideTopDistance | 0.25 |
| Inside | Front | -90° | 0° | insideDefaultDistance | 0.60 |
| Inside | Back | 90° | 0° | insideDefaultDistance | 0.60 |
| Inside | Left | 180° | 0° | insideDefaultDistance | 0.60 |
| Inside | Right | 0° | 0° | insideDefaultDistance | 0.60 |
| Outside | Home | -60° | 15° | outsideDefaultDistance | 1.00 |
| Outside | Front/Back/Left/Right/Top | 軸に沿ったヨー／ピッチ | outsideDefaultDistance | 1.00 |

## ヘッダーの区切りとレイアウト指針
- ヘッダーは 8 px 単位のスペーシングを採用し、ヒートマップ制御と帯域重みブロックの後に区切り線を挿入。
- フルレイアウトを維持する最小ウィンドウ幅は 980 px。
- スライダーラベルは Zoom モード時に "Zoom (10%-200%)" を表示し、Draw Scale モード時はツールチップのみ変更。Visualization Gain は専用の数値表示を持ちます。

## QA チェックリスト（抜粋）
1. Inside の 6 プリセットがズーム下限に違反しないこと。
2. Outside の各プリセットが直交・軸整合を保っていること。
3. Heatmap のラベル＋スライダー＋値ラベルが View Mode コンボ直下で右寄せに並ぶ（ずれは 1 px 未満）。
4. Colour Mix Pad ボタンでコールアウトが開閉し、閉じた後にフォーカスがエディタへ戻ること。
5. Visualization Gain スライダーがプリセット変更や Draw Scale モード切替に追従し、範囲外にクリップしないこと。
