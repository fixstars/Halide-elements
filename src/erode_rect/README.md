# 概要

2次元画像処理の基本的な処理のひとつであるErode(縮小) 処理をHalide で実装しました。
このErode は、処理ウィンドウの全体(長方形: Rectangle) に適用されます。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - 中心画素から( -(window_width/2), -(window_height/2) ) の位置から(window_width, window_height)の画素の最小値を出力する  
  - iteration回、上記処理を繰り返す
  - このソースコードでは、window_width = 3, window_height = 3, iteration = 2
---
Project Name: Erode(Rectangle), Category: Library, Tag: 画像処理, プリミティブ
