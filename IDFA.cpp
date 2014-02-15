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
//namespace llvm {}
	class BasicBlockInfo {
		public:
			BitVector *gen;
			BitVector *kill;
			BitVector *in;
			BitVector *out;
			BasicBlock *BB;
			BasicBlockInfo(BasicBlock *BB, unsigned len) {
				this->BB = BB;
				gen = new BitVector(len);
				kill = new BitVector(len);
				in = new BitVector(len);
				out = new BitVector(len);
			}
	};

	class InstInfo {
		public:
			BitVector *gen;
			BitVector *kill;
			BitVector *in;
			BitVector *out;
			Instruction *inst;
			InstInfo(Instruction *ii, unsigned len) {
				this->inst = ii;
				gen = new BitVector(len);
				kill = new BitVector(len);
				in = new BitVector(len);
				out = new BitVector(len);
			}
	};


	template<class T>
	class DataFlow {
		public:
			//ValueMap<T, unsigned> DomainMap...
			//ValueMap<BasicBlock *, BasicBlockInfo> BBMap...

/*

		T a;
		*/
		DataFlow() {
		}
	
		/*
		DataFlow(std::vector<T> domain, bool forward) {

		}
		*/

		void analysis(std::vector<T> domain, Function& F, bool isForward, ValueMap<const BasicBlock *, BasicBlockInfo *> &BBtoInfo, ValueMap<const Instruction *, InstInfo *> &InstToInfo) {
			//map the domain entry to index
			ValueMap<T, unsigned> domainToIdx;
			for (int i = 0; i < domain.size(); ++i) {
				domainToIdx[domain[i]] = i;
			}

			//Boundary Condition, and the top....
			//We don't need to calc top, since we have guarantee the meetOp....top meet any = any....

			//ValueMap<BasicBlock *, BasicBlockInfo *> BBtoInfo;
			//...........generate the BBtoInfo............
			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				//init the BBtoInfo
				BBtoInfo[&*Bi] = new BasicBlockInfo(&*Bi, domain.size());
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
				errs() << "shangxin" <<  F.back().getName() << "\n";
				//change to out...................haven't check...
				BBtoInfo[&(F.back())]->out = getBoundaryCondition(domain.size(), F, domainToIdx);
			} else {
				BBtoInfo[&(F.front())]->in = getBoundaryCondition(domain.size(), F, domainToIdx);
			}

			/*
			//init the flow map
			std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector *> flowFilter, flowReserve;
			initFlowFilter(F, flowFilter, flowReserve, domainToIdx);

			//debug....
			errs() << "print flowFilter\n";
			std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector *>::iterator piter, piterE;

			errs() << "flag1" << "\n";
			for (piter = flowFilter.begin(), piterE = flowFilter.end(); piter != piterE; ++piter) {
				errs() << (piter->first).first->getName() << "->" << (piter->first).second->getName() << "\n";
				BVprint(piter->second);
			}
			errs() << "flag2" << "\n";
			for (piter = flowReserve.begin(), piterE = flowReserve.end(); piter != piterE; ++piter) {
				errs() << (piter->first).first->getName() << "->" << (piter->first).second->getName() << "\n";
				BVprint(piter->second);
			}
			errs() << "flag3" << "\n";
			*/

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
			while (!Worklist.empty()) {
				BasicBlock *Bi = Worklist.back();
				Worklist.pop_back();
				BasicBlockInfo *BBinf = BBtoInfo[&*Bi];
				//BitVector * &bin = (isForward) ? BBinf->in : BBinf->out;
				//BitVector * &bout = (isForward) ? BBinf->out : BBinf->in;

				if (isForward) {

				} else {
					BitVector *oldOut = new BitVector(*(BBinf->out)); 
					
					for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
						BasicBlock *Si = *succIt;
						BasicBlockInfo *Sinf = BBtoInfo[Si];
						initGenKill(Si, Bi, domainToIdx, BBtoInfo);

						/*
						errs() << "Gen && Kill Set\n";
						errs() << Bi->getName() << "->" << Si->getName() << "\n";
						errs() << "gen:";
						BVprint(BBtoInfo[&*Si]->gen);
						errs() << "S-kill:";
						BVprint(BBtoInfo[&*Si]->kill);
						errs() << "-------------------------------------------------------\n";
						*/
						
						//Sinf->in is always a new value, pointer redirect...so we don't need to reset Sinf->in
						//delete Sinf->in;
						Sinf->in = transferFunc(Sinf->out, Sinf->gen, Sinf->kill);
						//errs() << "S-in:";
						//BVprint(BBtoInfo[&*Si]->in);
						//errs() << "-------------------------------------------------------\n";
	
						if (succIt == succ_begin(Bi)) {
							*(BBinf->out) = *(Sinf->in);
						} else {
							meetOp(BBinf->out, Sinf->in);	
						}
						/*
						errs() << "-------------------------------------------------------\n";
						errs() << Bi->getName() << "->" << Si->getName() << "\n";
						errs() << "in:";
						BVprint(BBtoInfo[&*Bi]->in);
						errs() << "out:";
						BVprint(BBtoInfo[&*Bi]->out);
						errs() << "-------------------------------------------------------\n";
						*/

					}

					/*
					errs() << "new worklist elem\n";
					errs() << "-------------------------------------------------------\n";
					errs() << "BB name:" << Bi->getName() << "\n";
					errs() << "Oldout:";
					BVprint(oldOut);
					errs() << "out:";
					BVprint(BBtoInfo[&*Bi]->out);
					errs() << "********************************************************\n";
					*/
			
					if (*oldOut != *(BBinf->out)) {
						errs() << "isChanged!" << "\n";
						for (pred_iterator predIt = pred_begin(Bi), predE = pred_end(Bi); predIt != predE; ++predIt) {
							//if (std::find(Worklist.begin(), Worklist.end(), Bi) == Worklist.end()) {
							BasicBlock *BB = *predIt;
							Worklist.push_back(BB);
							//}
						}
					}


				}
				/*
				errs() << "BB name:" << Bi->getName() << "\n";
				errs() << "in:";
				BVprint(BBtoInfo[&*Bi]->in);
				errs() << "out:";
				BVprint(BBtoInfo[&*Bi]->out);
				*/
			}

			errs() << "output:............\n";

			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				errs() << "BB name.....:" << Bi->getName() << "\n";
				BVprint(BBtoInfo[&*Bi]->out);
			}

			//ValueMap<Instruction *, InstInfo *> InstToInfo;
			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {

				errs() << "BB name.^^^^^^^:" << Bi->getName() << "\n";
				BasicBlock::iterator ii = Bi->end(), is = Bi->begin();
				//InstInfo *curInstInfo;
				while (true) {
					--ii;
					InstToInfo[&*ii] = new InstInfo(&*ii, domain.size());
					//curInstInfo = new InstInfo(&*ii, domain.size());
					initInstGenKill(&*ii, domainToIdx, InstToInfo);

					if (ii == (--(Bi->end()))) {
						//two pointer point to the same variable.....
						//curInstInfo->out = BBtoInfo[&*Bi]->out;
						InstToInfo[&*ii]->out = BBtoInfo[&*Bi]->out;
					} else {
						//curInstInfo->out = InstToInfo[&*(--ii)]->in;
						//++ii;
						BasicBlock::iterator ij = (++ii);
						--ii;
						InstToInfo[&*ii]->out = InstToInfo[&*(ij)]->in;
						
					}				
					//InstToInfo[&*ii]->in = transferFunc(curInstInfo->out, curInstInfo->gen, curInstInfo->kill);	
					InstToInfo[&*ii]->in = transferFunc(InstToInfo[&*ii]->out, InstToInfo[&*ii]->gen, InstToInfo[&*ii]->kill);	
					//errs() << *ii << "----";
					//BVprint(InstToInfo[&*ii]->out);

					if (ii == is) {
						break;
					}
				}
				//two pointer point to the same variable.....
				BBtoInfo[&*Bi]->in = InstToInfo[&*ii]->in;
				BVprint(BBtoInfo[&*Bi]->in);
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


		//flowmap
		void initFlowFilter(Function &F, std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector *> &flowFilter, std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector *> &flowReserve, ValueMap<T, unsigned> &domainToIdx) {
			errs() << "enter the flow function\n";
			for (inst_iterator ii = inst_begin(F), ie = inst_end(F); ii != ie; ++ii) {
				if (isa<PHINode>(&*ii)) {
				errs() << *ii << "*****************\n";
					PHINode *pN = dyn_cast<PHINode>(&*ii);
					BasicBlock *pBB = pN->getParent();
					unsigned num = pN->getNumIncomingValues();
					for (unsigned i = 0; i < num; ++i) {
						/*
						Value *curVal = pN->getIncomingValue();

						if (!(isa<Instruction>(curVal) || isa<Instruction>(curVal))) {
						}
						*/

						BasicBlock *jmpBB = pN->getIncomingBlock(i);
						std::pair<BasicBlock*, BasicBlock*> BBkey = std::make_pair(jmpBB, pBB);
						//std::map<std::pair<BasicBlock*, BasicBlock*>, BitVector*>::iter

						if (flowFilter.find(BBkey) == flowFilter.end()) {
							flowFilter[BBkey] = new BitVector(domainToIdx.size(), true);
							flowReserve[BBkey] = new BitVector(domainToIdx.size(), false);
						}
						Value *curVal = pN->getIncomingValue(i);
						if (domainToIdx.find(curVal) != domainToIdx.end()) {
							flowReserve[BBkey]->set(domainToIdx[curVal]);
						}
						
						for (unsigned j = 0; j < num; j++) {
							if (i != j) {
								if (domainToIdx.find(pN->getIncomingValue(j)) != domainToIdx.end()) {
									flowFilter[BBkey]->reset(domainToIdx[pN->getIncomingValue(j)]);
								}
							}
						}
					}

				}
			}
		}

		virtual void initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, InstInfo *> &InstToInfo) = 0;
		virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, BasicBlockInfo *> &BBtoInfo) = 0;
		virtual void meetOp(BitVector *op1, BitVector *op2) = 0;

		virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill) = 0;

		//Notice: BoundatryCondition for the Reaching Def may vary...argu -> true; instruction -> false....initially
		virtual BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) = 0;
		virtual BitVector* initFlowValues(int len) = 0;
	};
}
