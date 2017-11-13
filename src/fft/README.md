# 概要

画像処理・信号処理の分野で幅広く使われているFFT をHalide で実装しました。

# 主な仕様

- 入力: 32bit 浮動小数点数配列
- 出力: 32bit 浮動小数点数配列
- FPGA サイクル数: 0.25 element/cycle
- FPGA ゲート数:
  - BRAM: 61
  - DSP: 204
  - FF: 27705
  - LUT: 20361

---
Project Name: FFT, Category: Library
