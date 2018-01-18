# 概要

2次元画像処理の基本的な処理のひとつであるClose 処理をHalide で実装しました。
このClose は、処理ウィンドウの全体(長方形: Rectangle) に適用されます。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - iteration 回 dilate_rect を実行し、iteration 回 erode_rect を実行する
  - このソースコードでは、window_width = window_height = 3, iteration = 2, kernel は 3 x 3 のテーブル
---
Project Name: Close(Rectangle), Category: Library, Tag: 画像処理, プリミティブ
