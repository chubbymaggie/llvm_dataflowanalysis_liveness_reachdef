#clang -c -emit-llvm $1.c
opt -load ./Liveness.so -my-liveness $1.bc > /dev/null
	
