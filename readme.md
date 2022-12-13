```
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../
make
```

```
$LLVM_DIR/bin/clang -O1 -S -emit-llvm ../input/performance.c -o before.ll -fno-discard-value-names
$LLVM_DIR/bin/opt -load-pass-plugin ./lib/libBogusControlFlow.so -passes=BogusFlow  -o after.ll before.ll -S
clang-15 before.ll -o before.out
clang-15 after.ll -o after.out
./before.out
./after.out
```

```
$LLVM_DIR/bin/clang -O1 -S -emit-llvm ../input/performance2.c -o before.ll -fno-discard-value-names
$LLVM_DIR/bin/opt -load-pass-plugin ./lib/libBogusControlFlow.so -passes=BogusFlow  -o after.ll before.ll -S
clang-15 before.ll -o before.out
clang-15 after.ll -o after.out
./before.out
./after.out
```


```
$LLVM_DIR/bin/clang -O1 -S -emit-llvm ../input/baseball.c -o before.ll -fno-discard-value-names
$LLVM_DIR/bin/opt -load-pass-plugin ./lib/libBogusControlFlow.so -passes=BogusFlow  -o after.ll before.ll -S
clang-15 before.ll -o before.out
clang-15 after.ll -o after.out
./before.out
./after.out
```