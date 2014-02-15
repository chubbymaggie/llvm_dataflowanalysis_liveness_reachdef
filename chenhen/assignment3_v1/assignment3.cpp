// Compilers S14 Assignment 3: assignment3.cpp
//
// Based on code from Todd C. Mowry
// Modified by Arthur Peters
// Modified by Chenhan Yu (EID: yc22547)
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {

template <class NodeTy>
class WQUEUE {
  public:
	WQUEUE (bool direction) {forward = direction;}
	void enqueue (NodeTy* node) {
      int &x = nodeset[node];
	  if (x == 1) return;
	  x = 1;
	  nodequeue.push_back(node);
	}
	NodeTy *dequeue() {
	  NodeTy *node;
      assert(!nodequeue.empty());
	  if (forward) {
	    node = nodequeue.pop_front_val();
      }
	  else {
	    node = nodequeue.pop_back_val();
	  }
	  nodeset[node] = 0;
	  return node;
	}
	bool isempty () {return nodequeue.empty();}
  private:
	bool forward;
	llvm::DenseMap<NodeTy*, int> nodeset;
	SmallVector<NodeTy*, 20> nodequeue;
};

}

namespace {

template <class DomainTy, class NodeTy>
class IDFAF {
  public:
    IDFAF(DomainTy *domain_in, bool direction);
	virtual void trans_func();
	virtual void meet_oper();
	void analysis() {
      WQUEUE<NodeTy> wqueue(forward);
	  while (1) {
	    bool changed = false;
        /* Enqueue all nodes inside the domain. */
        for (typename iplist<NodeTy>::iterator node_I = domain->begin(), node_E = domain->end();
	        node_I != node_E; ++node_I) {
          NodeTy *node = dyn_cast<NodeTy>(node_I);
	      wqueue.enqueue(node);
        }
        /* Start to dequeue nodes one-by-one. */
        while (!wqueue.isempty()) {
	      NodeTy *node = wqueue.dequeue();
        }
		if (!changed) break;
      }
	};
  private:
    bool forward;
	DomainTy *domain;
};

template <class DomainTy, class NodeTy>
IDFAF<DomainTy, NodeTy>::IDFAF(DomainTy *domain_in, bool direction) {
  domain = domain_in;
  forward = direction;
};

}


namespace {

class Assignment3 : public FunctionPass {
public:
  static char ID;
  Assignment3() : FunctionPass(ID) {}

  bool iter_dataflow_analysis(Function &F);

  virtual bool runOnFunction(Function &F) {
    for (Function::iterator block_I = F.begin(), block_E = F.end(); block_I != block_E; ++block_I) {
      BasicBlock *B = dyn_cast<BasicBlock>(block_I);
    }
  }

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
};
/*
bool Assignment3::iter_data_flow_analysis(Function &F) {

  return true;
}
*/
// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char Assignment3::ID = 0;

// Register this pass to be used by language front ends.
// This allows this pass to be called using the command:
//    clang -c -Xclang -load -Xclang ./FunctionInfo.so loop.c
static void registerMyPass(const PassManagerBuilder &,
                           PassManagerBase &PM) {
    PM.add(new Assignment3());
}
RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                   registerMyPass);

// Register the pass name to allow it to be called with opt:
//    clang -c -emit-llvm loop.c
//    opt -load ./FunctionInfo.so -function-info loop.bc > /dev/null
// See http://llvm.org/releases/3.4/docs/WritingAnLLVMPass.html#running-a-pass-with-opt for more info.
RegisterPass<Assignment3> X("assignment3", "Function Information");

}
