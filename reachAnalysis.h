// CS380C S14 Assignment 3: reach.h
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
		class ReachAnalysis : public IDFA<FlowType> {
			public:
				ReachAnalysis() : IDFA<FlowType>() {}

				//meet operator
				void meetOp(BitVector *op1, BitVector *op2);
				//transfer functions
				BitVector* transferFunc(BitVector *input, BitVector *gen, BitVector *kill);
				//generate the gen and kill set for the instructions inside a basic block
				void initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, idfaInfo *> &InstToInfo);
				//generate the gen and kill set for each block
				void initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo);
				//get the boundary condition
				BitVector* getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx);
				//get the initial flow values
				BitVector* initFlowValues(int len);
		};

	/**
	 * meet operator
	 */
	template <class FlowType>
	void ReachAnalysis<FlowType>::meetOp(BitVector *op1, BitVector *op2) {
		(*op1) |= (*op2);
	}

	/**
	 * transfer functions
	 */
	template <class FlowType>
	BitVector* ReachAnalysis<FlowType>::transferFunc(BitVector *input, BitVector *gen, BitVector *kill) {
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
	void ReachAnalysis<FlowType>::initInstGenKill(Instruction *ii, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const Instruction *, idfaInfo *> &InstToInfo) {
		//do we need to add phi node def to the gen set?
		if (!isa<PHINode>(ii)) {
			idfaInfo *instInf = InstToInfo[ii];
			ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
			if (iter != domainToIdx.end()) {
				Value *val = ii;
				int valIdx = domainToIdx[val];
				(instInf->gen)->set(valIdx);
			}
		}
	}

	/*
	 * generate the gen and kill set for each block
	 */
	template <class FlowType>
	void ReachAnalysis<FlowType>::initGenKill(BasicBlock *Bi, BasicBlock *Pi, ValueMap<Value *, unsigned> &domainToIdx, ValueMap<const BasicBlock *, idfaInfo *> &BBtoInfo) {
		idfaInfo *BBinf = BBtoInfo[&*Bi];
		(BBinf->gen)->reset(0, domainToIdx.size());
		(BBinf->kill)->reset(0, domainToIdx.size());	
		for (BasicBlock::iterator ii = Bi->begin(), ie = Bi->end(); ii != ie; ++ii) {
			//do we need to add phi node def to the gen set?
			if(!isa<PHINode>(ii)){
				ValueMap<Value*, unsigned>::const_iterator iter = domainToIdx.find(dyn_cast<Instruction>(ii));
				if (iter != domainToIdx.end()) {
					Value *val = ii;
					int valIdx = domainToIdx[val];
					(BBinf->gen)->set(valIdx);
				}
			}
		}
	}

	/**
	 * get the boundary condition
	 */
	template <class FlowType>
	BitVector* ReachAnalysis<FlowType>::getBoundaryCondition(int len, Function &F, ValueMap<Value *, unsigned> &domainToIdx) {
		BitVector *output = new BitVector(len, false);
		for (Function::arg_iterator arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
			output->set(domainToIdx[&*arg]);
		}
		return output;
	}

	/**
	 * get the initial flow values
	 */
	template <class FlowType>
	BitVector* ReachAnalysis<FlowType>::initFlowValues(int len) {
		return new BitVector(len, false);
	}

}
