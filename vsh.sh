clang -O0 -emit-llvm -c $1.c -o $1.bc
opt -mem2reg $1.bc -o $1-simp.bc
#opt -load ./LocalOpts.so -my-local-opts $1-simp.bc -o $1-opt.bc
opt -load ./live.so -live $1-simp.bc > /dev/null
opt -load ./reach.so -reach $1-simp.bc > /dev/null
