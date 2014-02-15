template <class FValue>
class LiveDFA : public IDFAF<FValue> {
  public:
	std::map<BasicBlock*, bool> reachmap;
	LiveDFA(bool direction) : IDFAF<FValue>(direction) {};
	virtual void set_boundary(Function *F, DFANODE<FValue> *node) {};
	bool reachable(BasicBlock *pred, BasicBlock *B) {
	  if (reachmap.find(B) != reachmap.end()) return false;
	  reachmap.insert(std::pair<BasicBlock*, bool>(B, true));
	  if (pred == B) return true;
      for (succ_iterator succ_I = succ_begin(B), succ_E = succ_end(B);
	    succ_I != succ_E; ++succ_I) {
		/* Successor of B */
        BasicBlock *succ = *succ_I;
		if (reachable(pred, succ)) return true;
	  }
	  return false;
	};
	virtual void trans_func(DFANODE<FValue> *node) {
	  if (!node->isblock()) {
		Instruction *I = node->get_instr();
		{
		  if (IDFAF<FValue>::vmap.find(I) != IDFAF<FValue>::vmap.end()) node->kill->set(IDFAF<FValue>::vmap[I]);
		  for (User::op_iterator Op_I = I->op_begin(), Op_E = I->op_end();
			  Op_I != Op_E; ++Op_I) {
            Value *val = dyn_cast<Value>(Op_I);
		    if (IDFAF<FValue>::vmap.find(val) != IDFAF<FValue>::vmap.end()) node->gen->set(IDFAF<FValue>::vmap[val]);
          }
		}
	  }
	  {
		/* Perform transfer function.*/
		BitVector *tmp = new BitVector(2048, false);
        //SparseBitVector<SPARSE_LEN> *tmp = new SparseBitVector<SPARSE_LEN>();
		*tmp = *(node->out);
		*tmp &= *(node->kill);
		*tmp ^= *(node->out); // tmp = out - kill
		*tmp |= *(node->gen); // tmp = gen | (out - kill)
        *(node->in) = *tmp;
		delete tmp;
	  }
	};
	virtual void meet_oper(DFANODE<FValue> *node) {
	  if (node->isblock()) {
		BasicBlock *B = node->get_block();
        std::map<  BasicBlock*, DFANODE<FValue>*> *blockmap = IDFAF<FValue>::blockmap;

        /* Since it is livness, we travers through all the successors. */
        for (succ_iterator succ_I = succ_begin(B), succ_E = succ_end(B);
			succ_I != succ_E; ++succ_I) {
          BasicBlock *succ = *succ_I;
		  DFANODE<FValue> *next_node = (*blockmap)[succ];
          //*(node->out) |= *(next_node->in);
		  /* Check the reachability of each variable's def at the predecessor. */
	      int idx = next_node->in->find_first();
	      while (idx != -1) {
		    Value *value = IDFAF<FValue>::imap[idx];
			if (isa<Instruction>(value)) {
			  /* If "def(v)" is reachable at B, then union. */
			  Instruction *I = dyn_cast<Instruction>(value);
			  reachmap.clear();
			  if (reachable(B, I->getParent())) node->out->set(idx);
			}
			else if (isa<Argument>(value)) {
			  node->out->set(idx);
			}
            idx = next_node->in->find_next(idx);
		  }
		}
	  }
	  else {
		Instruction *I = node->get_instr();
        /* Since it is livness, we apply reverse order. */
		BasicBlock::iterator instr_I = I;
        std::map<  Instruction*, DFANODE<FValue>*> *instrmap = IDFAF<FValue>::instrmap;
		if (++instr_I != I->getParent()->end()) {
          Instruction *next_I = dyn_cast<Instruction>(instr_I);
		  DFANODE<FValue> *next_node = (*instrmap)[next_I];
          *(node->out) = *(next_node->in);
		}
	  }
	};
	virtual void set_domain(Function *F) {
	  int vc = 0;

	  /* Travel through all values. */
      for (Function::iterator block_I = F->begin(), block_E = F->end();
	      block_I != block_E; ++block_I) {
        BasicBlock *B = dyn_cast<  BasicBlock>(block_I);
        for (  BasicBlock::iterator instr_I = B->begin(), instr_E = B->end();
	        instr_I != instr_E; ++instr_I) {
          Instruction *I = dyn_cast<  Instruction>(instr_I);
		  for (User::op_iterator Op_I = I->op_begin(), Op_E = I->op_end();
			  Op_I != Op_E; ++Op_I) {
            Value *val = dyn_cast<Value>(Op_I);
			if (isa<Instruction>(val) || isa<Argument>(val)) {
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
