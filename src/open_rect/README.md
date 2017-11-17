# 概要

ノイズ除去などに使用される、Open 処理をHalide で実装しました。複数回の、Erosion(収縮) とDilation(膨張) を実行します。

# 主な仕様

- 入力: 1024 x 768 pixel グレースケール画像
- 出力: 1024 x 768 pixel グレースケール画像
- 処理内容:
  - iteration 回 Erode(Rectangle) を実行し、iteration 回 Dirate(Rectangle) を実行する
  - Erode(Rectangle), Dirate(Rectangle) の処理内容に関しては、それぞれのProject を参照のこと
  - このソースコードでは、window_width = 3, window_height = 3, iteration = 2
---
Project Name: Open(Rectangle), Category: Library, Tag: 画像処理, プリミティブ
