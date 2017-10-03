# Halide-elements

Halide DSLで記述された部品を集める場所です。

## ビルド方法
標準のMakefileを使用する場合、環境変数HALIDE_ROOTにHalideのルートディレクトリへのパスを設定して、```make```すればビルドできます。
- ソースコードからビルドしたHalideを使用する場合は、加えて環境変数HALIDE_BUILDにHalideのビルドディレクトリへのパスを設定して下さい。

## 追加方法

1. テンプレート（copy） をコピーして新しくディレクトリを作る
2. ファイル名を新しい名前に変更する
3. Makefileの中の"copy"を新たな名前に置き換える
4. xxx_generator.cc にHalideDSLのコードを書く
5. xxx_test.cc にテストコードを書く