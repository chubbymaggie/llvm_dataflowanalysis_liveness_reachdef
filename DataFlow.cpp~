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

		void analysis(std::vector<T> domain, Function& F, bool isForward) {
			//Gen, Kill
			//Block in, out
			//Instruction in, out

			//Domain????
			//how many blocks....each block has a BasicBlockInfo class...
			//map the domain entry to index
			ValueMap<T, unsigned> domainToIdx;
			for (int i = 0; i < domain.size(); ++i) {
				domainToIdx[domain[i]] = i;
			}

			//Boundary Condition, and the top....
			//We don't need to calc top, since we have guarantee the meetOp....top meet any = any....

			ValueMap<BasicBlock *, BasicBlockInfo *> BBtoInfo;
			//...........generate the BBtoInfo............
			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				//init the BBtoInfo
				BBtoInfo[&*Bi] = new BasicBlockInfo(&*Bi, domain.size());
			}

			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				//init the BBtoInfo
				BBtoInfo[&*Bi]->in = initFlowValues(domain.size());
			}

			BBtoInfo[&(F.back())]->in = getBoundaryCondition(domain.size());

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


			//init the gen and kill set
			//GenSet;
			//KillSet;
			int len = domain.size();
			initGenKill(len, F, domainToIdx, BBtoInfo);

			//for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {}


			//first implement with the ischanged flag, then transform it into the WorkList form...Note: 1. set or list 2. bfs, guarantee to traverse all the node from bottom to top at least once.
			bool isChanged = true;
			while (isChanged) {
				isChanged = false;
				errs() << "new round:" << "\n";
				for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {

					//errs() << "flag6:" << Bi->getName() << "\n";

					BasicBlockInfo *BBinf = BBtoInfo[&*Bi];
					BitVector * &bin = (isForward) ? BBinf->in : BBinf->out;
					BitVector * &bout = (isForward) ? BBinf->out : BBinf->in;

					// for liveness: out[B] = U(in[B], mask[B][Succ]
					//first, we don't consider the mask...
					//we don't consider out and in combination yet....
					//ValueMap<

					//errs() << "flag7:" << Bi->getName() << "\n";
					std::vector<BitVector *> meetInput;
					for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
						BasicBlock *BB = *succIt;

						//errs() << "flag8:" << Bi->getName();
						//errs() << "-------flag9:" << BB->getName() << "\n";
						
						std::pair<BasicBlock*, BasicBlock*> BBkey = std::make_pair(&*Bi, BB);
						BitVector *itemin = new BitVector(*(BBtoInfo[BB]->in));
						//no release memory...ugly programming style
						/*
						errs() << "itemin:";
						BVprint(itemin);
						errs() << "filter:";
						BVprint(flowFilter[BBkey]);
						errs() << "reserve:";
						BVprint(flowReserve[BBkey]);
						*/

						if (flowFilter.find(BBkey) != flowFilter.end()) {
							(*itemin) &= (*(flowFilter[BBkey]));
						}
						if (flowReserve.find(BBkey) != flowReserve.end()) {
							(*itemin) |= (*(flowReserve[BBkey]));
						}

						meetInput.push_back(itemin);
						//meetInput.push_back(BBtoInfo[BB]->in);
					}
					//delete BBtoInfo[&*Bi]->out???;
					if (!meetInput.empty()) {
						//BBtoInfo[&*Bi]->out = meetOp(meetInput);
						bin = meetOp(meetInput);
					}

					//BitVector *oldOut = BBtoInfo[&*Bi]->in;
					BitVector *oldOut = (isForward) ? BBinf->out : BBinf->in;

					//delete BBtoInfo[&*Bi]->in???;
					//BBtoInfo[&*Bi]->in = transferFunc(BBtoInfo[&*Bi]->out, BBtoInfo[&*Bi]->gen, BBtoInfo[&*Bi]->kill); 
					bout = transferFunc(bin, BBinf->gen, BBinf->kill); 

					//errs() << "oldin2:";
					//BVprint(oldOut);
					//errs() << "newin:";
					//BVprint(BBtoInfo[&*Bi]->in);

					if (!isChanged && *oldOut != *(BBtoInfo[&*Bi]->in)) {
						errs() << "isChanged!" << "\n";
						isChanged = true;
					}

					errs() << "BB name:" << Bi->getName() << "\n";
					errs() << "in:";
					BVprint(BBtoInfo[&*Bi]->in);
					errs() << "out:";
					BVprint(BBtoInfo[&*Bi]->out);
				}
			}
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


		virtual void initGenKill(int& len, Function &F, ValueMap<T, unsigned> &domainToIdx, ValueMap<BasicBlock *, BasicBlockInfo *>  &BBtoInfo) = 0;
		virtual BitVector* meetOp(std::vector<BitVector *> input) = 0;

		virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill) = 0;

		//Notice: BoundatryCondition for the Reaching Def may vary...argu -> true; instruction -> false....initially
		virtual BitVector* getBoundaryCondition(int len) = 0;
		virtual BitVector* initFlowValues(int len) = 0;
	};
}
