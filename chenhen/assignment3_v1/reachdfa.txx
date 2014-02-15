template <class FValue>
class ReachDFA : public IDFAF<FValue> {
  public:
	ReachDFA(bool direction) : IDFAF<FValue>(direction) {};
	virtual void set_boundary(Function *F, DFANODE<FValue> *node) {
	  for (Function::arg_iterator Arg_I = F->arg_begin(), Arg_E = F->arg_end();
		  Arg_I != Arg_E; ++Arg_I) {
		Value *arg = dyn_cast<Value>(Arg_I);
        if (IDFAF<FValue>::vmap.find(arg) != IDFAF<FValue>::vmap.end()) node->in->set(IDFAF<FValue>::vmap[arg]);
	  }
	};
	virtual void trans_func(DFANODE<FValue> *node) {
	  if (!node->isblock()) {
		Instruction *I = node->get_instr();
		// todo: explore more possible Instructions.
		if (I->getOpcode() == Instruction::Br) {
		}
		else if (I->getOpcode() == Instruction::Ret) {
		}
		else if (I->getOpcode() == Instruction::Store) {
		}
		/*
		else if (I->getOpcode() == Instruction::PHI) {
		  if (IDFAF<FValue>::vmap.find(I->getOperand(0)) != IDFAF<FValue>::vmap.end())
			node->gen->set(IDFAF<FValue>::vmap[I->getOperand(0)]);
		  if (IDFAF<FValue>::vmap.find(I->getOperand(1)) != IDFAF<FValue>::vmap.end())
			node->gen->set(IDFAF<FValue>::vmap[I->getOperand(1)]);
		}
		*/
		else {
		  if (IDFAF<FValue>::vmap.find(I) != IDFAF<FValue>::vmap.end())
			node->gen->set(IDFAF<FValue>::vmap[I]);
		  /*
		  if (!I->isUsedOutsideOfBlock(I->getParent())) {
		    if (IDFAF<FValue>::vmap.find(I) != IDFAF<FValue>::vmap.end()) {
			  node->kill->set(IDFAF<FValue>::vmap[I]);
              errs() << *I << '\n';
			}
		  }
		  */
		}
	  }
	  {
		/* Perform transfer function.*/
		BitVector *tmp = new BitVector(2048, false);
        //SparseBitVector<SPARSE_LEN> *tmp = new SparseBitVector<SPARSE_LEN>();
		*tmp = *(node->in);
		*tmp &= *(node->kill);
		*tmp ^= *(node->in); // tmp = in - kill
		*tmp |= *(node->gen); // tmp = gen | (in - kill)
        *(node->out) = *tmp;
		delete tmp;
	  }
	};
	virtual void meet_oper(DFANODE<FValue> *node) {
	  if (node->isblock()) {
		BasicBlock *B = node->get_block();
        std::map< BasicBlock*, DFANODE<FValue>*> *blockmap = IDFAF<FValue>::blockmap;

        /* Since it is reach definition, we travers through all the predecessors. */
        for (pred_iterator pred_I = pred_begin(B), pred_E = pred_end(B);
			pred_I != pred_E; ++pred_I) {
          BasicBlock *pred = *pred_I;
		  DFANODE<FValue> *next_node = (*blockmap)[pred];
          *(node->in) |= *(next_node->out);
		}
	  }
	  else {
	    Instruction *I = node->get_instr();
        /* Since it is reach definition, we apply normal order. */
		BasicBlock::iterator instr_I = I;
        std::map<  Instruction*, DFANODE<FValue>*> *instrmap = IDFAF<FValue>::instrmap;
		if (instr_I != I->getParent()->front()) {
		  --instr_I;
          Instruction *next_I = dyn_cast<  Instruction>(instr_I);
		  DFANODE<FValue> *next_node = (*instrmap)[next_I];
          *(node->in) = *(next_node->out);
		}
	  }
	};
	void set_domain(Function *F) {
	  int vc = 0;

	  /* Travel through all values. */
      for (Function::iterator block_I = F->begin(), block_E = F->end();
	      block_I != block_E; ++block_I) {
        BasicBlock *B = dyn_cast< BasicBlock>(block_I);
        for ( BasicBlock::iterator instr_I = B->begin(), instr_E = B->end();
	        instr_I != instr_E; ++instr_I) {
          Instruction *I = dyn_cast< Instruction>(instr_I);
		  for (User::op_iterator Op_I = I->op_begin(), Op_E = I->op_end();
			  Op_I != Op_E; ++Op_I) {
            Value *val = dyn_cast<Value>(Op_I);
			if (isa<  Instruction>(val) || isa<Argument>(val)) {
			  if (IDFAF<FValue>::vmap.find(val) == IDFAF<FValue>::vmap.end()) {
		        IDFAF<FValue>::vmap.insert(std::pair<FValue*, int>(val, vc));
		        IDFAF<FValue>::imap.insert(std::pair<int, FValue*>(vc, val));
				vc ++;
			  }
			}
		  }
	      if (IDFAF<FValue>::vmap.find(I) == IDFAF<FValue>::vmap.end()) {
		    IDFAF<FValue>::vmap.insert(std::pair<FValue*, int>(I, vc));
		    IDFAF<FValue>::imap.insert(std::pair<int, FValue*>(vc, I));
		    vc ++;
		  }
		}
	  }
	};
};
