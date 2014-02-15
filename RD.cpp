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
				os << "; ";
				BitVector &bv = *(InstToInfo[i]->in);
				for (unsigned i = 0; i < bv.size(); ++i) {
					if (bv[i]) {
						os << domain[i]->getName() << ", ";
					}
				}
				os << "\n";
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
				DataFlow<Value *>::analysis(domain, F, false, BBtoInfo, InstToInfo);

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
				InstInfo *instInf = InstToInfo[ii];
				//what if  v = v + 1 case.....
				User::op_iterator OI, OE;
				for (OI = ii->op_begin(), OE = ii->op_end(); OI != OE; ++OI) {
					
					Value *val = *OI;
					if (isa<Instruction>(val) || isa<Argument>(val)) {

						int valIdx = domainToIdx[val];
						(instInf->gen)->set(valIdx);
					}
				}
				ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
				if (iter != domainToIdx.end()) {
					Value *val = ii;
					int valIdx = domainToIdx[val];
					(instInf->kill)->set(valIdx);
				}
			}
			virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, BasicBlockInfo *> &BBtoInfo) {
				BasicBlockInfo *BBinf = BBtoInfo[&*Bi];
				(BBinf->gen)->reset(0, domainToIdx.size());
				(BBinf->kill)->reset(0, domainToIdx.size());	
				for (BasicBlock::iterator ii = Bi->begin(), ie = Bi->end(); ii != ie; ++ii) {
					if(isa<PHINode>(ii)){
						PHINode *pN = dyn_cast<PHINode>(&*ii);
						unsigned idx = pN->getBasicBlockIndex(Pi);

						if (idx >= 0 && idx < pN->getNumIncomingValues()) {
							Value *val = pN->getIncomingValue(idx);

							if (isa<Instruction>(val) || isa<Argument>(val)) {
								unsigned valIdx = domainToIdx[val];
								if (!((*(BBinf->kill))[valIdx])) {
									(BBinf->gen)->set(valIdx);
								}
							}
						}
						//duplicated code..............I will remove it later.............
						ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
						if (iter != domainToIdx.end()) {
							Value *val = ii;
							int valIdx = domainToIdx[val];
							if (!((*(BBinf->gen))[valIdx])) {
								(BBinf->kill)->set(valIdx);
							}
						}
					} else {
						User::op_iterator OI, OE;
						for (OI = ii->op_begin(), OE = ii->op_end(); OI != OE; ++OI) {
							Value *val = *OI;
							if (isa<Instruction>(val) || isa<Argument>(val)) {
								// val is used by insn
								//we need to judge whether valIdx is valid or not......if valIdx == -1.....No...just need to judge whether val exists in domainIdx......see below...
								//.........How to handle....................................int valIdx = domainToIdx[dyn_cast<instruction>(val)].................
								int valIdx = domainToIdx[val];

								//Assume that v = v op x will never exist in SSA form
								//BasicBlock *BB = &*Bi;
								BitVector tmp = *(BBinf->kill);
								if (!((tmp)[valIdx])) {
									(BBinf->gen)->set(valIdx);
								}
							}

						}
						ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
						if (iter != domainToIdx.end()) {
							Value *val = ii;
							//.........How to handle....................................int valIdx = domainToIdx[dyn_cast<instruction>(val)].................
							int valIdx = domainToIdx[val];
							BitVector tmp = *(BBinf->gen);
							if (!((tmp)[valIdx])) {
								(BBinf->kill)->set(valIdx);
							}
						}

					}
				}
			}
			virtual BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) {
				BitVector *output = new BitVector(len, false);
				for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
					Instruction *i = *arg;
					output.set(domainToIdx[i]);
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
