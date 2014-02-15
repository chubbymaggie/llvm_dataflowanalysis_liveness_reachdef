// Compilers S14 Assignment 3: assignment3.cpp
//
// Based on code from Todd C. Mowry
// Modified by Arthur Peters
// Modified by Chenhan Yu (EID: yc22547)
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"

#include <ostream>
#include <fstream>
#include <iostream>
#include <deque>
#include <map>

using namespace llvm;

namespace {

#include "ldfaf.txx"
#include "livedfa.txx"
#include "reachdfa.txx"

class Live : public FunctionPass {
public:
  static char ID;
  Live() : FunctionPass(ID) {}

  void parsing(Function *F, LiveDFA<Value> *livedfa, ReachDFA<Value> *reachdfa) {
    /* Only parse block level's in and out set. */
    for (Function::iterator block_I = F->begin(), block_E = F->end();
	      block_I != block_E; ++block_I) {
      BasicBlock *B = dyn_cast<BasicBlock>(block_I);
      DFANODE<Value> *live_node = (*livedfa->blockmap)[B];
      DFANODE<Value> *reach_node = (*reachdfa->blockmap)[B];
      (*live_node->in) &= (*reach_node->in);
      (*live_node->out) &= (*reach_node->out);
      livedfa->blk_anal(B, true);
    }
  }

  virtual bool runOnFunction(Function &F) {
    LiveDFA<Value> *livedfa = new LiveDFA<Value>(false);
	livedfa->fun_anal(&F);

	/* Note that if the live analysis didn't provide minial live set.
	 * We apply reach def analysis to help parse the in and out set.
	 *
	 * ReachDFA<Value> *reachdfa = new ReachDFA<Value>(true);
	 * reachdfa->fun_anal(&F, );
	 * parsing(&F, livedfa, reachdfa);
	 */
    livedfa->report(&F, livedfa->imap);
	//Annotator<Value> annot(livedfa);
    //F.print(errs(), &annot);
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
};

// LLVM uses the address of this static member to identify the pass, so the
// initialization Value is unimportant.
char Live::ID = 0;

// Register this pass to be used by language front ends.
// This allows this pass to be called using the command:
//    clang -c -Xclang -load -Xclang ./FunctionInfo.so loop.c
static void registerMyPass(const PassManagerBuilder &,
                           PassManagerBase &PM) {
  PM.add(new Live());
}

RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                   registerMyPass);

// Register the pass name to allow it to be called with opt:
//    clang -c -emit-llvm loop.c
//    opt -load ./FunctionInfo.so -function-info loop.bc > /dev/null
// See http://llvm.org/releases/3.4/docs/WritingAnLLVMPass.html#running-a-pass-with-opt for more info.
RegisterPass<Live> X("live", "Liveness Avalysis.");

}
