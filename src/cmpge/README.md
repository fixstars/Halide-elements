# 概要

2つの入力画像の比較演算をHalideで実装しました。

# 主な仕様

- 入力0: 768 x 1280 pixel, グレースケール画像
- 入力1: 768 x 1280 pixel, グレースケール画像
- 出力0: 768 x 1280 pixel, グレースケール画像
- 処理内容:
  - 出力0(i, j) = (入力0(i, j) >= 入力1(i, j)) ? (型の最大値)：0 の比較演算を行う
---
Project Name: 比較演算(Greater than or equal), Category: Library
