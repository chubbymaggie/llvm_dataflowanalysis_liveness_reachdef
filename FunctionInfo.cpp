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
//#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/CallSite.h"

#include "llvm/DebugInfo.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/FormattedStream.h"



#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {

	class FunctionInfo : public FunctionPass {
		public:
			static char ID;
			FunctionInfo() : FunctionPass(ID) {}

			virtual bool runOnFunction(Function &F) {
				int argNum = 0;
				for (llvm::Function::arg_iterator AI = F.arg_begin(); AI != F.arg_end(); ++AI) {
					++argNum;
				}	
				int instNum = 0;
				for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
					//errs() << *I << "\n";
					++instNum;
				}

				/*
				for (Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
					instNum += BB->size();
				}
				*/
				int callsitesNum = 0;
				for (Value::use_iterator U = F.use_begin(); U != F.use_end(); ++U) {
					if (Instruction *use = dyn_cast<Instruction>(*U)) {
						CallSite CS(use);
						if (CS.getCalledValue() == &F) {
							callsitesNum++;
						}
					}
				}
				errs() << F.getName() << ": arguments=" << argNum << ", call sites=" << callsitesNum << ", basic blocks=" << F.size() << ", instructions=" << instNum << "\n";

				return false; 
			} // We don't modify the program, so we preserve all analyses

			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.setPreservesAll();
			}
	};

	// LLVM uses the address of this static member to identify the pass, so the
	// initialization value is unimportant.
	char FunctionInfo::ID = 0;

	// Register this pass to be used by language front ends.
	// This allows this pass to be called using the command:
	//    clang -c -Xclang -load -Xclang ./FunctionInfo.so loop.c
	static void registerMyPass(const PassManagerBuilder &,
			PassManagerBase &PM) {
		PM.add(new FunctionInfo());
	}
	RegisterStandardPasses
		RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
				registerMyPass);

	// Register the pass name to allow it to be called with opt:
	//    clang -c -emit-llvm loop.c
	//    opt -load ./FunctionInfo.so -function-info loop.bc > /dev/null
	// See http://llvm.org/releases/3.4/docs/WritingAnLLVMPass.html#running-a-pass-with-opt for more info.
	RegisterPass<FunctionInfo> X("function-info", "Function Information");

}
