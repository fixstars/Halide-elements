# 概要

入力画像のヒストグラムを作成する処理をHalide で実装しました。

# 主な仕様

- 入力: 768 x 1280 pixel, グレースケール画像 (unit8 or unit16)
- 出力: 768 x 1280 pixel, グレースケール画像 (uint 32)
- 処理内容:
  - 出力のサイズになるように正規化されたヒストグラムを返す
  – 出力のx座標は、入力画像(i, j) x (出力画像の幅 / UINT[8 or 16]\_MAX)で計算する。
---
Project Name: ヒストグラム, Category: Library
