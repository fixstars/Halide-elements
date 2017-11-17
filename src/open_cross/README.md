# 概要

ノイズ除去などに使用される、Open 処理をHalide で実装しました。複数回の、Erosion(収縮) とDilation(膨張) を実行します。

# 主な仕様

- 入力: 1024 x 768 pixel グレースケール画像(unit8)
- 出力: 1024 x 768 pixel グレースケール画像(unit8)
- 処理内容:
  - iteration 回 Erode(Cross) を実行し、iteration 回 Dirate(Cross) を実行する
  - Erode, Dirate の処理内容に関しては、Erode(Cross), Dirate(Cross) のProject を参照のこと
  - このソースコードでは、window_width = 3, window_height = 3, iteration = 2
---
Project Name: Open(Cross), Category: Library, Tag: 画像処理, プリミティブ
