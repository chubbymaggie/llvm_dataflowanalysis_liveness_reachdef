// CS380C S14 Assignment 3: liveAnalysis.h
// 
// Based on code from Todd C. Mowry
// Modified by Arthur Peters
// Modified by Jianyu Huang (UT EID: jh57266)
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
#include "llvm/Assembly/AssemblyAnnotationWriter.h"

#include "IDFA.h"

#include <ostream>
#include <fstream>
#include <iostream>


using namespace llvm;

namespace {
	template <class FlowType>
	class LiveAnalysis: public IDFA<FlowType> {
		public:
		LiveAnalysis() : IDFA<FlowType>() {}
		//meet operator
		virtual void meetOp(BitVector *op1, BitVector *op2);
		//transfer functions
		virtual BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill);
		//generate the gen and kill set for the instructions inside a basic block
		virtual void initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, idfaInfo *> &InstToInfo);
		//generate the gen and kill set for each block
		virtual void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo);
		//get the boundary condition
		virtual BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx);
		//get the initial flow values
		virtual BitVector* initFlowValues(int len);
	};

	/**
	 * meet operator
	 */
	template <class FlowType>
	void LiveAnalysis<FlowType>::meetOp(BitVector *op1, BitVector *op2) {
		(*op1) |= (*op2);
	}

	/**
	 * transfer functions
	 */
	template <class FlowType>
	BitVector* LiveAnalysis<FlowType>::transferFunc(BitVector *input, BitVector *gen, BitVector *kill) {
		BitVector* output = new BitVector(*kill);
		output->flip();
		(*output) &= *input;
		(*output) |= *gen;
		return output;
	}

	/**
	 * generate the gen and kill set for the instructions inside a basic block
	 */
	template <class FlowType>
	void LiveAnalysis<FlowType>::initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, idfaInfo *> &InstToInfo) {
		idfaInfo *instInf = InstToInfo[ii];
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

	/**
	 * generate the gen and kill set for each block
	 */
	template <class FlowType>
	void LiveAnalysis<FlowType>::initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo) {
		idfaInfo *BBinf = BBtoInfo[&*Bi];
		(BBinf->gen)->reset(0, domainToIdx.size());
		(BBinf->kill)->reset(0, domainToIdx.size());	
		//Gen set
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

			} else {
				User::op_iterator OI, OE;
				for (OI = ii->op_begin(), OE = ii->op_end(); OI != OE; ++OI) {
					Value *val = *OI;
					if (isa<Instruction>(val) || isa<Argument>(val)) {
						int valIdx = domainToIdx[val];
						//v = v op x will never exist in SSA form
						BitVector tmp = *(BBinf->kill);
						if (!((tmp)[valIdx])) {
							(BBinf->gen)->set(valIdx);
						}
					}

				}
			}
			//Kill set 
			ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
			if (iter != domainToIdx.end()) {
				Value *val = ii;
				int valIdx = domainToIdx[val];
				if (!((*(BBinf->gen))[valIdx])) {
					(BBinf->kill)->set(valIdx);
				}
			}
		}
	}

	/**
	 * get the boundary condition
	 */
	template <class FlowType>
	BitVector* LiveAnalysis<FlowType>::getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) {
		return new BitVector(len, false);
	}

	/**
	 * get the initial flow values
	 */
	template <class FlowType>
	BitVector* LiveAnalysis<FlowType>::initFlowValues(int len) {
		return new BitVector(len, false);
	}
}
