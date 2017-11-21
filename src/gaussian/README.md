# 概要

画像の平滑化処理に使われるガウシアンフィルタをHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - カーネルを下記の式で生成する
    - \exp ( - \frac{(Distance\ from\ center)^2}{2 \sigma ^2})
  - 続いてこのカーネルの要素の総和でカーネルの各要素を割る
  - 生成されたカーネルを正規化し、ガウシアンカーネルとする
  - sigma をパラメータとしたwindow_width x window_height のガウシアンカーネルとの畳み込み処理を行う  
  - このソースコードでは、sigma = 1.0, window_height = window_width = 3
---
Project Name: Gaussian, Category: Library, Tag: 画像処理, プリミティブ