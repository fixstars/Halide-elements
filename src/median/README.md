# 概要

ノイズ除去ための1手法である、Median フィルタをHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - (i, j) 成分における周囲 window_width、window_height の画素の中央値を求め、(i, j) 成分に結果を返す。
  - window_width は正の奇数 (サンプルでは3)
  - window_height は正の奇数 (サンプルでは3)
---
Project Name: Median, Category: Library, Tag: 画像処理, プリミティブ
