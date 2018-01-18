# 概要

エッジ抽出等に使われるLaplacian フィルタをHalide で実装しました。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - 次のKernel 行列との畳込み演算(Convolution) を行う
  
  <a href="https://www.codecogs.com/eqnedit.php?latex=\begin{pmatrix}&space;-1&space;&&space;-1&&space;-1\\&space;-1&&space;8&&space;-1\\&space;-1&&space;-1&&space;-1&space;\end{pmatrix}" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\begin{pmatrix}&space;-1&space;&&space;-1&&space;-1\\&space;-1&&space;8&&space;-1\\&space;-1&&space;-1&&space;-1&space;\end{pmatrix}" title="\begin{pmatrix} -1 & -1& -1\\ -1& 8& -1\\ -1& -1& -1 \end{pmatrix}" /></a>
  
---
Project Name: Laplacian, Category: Library, Tag: 画像処理, プリミティブ
