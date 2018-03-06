# 概要

２枚の画像の類似度を計算する、SAD 処理をHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel グレースケール画像 x 2
- 出力: 1024 x 768 pixel グレースケール画像
- 処理内容:
  - output(x,y) = abs(input0(x,y)-input1(x,y)) で画素ごとの差分を出力する
---
Project Name: SAD, Category: Library, Tag: 画像処理, プリミティブ
