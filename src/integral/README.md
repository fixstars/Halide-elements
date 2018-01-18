# 概要

入力画像に関して注目画素の(0,0)からの値の総和を計算し、いわゆる積分画像(Integral Image) を出力します。

# 主な仕様

- 入力: 1024 x 768 pixel, グレースケール画像
- 出力: 1024 x 768 pixel, グレースケール画像
- 処理内容:
  - 以下の通り、積分値を出力する
  
  <a href="https://www.codecogs.com/eqnedit.php?latex=\mathrm{output0}(i,j)&space;=&space;\sum_{y=0}^{j}&space;\sum_{x=0}^{i}&space;\mathrm{input0}&space;(x,y)" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\mathrm{output0}(i,j)&space;=&space;\sum_{y=0}^{j}&space;\sum_{x=0}^{i}&space;\mathrm{input0}&space;(x,y)" title="\mathrm{output0}(i,j) = \sum_{y=0}^{j} \sum_{x=0}^{i} \mathrm{input0} (x,y)" /></a>
  
---
Project Name: Integral, Category: Library, Tag: 画像処理, プリミティブ
