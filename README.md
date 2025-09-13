# src2build

UTF-8で記述されたファイルをShift_JISに変換しつつbuildディレクトリにコピーします。

無保証につき各自の責任で使用して下さい。

## Usage

* &lt;src&gt; …… 変換前(UTF-8)のファイルが置かれているディレクトリ。
* &lt;build&gt; …… 変換後(Shift_JIS)のファイルを書き込むディレクトリ。
  ディレクトリ名はbuildで固定。

&lt;build&gt;を作りたい場所にカレントディレクトリを変更し、以下のコマンドを実行します。
```
src2build --init-srcdir
```

カレントディレクトリにsrcdir.txtファイルが作成されます。
このファイルは&lt;src&gt;のパス名を記すテキストファイルです。
初期状態ではカレントディレクトリのsrcディレクトリが記入されています。
* &lt;src&gt;の名称がsrcで、&lt;build&gt;と同じ場所にあるなら変更不要です。
* それ以外の場合、テキストエディタで開き&lt;src&gt;のパス名に書き換えます。。

ここまでが準備作業です。次に以下のコマンドを実行します。
```
src2build
```

&lt;build&gt;ディレクトリが作成され、&lt;src&gt;ディレクトリ内のファイル(UTF-8)が
Shift_JISに変換され&lt;build&gt;に書き込まれます。
サブディレクトリがあれば再帰処理されます。

変換元のファイルを編集保存した場合は再度`src2build`コマンドを実行すれば、
既存ファイルのタイムスタンプを比較して新しいファイルのみ上書きされます。

### 応用編

&lt;build&gt;内のサブディレクトリで実行すると親ディレクトリを遡ってsrcdir.txtを探し、
見つかったディレクトリを基点として処理します。

実行時の引数で&lt;src&gt;のパス名を指定することができます。
この場合srcdir.txtは読み込まれません。

## Build

PCやネット上での取り扱いを用意にするために、src/内のファイルはUTF-8で記述されています。
X68000上でビルドする際には、UTF-8からShift_JISへの変換が必要です。

### src2buildを使用する場合

必要ツール: [src2build](https://github.com/kg68k/src2build)

srcディレクトリのある場所で以下のコマンドを実行します。
```
src2build src
make -C build
```

### u8tosjを使用する方法

必要ツール: [u8tosj](https://github.com/kg68k/u8tosj)

srcディレクトリのある場所で以下のコマンドを実行します。
```
make
make -C build
```

### その他の方法

src/内のファイルを適当なツールで適宜Shift_JISに変換してからsrc/内の`make`を実行してください。  
UTF-8のままでは正しくビルドできません。

## License
GNU GENERAL PUBLIC LICENSE Version 3 or later.

## Author
TcbnErik / https://github.com/kg68k/src2build
