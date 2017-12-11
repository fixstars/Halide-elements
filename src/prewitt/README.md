# 概要

2次元画像処理において、領域の境界(エッジ)を抽出する処理のひとつであるPrewitt をHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - Kx カーネルとのconvolution(畳み込み)による(i, j) 成分の画素値をKx(i, j) とし、Ky カーネルとのconvolution による(i, j) 成分の画素値をKy(i, j) とする。このとき、出力結果の(i, j)  成分は以下となる。

<img src = "https://latex.codecogs.com/gif.latex?\sqrt{K_x&space;(i,&space;j)^2&space;&plus;&space;K_y&space;(i,&space;j)^2}"> </img>

  - Kx カーネル

<img src = "https://latex.codecogs.com/gif.latex?\begin{pmatrix}&space;-1&space;&&space;0&space;&&space;1\\&space;-1&space;&&space;0&space;&&space;1\\&space;-1&space;&&space;0&space;&&space;1\\&space;\end{pmatrix}"></img>

  - Ky カーネル

<img src = "https://latex.codecogs.com/gif.latex?\begin{pmatrix}&space;-1&space;&&space;-1&space;&&space;-1\\&space;0&space;&&space;0&space;&&space;0\\&space;1&space;&&space;1&space;&&space;1\\&space;\end{pmatrix}"></img>

---
Project Name: Prewitt, Category: Library, Tag: 画像処理, プリミティブ
