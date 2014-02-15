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


#include "llvm/IR/BasicBlock.h"


#include "llvm/Support/InstIterator.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Support/PatternMatch.h"

#include "llvm/Assembly/AssemblyAnnotationWriter.h"


#include "IDFA.cpp"


#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {
	class Annotator : public AssemblyAnnotationWriter {
		public:
			ValueMap<const BasicBlock *, BasicBlockInfo *> &BBtoInfo;		
			ValueMap<const Instruction *, InstInfo *> &InstToInfo;
			std::vector<Value*> &domain;
			Annotator(ValueMap<const BasicBlock *, BasicBlockInfo *> &BI, ValueMap<const Instruction *, InstInfo *> &II, std::vector<Value*> &dm): BBtoInfo(BI), InstToInfo(II), domain(dm) {
			}

			virtual void emitBasicBlockStartAnnot(const BasicBlock *bb, formatted_raw_ostream &os) {
				os << "; ";
				//BasicBlock *bi = *bb;
				BitVector &bv = *(BBtoInfo[bb]->in);
				for (unsigned i = 0; i < bv.size(); ++i) {
					if (bv[i]) {
						os << domain[i]->getName() << ", ";
					}
				}
				os << "\n";
			}

			virtual void emitInstructionAnnot(const Instruction *i, formatted_raw_ostream &os) {
				if (!isa<PHINode>(i)) {
					os << "; ";
					BitVector &bv = *(InstToInfo[i]->in);
					for (unsigned i = 0; i < bv.size(); ++i) {
						if (bv[i]) {
							os << domain[i]->getName() << ", ";
						}
					}
					os << "\n";
				}
			}
	};

	class ReachDef : public DataFlow<llvm::Value *>, public FunctionPass {
		public:
			static char ID;
			ReachDef() : DataFlow<llvm::Value *>(), FunctionPass(ID) {

			}

			virtual bool runOnFunction(Function &F) {
				std::vector<Value *> domain;
				for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
					domain.push_back(arg);
				}
				for (inst_iterator ii = inst_begin(F), ie = inst_end(F); ii != ie; ++ii) {
					if (!ii->getName().empty()) {
						domain.push_back(&*ii);
					}
				}
				//debugging.....
				errs() << "Domain: ";
				for (int i = 0; i < domain.size(); ++i) {
					errs() << domain[i]->getName();
					errs() << ", ";
				}
				errs() << "\n";



				ValueMap<const BasicBlock *, BasicBlockInfo *> BBtoInfo;		
				ValueMap<const Instruction *, InstInfo *> InstToInfo;

				//DataFlow<Value *>::analysis(domain, F, false);
				DataFlow<Value *>::analysis(domain, F, true, BBtoInfo, InstToInfo);

				Annotator annot(BBtoInfo, InstToInfo, domain);
				F.print(errs(), &annot);


				return false;
			}


			//this function may need to be changed, since the phi should be handled in meetOp....
			virtual void meetOp(BitVector *op1, BitVector *op2) {
				(*op1) |= (*op2);
			}

			virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill) {
				BitVector* output = new BitVector(*kill);
				output->flip();
				(*output) &= *input;
				(*output) |= *gen;
				return output;
			}
			
			virtual void initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, InstInfo *> &InstToInfo) {
				//do we need to delete the following condition judgement?
				if (!isa<PHINode>(ii)) {
					InstInfo *instInf = InstToInfo[ii];

					ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
					if (iter != domainToIdx.end()) {
						Value *val = ii;
						int valIdx = domainToIdx[val];
						(instInf->gen)->set(valIdx);
					}
			    }

			}

			virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, BasicBlockInfo *> &BBtoInfo) {
				BasicBlockInfo *BBinf = BBtoInfo[&*Bi];
				(BBinf->gen)->reset(0, domainToIdx.size());
				(BBinf->kill)->reset(0, domainToIdx.size());	
				for (BasicBlock::iterator ii = Bi->begin(), ie = Bi->end(); ii != ie; ++ii) {
					//??????????do we need to delete the following contition judgement?
					if(!isa<PHINode>(ii)){
						ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
						if (iter != domainToIdx.end()) {
							Value *val = ii;
							//.........How to handle....................................int valIdx = domainToIdx[dyn_cast<instruction>(val)].................
							int valIdx = domainToIdx[val];
							//BitVector tmp = *(BBinf->kill);
							//if (!((tmp)[valIdx])) {
								(BBinf->gen)->set(valIdx);
							//}
						}
					}

				}
			}

			virtual BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) {
				BitVector *output = new BitVector(len, false);
				for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
					//Instruction *i = *arg;
					output->set(domainToIdx[&*arg]);
				}
				return output;
			}


			virtual BitVector* initFlowValues(int len) {
				return new BitVector(len, false);
			}

			// We don't modify the program, so we preserve all analyses
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.setPreservesAll();
			}


	};

	// LLVM uses the address of this static member to identify the pass, so the
	// initialization value is unimportant.
	char ReachDef::ID = 0;

	// Register this pass to be used by language front ends.
	// This allows this pass to be called using the command:
	//    clang -c -Xclang -load -Xclang ./FunctionInfo.so loop.c
	static void registerMyPass(const PassManagerBuilder &,
			PassManagerBase &PM) {
		PM.add(new ReachDef());
	}
	RegisterStandardPasses
		RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
				registerMyPass);

	// Register the pass name to allow it to be called with opt:
	//    clang -c -emit-llvm loop.c
	//    opt -load ./FunctionInfo.so -function-info loop.bc > /dev/null
	// See http://llvm.org/releases/3.4/docs/WritingAnLLVMPass.html#running-a-pass-with-opt for more info.
	RegisterPass<ReachDef> X("my-reachdef", "my-reachdef");

}
