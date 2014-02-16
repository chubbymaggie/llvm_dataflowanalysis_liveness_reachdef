// 15-745 S13 Assignment 1: FunctionInfo.cpp
// 
// Based on code from Todd C. Mowry
// Modified by Arthur Peters
// Modified by Jianyu Huang(UT EID: jh57266)
////////////////////////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Support/CFG.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/FormattedStream.h"

#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace {
	class idfaInfo {
		public:
			BitVector *gen;
			BitVector *kill;
			BitVector *in;
			BitVector *out;
			idfaInfo(unsigned len) {
				gen = new BitVector(len);
				kill = new BitVector(len);
				in = new BitVector(len);
				out = new BitVector(len);
			}
	};

	template<class T>
		class DataFlow {
			public:
			DataFlow() {
				}
				/*
				   DataFlow(std::vector<T> domain, bool forward) {

				   }
				   */
				void analysis(std::vector<T> domain, Function& F, bool isForward, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo, ValueMap<const Instruction *, idfaInfo *> &InstToInfo) {
					//map the domain entry to index
					ValueMap<T, unsigned> domainToIdx;
					for (int i = 0; i < domain.size(); ++i) {
						domainToIdx[domain[i]] = i;
					}

					//Boundary Condition, and the top....
					//ValueMap<BasicBlock *, idfaInfo *> BBtoInfo;
					//...........generate the BBtoInfo............
					for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
						//init the BBtoInfo
						BBtoInfo[&*Bi] = new idfaInfo(domain.size());
					}
					if (!isForward) {
						for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
							//init the BBtoInfo
							BBtoInfo[&*Bi]->in = initFlowValues(domain.size());
						}
					} else {
						for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
							//init the BBtoInfo
							BBtoInfo[&*Bi]->out = initFlowValues(domain.size());
						}
					}
					if (!isForward) {
						//change to out...................haven't check...
						BBtoInfo[&(F.back())]->out = getBoundaryCondition(domain.size(), F, domainToIdx);
					} else {
						//delete first..... release the memory
						BBtoInfo[&(F.front())]->in = getBoundaryCondition(domain.size(), F, domainToIdx);
					}
					BVprint(BBtoInfo[&(F.front())]->in);

					std::vector<BasicBlock *> Worklist;
					if (isForward) {
						Function::iterator Bi = F.end(), Bs = F.begin();
						while (true) {
							-- Bi;
							Worklist.push_back(&*Bi);
							if (Bi == Bs) {
								break;
							}
						}
					} else {
						for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
							Worklist.push_back(&*Bi);
						}
					}

					if (isForward) {
						while (!Worklist.empty()) {
							BasicBlock *Bi = Worklist.back();
							Worklist.pop_back();
							idfaInfo *BBinf = BBtoInfo[&*Bi];
							//BitVector * &bin = (isForward) ? BBinf->in : BBinf->out;
							//BitVector * &bout = (isForward) ? BBinf->out : BBinf->in;


							BitVector *oldOut = new BitVector(*(BBinf->in));
							for (pred_iterator predIt = pred_begin(Bi), predE = pred_end(Bi); predIt != predE; ++predIt) {
								BasicBlock *Pi = *predIt;
								idfaInfo *Pinf = BBtoInfo[Pi];
								initGenKill(Pi, Bi, domainToIdx, BBtoInfo);

								Pinf->out = transferFunc(Pinf->in, Pinf->gen, Pinf->kill);
								if (predIt == pred_begin(Bi)) {
									*(BBinf->in) = *(Pinf->out);
								} else {
									meetOp(BBinf->in, Pinf->out);
								}

							}

							if (*oldOut != *(BBinf->in)) {
								for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
									//if (std::find(Worklist.begin(), Worklist.end(), Bi) == Worklist.end()) {
									BasicBlock *BB = *succIt;
									Worklist.push_back(BB);
									//}
								}
							}
						}

						//ValueMap<Instruction *, idfaInfo *> InstToInfo;
						for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
							BasicBlock::iterator ii, ie;
							for (ii = Bi->begin(), ie = Bi->end(); ii != ie; ++ii) {
								InstToInfo[&*ii] = new idfaInfo(domain.size());
								initInstGenKill(&*ii, domainToIdx, InstToInfo);
								if (ii == (Bi->begin())) {
									InstToInfo[&*ii]->in = BBtoInfo[&*Bi]->in;
								} else {
									BasicBlock::iterator ij = (--ii);
									++ii;
									InstToInfo[&*ii]->in = InstToInfo[&*(ij)]->out;

								}
								InstToInfo[&*ii]->out = transferFunc(InstToInfo[&*ii]->in, InstToInfo[&*ii]->gen, InstToInfo[&*ii]->kill);
							}

							//two pointer point to the same variable.....
							BBtoInfo[&*Bi]->out = InstToInfo[&*(--ie)]->out;

							BVprint(BBtoInfo[&*Bi]->out);
						}
					} else {

						while (!Worklist.empty()) {
							BasicBlock *Bi = Worklist.back();
							Worklist.pop_back();
							idfaInfo *BBinf = BBtoInfo[&*Bi];
							//BitVector * &bin = (isForward) ? BBinf->in : BBinf->out;
							//BitVector * &bout = (isForward) ? BBinf->out : BBinf->in;


							BitVector *oldOut = new BitVector(*(BBinf->out)); 

							for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
								BasicBlock *Si = *succIt;
								idfaInfo *Sinf = BBtoInfo[Si];
								initGenKill(Si, Bi, domainToIdx, BBtoInfo);


								//Sinf->in is always a new value, pointer redirect...so we don't need to reset Sinf->in
								//delete Sinf->in;
								Sinf->in = transferFunc(Sinf->out, Sinf->gen, Sinf->kill);
								if (succIt == succ_begin(Bi)) {
									*(BBinf->out) = *(Sinf->in);
								} else {
									meetOp(BBinf->out, Sinf->in);	
								}
							}

							if (*oldOut != *(BBinf->out)) {
								for (pred_iterator predIt = pred_begin(Bi), predE = pred_end(Bi); predIt != predE; ++predIt) {
									//if (std::find(Worklist.begin(), Worklist.end(), Bi) == Worklist.end()) {
									BasicBlock *BB = *predIt;
									Worklist.push_back(BB);
									//}
								}
							}
						}

						for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
							BasicBlock::iterator ii = Bi->end(), is = Bi->begin();
							//idfaInfo *curidfaInfo;
							while (true) {
								--ii;
								InstToInfo[&*ii] = new idfaInfo(domain.size());
								initInstGenKill(&*ii, domainToIdx, InstToInfo);

								if (ii == (--(Bi->end()))) {
									//two pointer point to the same variable.....
									//curidfaInfo->out = BBtoInfo[&*Bi]->out;
									InstToInfo[&*ii]->out = BBtoInfo[&*Bi]->out;
								} else {
									//curidfaInfo->out = InstToInfo[&*(--ii)]->in;
									BasicBlock::iterator ij = (++ii);
									--ii;
									InstToInfo[&*ii]->out = InstToInfo[&*(ij)]->in;

								}				
								InstToInfo[&*ii]->in = transferFunc(InstToInfo[&*ii]->out, InstToInfo[&*ii]->gen, InstToInfo[&*ii]->kill);	

								if (ii == is) {
									break;
								}
							}
							//two pointer point to the same variable.....
							BBtoInfo[&*Bi]->in = InstToInfo[&*ii]->in;
							BVprint(BBtoInfo[&*Bi]->in);
						}


					}

					//Annotator annot(BBtoInfo, InstToInfo, domainToIdx);
					//F.print(errs(), &annot);
				}
				//print the Bitvector
				void BVprint(BitVector* BV) {
					for (int i = 0; i < (*BV).size(); ++i) {
						errs() << (((*BV)[i])?"1":"0");
					}
					errs() << "\n";
				}

				virtual void initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, idfaInfo *> &InstToInfo) = 0;
				virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo) = 0;
				virtual void meetOp(BitVector *op1, BitVector *op2) = 0;

				virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill) = 0;

				//Notice: BoundatryCondition for the Reaching Def may vary...argu -> true; instruction -> false....initially
				virtual BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) = 0;
				virtual BitVector* initFlowValues(int len) = 0;
		};
}
