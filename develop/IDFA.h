// CS380C S14 Assignment 3: IDFA.h
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
	/**
	 * data structure for analysis results (both for basicblock and for instructions)
	 */
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

	typedef ValueMap<const BasicBlock *, idfaInfo *> BinfoMap;
	typedef ValueMap<const Instruction *, idfaInfo *> IinfoMap;

	/**
	 * A skeleton for printing the analysis results.
	 * Based on code from Arthur Peters
	 * Modified by Jianyu Huang
	 */
	template<class FlowType>
		class Annotator : public AssemblyAnnotationWriter {
			public:
				BinfoMap &BBtoInfo;		
				IinfoMap &InstToInfo;
				std::vector<FlowType> &domain;
				Annotator<FlowType>(BinfoMap &BI, IinfoMap &II, std::vector<FlowType> &dm): BBtoInfo(BI), InstToInfo(II), domain(dm) {
				}
				virtual void emitBasicBlockStartAnnot(const BasicBlock *bb, formatted_raw_ostream &os) {
					os << "; ";
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
	
	/**
	 * Iterative DataFlow Analysis Framework
	 */
	template<class FlowType>
		class IDFA {
			public:
				BinfoMap BBtoInfo;		
				IinfoMap InstToInfo;
				ValueMap<FlowType, unsigned> domainToIdx;
				IDFA() {}
				/*
				BinfoMap& getBBtoInfo() {
					return this->BBtoInfo;
				}
				IinfoMap& InstToInfo() {
					return this->InstToInfo;
				}
				ValueMap<FlowType, unsigned>& getdomainToIdx() {
					return this->domainToIdx;
				}
				*/
				// core function, Iteration DataFlow Analysis
				void analysis(std::vector<FlowType> domain, Function& F, bool isForward);
				// set the initial flow values for each block with the client defined initFlowValue function
				void setFlowValues(Function &F, bool isForward, int len); 
				//set the Boundary Condition
				void setBoundaryCondition(Function &F, bool isForward);
				//forward analysis of the basic block for the worklist algorithm 
				void preorder(std::vector<BasicBlock *> &Worklist);
				//backward analysis of the basic block for the worklist algorithm 
				void postorder(std::vector<BasicBlock *> &Worklist);
				//worklist algorithm for iterative dataflow analysis
				void WorklistAlg(bool isForward, Function &F);
				//dataflow analysis for instruction level
				void InstAnalysis(Function &F, bool isForward);
				//forward analysis for instruction level
				void preorderInst(Function &F);
				//backward analysis for instruction level
				void postorderInst(Function &F);

				void BVprint(BitVector* BV);

				/**
				 * virtual functions for the interface of subclss
				 */
				//meet operator
				virtual void meetOp(BitVector *op1, BitVector *op2) = 0;
				//transfer functions
				virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill) = 0;
				//get the boundary condition
				virtual BitVector* getBoundaryCondition(int len, Function &F) = 0;
				//get the initial flow values
				virtual BitVector* initFlowValues(int len) = 0;
				//generate the gen and kill set for the instructions inside a basic block
				virtual void initInstGenKill(Instruction *ii) = 0;
				//generate the gen and kill set for each block
				virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi) = 0;
		};


	/**
	 * core function, iteration dataflow analysis		 		
	 * 1. set the initial flow value
	 * 2. set the boundary condition
	 * 3. compute the in and out set for the block (inlucding computing the gen and kill set for the block)
	 * 4. compute the in and out set for the instruction (including computing the gen and kill set for the instruction)
	 */
	template<class FlowType>
	void IDFA<FlowType>::analysis(std::vector<FlowType> domain, Function& F, bool isForward) {
		//map the domain entry to index
		ValueMap<FlowType, unsigned> domainToIdx;
		for (int i = 0; i < domain.size(); ++i) {
			domainToIdx[domain[i]] = i;
		}

		for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
			//init the BBtoInfo
			BBtoInfo[&*Bi] = new idfaInfo(domain.size());
		}
		//set the initial flow values for each block with the client defined initFlowValue function
		setFlowValues(F, isForward, domain.size());
		//set the Boundary Condition


		
		setBoundaryCondition(F, isForward);
		WorklistAlg(isForward, F);
		for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
			errs() << "Worlist after:";
			errs() << "gen:";
			BVprint(BBtoInfo[&*Bi]->gen);
			errs() << "kill:";
			BVprint(BBtoInfo[&*Bi]->kill);
			errs() << "in:";
			BVprint(BBtoInfo[&*Bi]->in);
			errs() << "out:";
			BVprint(BBtoInfo[&*Bi]->out);
		}
		InstAnalysis(F, isForward);
	}


	/**
	 * set the initial flowvalues 
	 */
	template<class FlowType>
	void IDFA<FlowType>::setFlowValues(Function &F, bool isForward, int len) {
		if (isForward) {
			//for forward Interative DataFlow Analysis, init with in[B] = initFlowValue
			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				//init the BBtoInfo
				BBtoInfo[&*Bi]->in = initFlowValues(len);
			}
		} else {
			//for backward Interative DataFlow Analysis, init with out[B] = initFlowValue
			for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
				BBtoInfo[&*Bi]->out = initFlowValues(len);
			}
		}
	}

	/**
	 * set the boundary conditions
	 */
	template<class FlowType>
	void IDFA<FlowType>::setBoundaryCondition(Function &F, bool isForward) {
		if (isForward) {
			//delete first..... release the memory
			//delete BBtoInfo[&(F.front())]->in;
			BBtoInfo[&(F.front())]->in = getBoundaryCondition(domainToIdx.size(), F);
		} else {
			BBtoInfo[&(F.back())]->out = getBoundaryCondition(domainToIdx.size(), F);
		}
	}
	
	/**
	 * forward analysis of the basic block for the worklist algorithm 
	 */
	template<class FlowType>
	void IDFA<FlowType>::postorder(std::vector<BasicBlock *> &Worklist) {
		BasicBlock *Bi = Worklist.back();
		Worklist.pop_back();
		idfaInfo *BBinf = BBtoInfo[&*Bi];
		BitVector *oldOut = new BitVector(*(BBinf->out)); 
		for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
			BasicBlock *Si = *succIt;
			idfaInfo *Sinf = BBtoInfo[Si];
			initGenKill(Si, Bi);

			errs() << "..........................";
			errs() << Si->getName() << Bi->getName() << "\n";
			BVprint(BBtoInfo[&*Si]->gen);
			BVprint(BBtoInfo[&*Si]->kill);
			errs() << "&&&&&&&&&&&&&&&&&&&&&&&&&&&";

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
				BasicBlock *BB = *predIt;
				Worklist.push_back(BB);
			}
		}
	}

	/**
	 * backward analysis of the basic block for the worklist algorithm 
	 */
	template<class FlowType>
	void IDFA<FlowType>::preorder(std::vector<BasicBlock *> &Worklist) {
		BasicBlock *Bi = Worklist.back();
		Worklist.pop_back();
		idfaInfo *BBinf = BBtoInfo[&*Bi];
		BitVector *oldOut = new BitVector(*(BBinf->in));
		for (pred_iterator predIt = pred_begin(Bi), predE = pred_end(Bi); predIt != predE; ++predIt) {
			BasicBlock *Pi = *predIt;
			idfaInfo *Pinf = BBtoInfo[Pi];
			initGenKill(Pi, Bi);
			Pinf->out = transferFunc(Pinf->in, Pinf->gen, Pinf->kill);
			if (predIt == pred_begin(Bi)) {
				*(BBinf->in) = *(Pinf->out);
			} else {
				meetOp(BBinf->in, Pinf->out);
			}
		}
		if (*oldOut != *(BBinf->in)) {
			for (succ_iterator succIt = succ_begin(Bi), succE = succ_end(Bi); succIt != succE; ++succIt) {
				BasicBlock *BB = *succIt;
				Worklist.push_back(BB);
			}
		}
	}

	/**
	 * worklist algorithm for iterative dataflow analysis
	 */
	template<class FlowType>
	void IDFA<FlowType>::WorklistAlg(bool isForward, Function &F) {
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
			if (isForward) 
				preorder(Worklist);
			else 
				postorder(Worklist);
		}	
	}

	/**
	 * dataflow analysis for instruction level
	 */
	template<class FlowType>
	void IDFA<FlowType>::InstAnalysis(Function &F, bool isForward) {
		if (isForward) {
			preorderInst(F);
		} else {
			postorderInst(F);
		}
	}

	/**
	 * forward analysis for instruction level
	 */
	template<class FlowType>
	void IDFA<FlowType>::preorderInst(Function &F) {
		for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
			BasicBlock::iterator ii, ie;
			for (ii = Bi->begin(), ie = Bi->end(); ii != ie; ++ii) {
				InstToInfo[&*ii] = new idfaInfo(domainToIdx.size());
				initInstGenKill(&*ii);
				if (ii == (Bi->begin())) {
					InstToInfo[&*ii]->in = BBtoInfo[&*Bi]->in;
				} else {
					BasicBlock::iterator ij = (--ii);
					++ii;
					InstToInfo[&*ii]->in = InstToInfo[&*(ij)]->out;

				}
				InstToInfo[&*ii]->out = transferFunc(InstToInfo[&*ii]->in, InstToInfo[&*ii]->gen, InstToInfo[&*ii]->kill);
			}
			BBtoInfo[&*Bi]->out = InstToInfo[&*(--ie)]->out;
		}
	}

	/**
	 * backward analysis for instruction level
	 */
	template<class FlowType>
	void IDFA<FlowType>::postorderInst(Function &F) {
		for (Function::iterator Bi = F.begin(), Be = F.end(); Bi != Be; ++Bi) {
			BasicBlock::iterator ii = Bi->end(), is = Bi->begin();
			while (true) {
				--ii;
				InstToInfo[&*ii] = new idfaInfo(domainToIdx.size());
				initInstGenKill(&*ii);
				if (ii == (--(Bi->end()))) {
					InstToInfo[&*ii]->out = BBtoInfo[&*Bi]->out;
				} else {
					BasicBlock::iterator ij = (++ii);
					--ii;
					InstToInfo[&*ii]->out = InstToInfo[&*(ij)]->in;
				}				
				InstToInfo[&*ii]->in = transferFunc(InstToInfo[&*ii]->out, InstToInfo[&*ii]->gen, InstToInfo[&*ii]->kill);	

				if (ii == is) {
					break;
				}
			}
			BBtoInfo[&*Bi]->in = InstToInfo[&*ii]->in;
		}
	}

	template<class FlowType>
	void IDFA<FlowType>::BVprint(BitVector* BV) {
		for (int i = 0; i < (*BV).size(); ++i) {
			errs() << (((*BV)[i])?"1":"0");
		}
		errs() << "\n";
	}

}
