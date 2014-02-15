#define SPARSE_LEN 1024

class LATTICE : public SparseBitVector<2048>, public BitVector {
  public:
    LATTICE(bool sparse) : BitVector(256, false) {};
    LATTICE() : SparseBitVector<2048>() {};
};

template <class T>
class WQUEUE {
  public:
	WQUEUE (bool direction) { forward = direction; }
	void enqueue (T* node) { nodequeue.push_back(node); }
	T *dequeue() {
	  T *node;
      assert(!nodequeue.empty());
	  if (forward) {
		node = nodequeue.front();
		nodequeue.pop_front();
	  }
	  else {
		node = nodequeue.back();
		nodequeue.pop_back();
	  }
	  return node;
	}
	bool isempty () {return nodequeue.empty();}
  private:
	bool forward;
	std::deque<T*> nodequeue;
};

template <class FValue>
class DFANODE {
  public:
	void init_set() {
	  in = new BitVector(2048, false);
	  out = new BitVector(2048, false);
	  gen = new BitVector(2048, false);
	  kill = new BitVector(2048, false);
	  tmp_in = new BitVector(2048, false);
	  tmp_out = new BitVector(2048, false);
	  /*
	  in = new SparseBitVector<SPARSE_LEN>();
	  out = new SparseBitVector<SPARSE_LEN>();
	  gen = new SparseBitVector<SPARSE_LEN>();
	  kill = new SparseBitVector<SPARSE_LEN>();
	  tmp_in = new SparseBitVector<SPARSE_LEN>();
	  tmp_out = new SparseBitVector<SPARSE_LEN>();
	  */
	};
	DFANODE(Instruction *node_ptr) {
      instr = node_ptr;
	  isblk = false;
	  init_set();
	}
	DFANODE(BasicBlock *node_ptr) {
      block = node_ptr;
	  isblk = true;
	  init_set();
	}
	void snapshot() { *tmp_in = *in; *tmp_out = *out;}
	bool changed() { return !(*tmp_in == *in && *tmp_out == *out); }
    Instruction *get_instr() { return instr; }
    BasicBlock *get_block() { return block; }
	bool isblock() { return isblk; };
	void print_in(std::map<int, FValue*> imap) { errs() << ";"; print_set(*in, imap); }
	void print_out(std::map<int, FValue*> imap) { errs() << ";"; print_set(*out, imap); }
	void print_gen(std::map<int, FValue*> imap) { errs() << ";"; print_set(*gen, imap); }
	void print_kill(std::map<int, FValue*> imap) { errs() << ";"; print_set(*kill, imap); }
	/*
	void print_set(SparseBitVector<SPARSE_LEN> vec, std::map<int, FValue*> imap) {
	  int idx;
      for (SparseBitVector<SPARSE_LEN>::iterator sp_I = vec.begin(), sp_E = vec.end();
		  sp_I != sp_E; ++sp_I) {
		idx = dyn_cast<unsigned>(sp_I);
		if (imap[idx]->hasName()) errs() << "%" << imap[idx]->getName() << ", ";
		else errs() << *(imap[idx]) << ", ";
	  }
	}
	*/
	void print_set(BitVector vec, std::map<int, FValue*> imap) {
	  int idx;
	  errs() << "";
	  idx = vec.find_first();
	  while (idx != -1) {
		if (imap[idx]->hasName()) errs() << "%" << imap[idx]->getName() << ", ";
		else errs() << *(imap[idx]) << ", ";
        idx = vec.find_next(idx);
	  }
      errs() << "" << '\n';
	}
	bool isblk;
	Instruction *instr;
	BasicBlock *block;
	BitVector *in;
	BitVector *out;
	BitVector *gen;
	BitVector *kill;
	BitVector *tmp_in;
	BitVector *tmp_out;
	/*
	SparseBitVector<SPARSE_LEN> *in;
	SparseBitVector<SPARSE_LEN> *out;
	SparseBitVector<SPARSE_LEN> *gen;
	SparseBitVector<SPARSE_LEN> *kill;
	SparseBitVector<SPARSE_LEN> *tmp_in;
	SparseBitVector<SPARSE_LEN> *tmp_out;
	*/
};

template <class FValue>
class IDFAF {
  public:
    IDFAF(bool direction);
    bool forward;
    void blk_anal(BasicBlock *B, bool propogate);
	void fun_anal(Function *F);
	void report(Function *F);
    void report(Function *F, std::map<int, FValue*> imap);
	virtual void set_boundary(Function *F, DFANODE<FValue> *node) {}
	virtual void trans_func(DFANODE<FValue> *node) {};
	virtual void meet_oper(DFANODE<FValue> *node) {};
	virtual void set_domain(Function *F) {};
	virtual void top() {};
    std::map<Instruction*, DFANODE<FValue>*> *instrmap;
    std::map<BasicBlock*, DFANODE<FValue>*> *blockmap;
	std::map<FValue*, int> vmap;
	std::map<int, FValue*> imap;
};

template <class FValue>
IDFAF<FValue>::IDFAF(bool direction) {
  forward = direction;
  instrmap = new std::map<Instruction*, DFANODE<FValue>*>();
  blockmap = new std::map<BasicBlock*, DFANODE<FValue>*>();
};

template <class FValue>
void IDFAF<FValue>::report(Function *F, std::map<int, FValue*> imap) {
  for (Function::iterator block_I = F->begin(), block_E = F->end();
	  block_I != block_E; ++block_I) {
    BasicBlock *B = dyn_cast<BasicBlock>(block_I);
	DFANODE<FValue> *blknode = (*blockmap)[B];
	errs() << '\n';
    blknode->print_in(imap);
	if (B->hasName()) {
	  errs() << "                                                        ";
	  errs() << B->getName() << ": " << '\n';
	}
    for (BasicBlock::iterator instr_I = B->begin(), instr_E = B->end();
	    instr_I != instr_E; ++instr_I) {
        Instruction *I = dyn_cast<Instruction>(instr_I);
	  if (I->getOpcode() != Instruction::PHI) {
	    DFANODE<FValue> *dfanode = (*instrmap)[I];
	    dfanode->print_in(imap);
	  }
	    errs() << "                                                        ";
	    errs() << *I << '\n';
	}
    if (B->back().getOpcode() != Instruction::Br) {
	  blknode->print_out(imap);
	}
  }
};

template <class FValue>
void IDFAF<FValue>::blk_anal(BasicBlock *B, bool propogate) {
  DFANODE<FValue> *ret = (*blockmap)[B];
  WQUEUE<Instruction> *wqueue = new WQUEUE<Instruction>(forward);

  /* Loop over   Instructions and insert them to the work list. */
  for (BasicBlock::iterator instr_I = B->begin(), instr_E = B->end();
	    instr_I != instr_E; ++instr_I) {
    Instruction *I = dyn_cast<Instruction>(instr_I);
	/* If DFANODE hasn't been created. */
	if (instrmap->find(I) == instrmap->end()) {
	  DFANODE<FValue> *dfanode = new DFANODE<FValue>(I);
      std::pair<Instruction*, DFANODE<FValue>*> *pair
	    = new std::pair<Instruction*, DFANODE<FValue>*>(I, dfanode);
      instrmap->insert(*pair);
    }
	wqueue->enqueue(I);
  }

  /* Loop over instrions to get gen and kill sets. */
  while (!wqueue->isempty()) {
	Instruction *I = wqueue->dequeue();
	DFANODE<FValue> *dfanode = (*instrmap)[I];
	/* The first or last   Instruction of this block. Propogate in or out set. */
	if (forward) {
	  if (B->front().isIdenticalTo(I)) *(dfanode->in) = *(ret->in);
	}
	else {
	  if (B->back().isIdenticalTo(I)) *(dfanode->out) = *(ret->out);
	}
	meet_oper(dfanode);
	trans_func(dfanode);
	if (!propogate) {
      BitVector *tmp = new BitVector(2048, false);
      //SparseBitVector<SPARSE_LEN> *tmp = new SparseBitVector<SPARSE_LEN>();
	  *(ret->gen) |= *(dfanode->gen); /* First merge gen set. */
	  *tmp = *(ret->gen);
	  *tmp &= *(dfanode->kill); /* Check if any value is killed. */
	  *(ret->gen) ^= *tmp; /* Eliminate killed values from gen set. */
	  *tmp = *(ret->gen);
	  *tmp &= *(ret->kill); /* Check if any value be regen such as PHI. */
	  *(ret->kill) ^= *tmp; /* Elimiate those alive value from kill set. */
	  *(ret->kill) |= *(dfanode->kill); /* Update kill set */
      delete tmp;
	}
  }
};

template <class FValue>
void IDFAF<FValue>::fun_anal(Function *F) {
  WQUEUE<BasicBlock> *wqueue = new WQUEUE<BasicBlock>(forward);

  /* Setup semi lattice domain. */
  set_domain(F);

  while (1) {
    bool changed = false;

	/* Loop over BasicBlocks to generate block level gen, kill sets. */
    for (Function::iterator block_I = F->begin(), block_E = F->end();
	    block_I != block_E; ++block_I) {
      BasicBlock *B = dyn_cast<BasicBlock>(block_I);
	  /* Generate DFANODE for the block, if it hasn't been createded yet.*/
      if (blockmap->find(B) == blockmap->end()) {
        DFANODE<FValue> *dfanode = new DFANODE<FValue>(B);
		/* Set boundary condition. In SSA form, this is OK, since
		 * arguements won't be redefined in. */
		set_boundary(F, dfanode);
        std::pair<BasicBlock*, DFANODE<FValue>*> *pair
		  = new std::pair<BasicBlock*, DFANODE<FValue>*>(B, dfanode);
        blockmap->insert(*pair);
	    blk_anal(B, false);
	  }
	  wqueue->enqueue(B);
	}

    while (!wqueue->isempty()) {
      BasicBlock *B = wqueue->dequeue();
	  DFANODE<FValue> *dfanode = (*blockmap)[B];
	  dfanode->snapshot();
      meet_oper(dfanode);
	  trans_func(dfanode);
	  if (dfanode->changed()) changed = true;
	}

    if (!changed) break;
  }

  /* Propogate the block info into Instructions. */
  for (Function::iterator block_I = F->begin(), block_E = F->end();
	  block_I != block_E; ++block_I) {
    BasicBlock *B = dyn_cast<BasicBlock>(block_I);
    blk_anal(B, true);
  }
};

template<class FValue>
class Annotator : public AssemblyAnnotationWriter {
  public:
	IDFAF<FValue> *idfaf;
	Annotator(IDFAF<FValue> *a) { idfaf = a; }
	virtual void emitBasicBlockStartAnnot(const BasicBlock *bb, formatted_raw_ostream &os) {
	  BasicBlock *B = const_cast<BasicBlock*>(bb);
      os << "; ";
	  DFANODE<FValue> *dfanode = (*idfaf->blockmap)[B];
	  int idx = dfanode->in->find_first();
	  while (idx != -1) {
		os << "%" << idfaf->imap[idx]->getName() << ", ";
        idx = dfanode->in->find_next(idx);
	  }
	  os << '\n';
	}
	virtual void emitInstructionStartAnnot(const Instruction *i, formatted_raw_ostream &os) {
	  Instruction *I = const_cast<Instruction*>(i);
      os << "; ";
	  DFANODE<FValue> *dfanode = (*idfaf->instrmap)[I];
	  int idx = dfanode->in->find_first();
	  while (idx != -1) {
		os << "%" << idfaf->imap[idx]->getName() << ", ";
        idx = dfanode->in->find_next(idx);
	  }
	  os << '\n';
	}
};
