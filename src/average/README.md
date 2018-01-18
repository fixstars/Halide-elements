# 概要

ノイズ除去ための1手法である、Average フィルタをHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - (i, j) 成分における周囲 window_width、window_height の画素の平均値を求め、(i, j) 成分に結果を返す
  - 演算結果の小数点以下は四捨五入する
  - window_width は正の奇数 (サンプルでは3)
  - window_height は正の奇数 (サンプルでは3)
---
Project Name: Average, Category: Library, Tag: 画像処理, プリミティブ
