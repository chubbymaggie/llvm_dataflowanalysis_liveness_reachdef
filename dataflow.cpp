// 15-745 S13 Assignment 1: FunctionInfo.cpp
// 
// Based on code from Todd C. Mowry
// Modified by Arthur Peters
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Support/PatternMatch.h"


#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

	class DataFlow {
		public:
			ValueMap<BasicBlock::iterator, BitVector*> *in;
			ValueMap<BasicBlock::iterator, BitVector*> *out;
		DataFlow() {
			in = new ValueMap<BasicBlock::iterator, BitVector*>();
			out = new ValueMap<BasicBlock::iterator, BitVector*>();
		}
	}
}
