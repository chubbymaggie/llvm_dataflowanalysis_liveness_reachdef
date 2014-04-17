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

#include "llvm/Support/PatternMatch.h"


#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

	class OptSummary {
		public:
			int algbraicIdent;
			int constantFold;
			int strengthReduce;
			int deleteUnused;
			//int consecutiveUnion;
			//int associate;
			OptSummary(): algbraicIdent(0), constantFold(0), strengthReduce(0), deleteUnused(0) {}//, consecutiveUnion(0), associate(0) {}
	};

	class LocalOpts : public FunctionPass {
		public:
			static char ID;
			LocalOpts() : FunctionPass(ID) {}


			void replaceAndErase(Value *val, BasicBlock::iterator &ii) {
				//errs() << "in replaceAndErase" << "\n";
				if (val != NULL) {
					//errs() << "fdsafsafsafsafadsafdsafa: " << (val->getType() == ii->getType()) << "\n";
					ii->replaceAllUsesWith(val);
				}
				BasicBlock::iterator tmp = ii;
				++ii;
				if (tmp->getParent()) {
					tmp->eraseFromParent();
				}
			}


			virtual bool runOnFunction(Function &F) {
				OptSummary *ops = new OptSummary();
				bool funcchanged = false;

				for (Function::iterator BB = F.begin(), Be = F.end(); BB != Be; ++BB) {
					//for (BasicBlock::iterator ii = BB->begin(), ie = BB->end(); ii != ie; ++ii) {}
					bool bbchanged = true;
					while (bbchanged) {
						bbchanged = false;
						BasicBlock::iterator ii = BB->begin(), ie = BB->end();
						while (ii != ie) {
							bool instchanged = false;
							Value *L, *R, *res;
							if (ii->getNumOperands() == 2) {
								L = ii->getOperand(0);
								R = ii->getOperand(1);
							}

							unsigned op = ii->getOpcode();

							APInt zero, one;
							//32 bit or 64 bit integer?
							if (IntegerType *inttype = dyn_cast<IntegerType>(ii->getType())) {
								zero = APInt(inttype->getBitWidth(), 0);
								one = APInt(inttype->getBitWidth(), 1);	
							}

/*				
							if (op == Instruction::SExt) {
								Value *S = ii->getOperand(0);
								if (ConstantInt *SC = dyn_cast<ConstantInt>(S)) {
									//Constant -> Value
									Value *conv = ConstantInt::get(ii->getType(), SC->getSExtValue());//, false);
									replaceAndErase(conv, ii);
									//ops->constantFold++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::Trunc) {
								Value *S = ii->getOperand(0);
								if (ConstantInt *SC = dyn_cast<ConstantInt>(S)) {
									//Value *conv = ConstantInt::get(ii->getType(), SC->getValue());
									Value *conv = ConstantInt::get(ii->getType(), SC->getSExtValue());
									replaceAndErase(conv, ii);
									//ops->constantFold++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							}
*/
							if (op == Instruction::Add ||op == Instruction::Sub ||op == Instruction::Mul ||op == Instruction::SDiv ||op == Instruction::UDiv ||op == Instruction::Shl ||op == Instruction::AShr ||op == Instruction::LShr) {
								//no one use this instruction...then we delete it.....
								if (!ii->hasNUsesOrMore(1)) {
									//hope this works.....&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
									replaceAndErase(NULL, ii);
									ops->deleteUnused++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							}

if (op == Instruction::Add) {
								if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
									if (LC->getValue() == zero) {
										replaceAndErase(R, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									if (RC->getValue() == zero) {
										replaceAndErase(L, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							} else if (op == Instruction::Sub) {
								//dyn_cast<Instruction>(L);

								//if ((dyn_cast<Instruction>(L))->isIdenticalTo(dyn_cast<Instruction>(R))) {
								//}
								if (L == R) {
									replaceAndErase(ConstantInt::get(L->getContext(), zero), ii);
									ops->algbraicIdent++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									if (RC->getValue() == zero) {
										replaceAndErase(L, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
									//change the following cast into dyn_cast???
								} 
							} else if (op == Instruction::Mul) {
								if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
									if (LC->getValue() == zero) {
										replaceAndErase(ConstantInt::get(LC->getContext(), zero), ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									} else if (LC->getValue() == one) {
										replaceAndErase(R, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									if (RC->getValue() == zero) {
										replaceAndErase(ConstantInt::get(RC->getContext(), zero), ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									} else if (RC->getValue() == one) {
										replaceAndErase(L, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							} else if (op == Instruction::UDiv || op == Instruction::SDiv) {
								//.............we need to judge whether R is 0.....
								//if R is a variable, can we just add a assert R != 0??????????????????????
								//............................assert..............................

								//if (dyn_cast<Instruction>(L)->isIdenticalTo(dyn_cast<Instruction>(R))) {
								//}
								//...........

								if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
									if (LC->getValue() == zero) {
										//........judge whether *RC is equal to zero?????????????????????????????????
										//
										if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
											if (RC->getValue() == zero) {
												errs() << "Divided by Zero Error"<<"\n";
											}
										}

										replaceAndErase(ConstantInt::get(LC->getContext(), zero), ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}	
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									if (RC->getValue() == one) {
										replaceAndErase(L, ii);
										ops->algbraicIdent++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								} else if (L == R) {//...........and L != 0
									//if (dyn_cast<Instruction>(L)->isIdenticalTo(dyn_cast<Instruction>(R))) {
									//L is not a const........if (cast<ConstantInt>(L)->isIdenticalTo(cast<ConstantInt>(R))) {
									//}
									//}
									//

									//if L.type is Constant or Instruction??????

									if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
										if (RC->getValue() == zero) {
											errs() << "Divided by Zero Error"<<"\n";
										}
									}

									replaceAndErase(ConstantInt::get(L->getContext(), one), ii);
									ops->algbraicIdent++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							}


							if (op == Instruction::Add) {

								if (addHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::Sub) {
								if (subHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::Mul) {
								if (mulHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::SDiv) {
								if (sdivHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::UDiv) {
								if (udivHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::Shl) {
								if (shlHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::LShr) {
								if (lshrHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::AShr) {
								if (ashrHelper(L, R, res, 3, ops, ii)) {
									replaceAndErase(res, ii);
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							} else if (op == Instruction::Store) {
								Value *varptr = dyn_cast<StoreInst>(ii)->getPointerOperand();
								Value *value = dyn_cast<StoreInst>(ii)->getValueOperand();
								if (isa<Constant>(*value)) {
									for (Value::use_iterator u = varptr->use_begin(), ue = varptr->use_end(); u != ue; ++u) {
										if (LoadInst *li = dyn_cast<LoadInst>(*u)) {
											li->replaceAllUsesWith(value);
											++ii;
											//we need to add counter to marker......
											//ops->algbraicIdent++;
											instchanged = true;	bbchanged = true; funcchanged = true;
										//	errs() << "flag***666" << "\n";
											continue;
										}
									}
								}
							} else {
								;
							}



							//Constant Folding
							if (op == Instruction::Add || op == Instruction::Sub || op == Instruction::Mul || op == Instruction::SDiv || op == Instruction::UDiv || op == Instruction::Shl || op == Instruction::AShr || op == Instruction::LShr ) {
								if (ii->getNumOperands() == 2 && isa<ConstantInt>(L) && isa<ConstantInt>(R)) {
									Value *result = calcOpRes(op, cast<ConstantInt>(L), cast<ConstantInt>(R));
									if (result) {
										replaceAndErase(result, ii);
										ops->constantFold++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									} else {

									}
								}
							}


							//errs() << *ii << "\n";


							
							if (op == Instruction::Add ||op == Instruction::Mul) {
								//need to be careful enough.....likely that L cannot be an instruction......then dyn_cast<Instruction> should return NULL....
								//llvm::Instruction *LI = dyn_cast<Instruction>(L);
							//	errs() << "wo yao mei zi" << "\n";
								if (consecutiveJoin(op, L, R, ii)) { 
									//ops->consecutiveUnion++;
									ops->constantFold++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								} else if (consecutiveJoin(op, R, L, ii)) {
									//ops->consecutiveUnion++;
									ops->constantFold++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							}


							/*
							//Strength Reductions
							if (op == Instruction::Mul) {
								if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
									APInt mulval = LC->getValue();

									Instruction *RI = dyn_cast<Instruction>(R);

									errs() << RI->getNumOperands() << "\n";


									if (mulval.isPowerOf2()) {
										unsigned lshift = mulval.logBase2();

										BinaryOperator *newInst = BinaryOperator::Create(Instruction::Shl, R, ConstantInt::get(L->getType(), lshift));// isSigned is set to false
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									} 
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									APInt mulval = RC->getValue();
									if (mulval.isPowerOf2()) {
										unsigned lshift = mulval.logBase2();
										BinaryOperator *newInst = BinaryOperator::Create(Instruction::Shl, L, ConstantInt::get(R->getType(), lshift));// isSigned is set to false
										//ii->getParent()->getInstList().insertafter(ii, newInst);
										//insert before ii
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							} else if (op == Instruction::SDiv || op == Instruction::UDiv) {
								if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									APInt divval = RC->getValue();
									if (divval.isPowerOf2()) {
										unsigned rshift = divval.logBase2();
										BinaryOperator *newInst;
										if (op == Instruction::SDiv) {
											newInst = BinaryOperator::Create(Instruction::AShr, L, ConstantInt::get(R->getType(), rshift));// isSigned is set to false
										} else {
											newInst = BinaryOperator::Create(Instruction::LShr, L, ConstantInt::get(R->getType(), rshift));// isSigned is set to false
										}
										//ii->getParent()->getInstList().insertafter(ii, newInst);
										//insert before ii
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							}
							*/


							if (!instchanged) {
								++ii;
							} else {
								//bbchanged = true; funcchanged = true; 
							}

						}


					}


				//	errs() << "hihihihihiihi\n";

					bbchanged = true;
					while (bbchanged) {
						bbchanged = false;
						BasicBlock::iterator ii = BB->begin(), ie = BB->end();
						while (ii != ie) {
							bool instchanged = false;
							Value *L, *R, *res;
							if (ii->getNumOperands() == 2) {
								L = ii->getOperand(0);
								R = ii->getOperand(1);
							}

							unsigned op = ii->getOpcode();

							if (op == Instruction::Add ||op == Instruction::Sub ||op == Instruction::Mul ||op == Instruction::SDiv ||op == Instruction::UDiv ||op == Instruction::Shl ||op == Instruction::AShr ||op == Instruction::LShr) {
								//no one use this instruction...then we delete it.....
								if (!ii->hasNUsesOrMore(1)) {
									//hope this works.....&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
									replaceAndErase(NULL, ii);
									ops->deleteUnused++;
									instchanged = true; bbchanged = true; funcchanged = true;
									continue;
								}
							}

						//	errs() << "***************************\n";

							//Strength Reductions
							if (op == Instruction::Mul) {
								if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
									APInt mulval = LC->getValue();

									//Instruction *RI = dyn_cast<Instruction>(R);
									//errs() << RI->getNumOperands() << "\n";


									if (mulval.isPowerOf2()) {
										unsigned lshift = mulval.logBase2();

										BinaryOperator *newInst = BinaryOperator::Create(Instruction::Shl, R, ConstantInt::get(L->getType(), lshift));// isSigned is set to false
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									} 
								} else if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									APInt mulval = RC->getValue();
									if (mulval.isPowerOf2()) {
										unsigned lshift = mulval.logBase2();
										BinaryOperator *newInst = BinaryOperator::Create(Instruction::Shl, L, ConstantInt::get(R->getType(), lshift));// isSigned is set to false
										//ii->getParent()->getInstList().insertafter(ii, newInst);
										//insert before ii
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							//} else if (op == Instruction::SDiv || op == Instruction::UDiv) {
							} else if (op == Instruction::UDiv) {
								if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
									APInt divval = RC->getValue();
									if (divval.isPowerOf2()) {
										unsigned rshift = divval.logBase2();
										BinaryOperator *newInst;
										if (op == Instruction::SDiv) {
											newInst = BinaryOperator::Create(Instruction::AShr, L, ConstantInt::get(R->getType(), rshift));// isSigned is set to false
										} else {
											newInst = BinaryOperator::Create(Instruction::LShr, L, ConstantInt::get(R->getType(), rshift));// isSigned is set to false
										}
										//ii->getParent()->getInstList().insertafter(ii, newInst);
										//insert before ii
										ii->getParent()->getInstList().insert(ii, newInst);

										replaceAndErase(newInst, ii);
										ops->strengthReduce++;
										instchanged = true; bbchanged = true; funcchanged = true;
										continue;
									}
								}
							}

							//errs() << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";


							if (!instchanged) {
								++ii;
							} else {
								//bbchanged = true; funcchanged = true; 
							}


						}
					}
				}
			
				errs() << "Optimizations For Function [" + F.getName() + "]:\n";
				errs() << "Algebraic Identities: " << ops->algbraicIdent << "\n";
				errs() << "Constant Folding: " << ops->constantFold << "\n";
				errs() << "Strength Reduction: " << ops->strengthReduce << "\n";
				errs() << "Delete Unused: " << ops->deleteUnused << "\n\n";
				return funcchanged;
			}

			bool addHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}

				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::Add, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
					}
				}
				// A + 0 = 0 + A = A
				if (match(L, m_Zero())) {
					//res = Constant::getNullValue(L->getType());
					res = R;
					ops->algbraicIdent ++;
					return true;
				}
				if (match(R, m_Zero())) {
					//res = Constant::getNullValue(L->getType());
					res = L;
					ops->algbraicIdent ++;
					return true;
				}
				
				//A + A = A * 2
				if (match(R, m_Specific(L))) {
					BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, L, ConstantInt::get(L->getType(), 2));
					ii->getParent()->getInstList().insert(ii, newInst);
					res = newInst;
					ops->algbraicIdent++;
					return true;
				}


				// A + A * N = A * (1+N)
				{
					Value *N = 0, *M;
					if (match(R, m_Mul(m_Specific(L), m_Value(N))) ) {
						if (addHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, L, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A + N * A = A * (1+N)
				{
					Value *N = 0, *M;
					if (match(R, m_Mul(m_Value(N), m_Specific(L))) ) {
						if (addHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, L, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A * N + A= (1+N) * A
				{
					Value *N = 0, *M;
					if (match(L, m_Mul(m_Specific(R), m_Value(N))) ) {
						if (addHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, R);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// N * A + A= (1+N) * A
				{
					Value *N = 0, *M;
					if (match(L, m_Mul(m_Value(N), m_Specific(R))) ) {
						if (addHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, R);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}


				// A + (B - A) = (B - A) + A = B
				Value *B = 0;
				if (match(R, m_Sub(m_Value(B), m_Specific(L))) || match(L, m_Sub(m_Value(B), m_Specific(R)))) {
					res = B;
					ops->algbraicIdent++;
					return true;
				}
				//Associate
				// A+(B+C) = (A+B)+C or (A+C)+B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Add(m_Value(B), m_Value(C)))) {
						if (addHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent ++;
							return true;
						}
						if (addHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// (A+B)+C = (A+C)+B or (B+C)+A
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Add(m_Value(A), m_Value(B)))) {
						if (addHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (addHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, A);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// (A-B)+C = A+(C-B) or (A+C)-B
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Sub(m_Value(A), m_Value(B)))) {
						if (subHelper(C, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (addHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A+(B-C) = (A+B)-C or (A-C)+B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Sub(m_Value(B), m_Value(C)))) {
						if (addHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (subHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				
				
				//Combine
				{
					Value *A = 0, *B = 0, *C = 0, *D = 0, *M;
					if (match(L, m_Mul(m_Value(A), m_Value(B))) && match(R, m_Mul(m_Value(C), m_Value(D)))) {
						// A*B+A*D=A*(B+D)
						if (A == C) {
							if (addHelper(B, D, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, A, M);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B+C*A=A*(B+C)
						if (A == D) {
							if (addHelper(B, C, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, A, M);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B+B*D = (A+D)*B
						if (B == C) {
							if (addHelper(A, D, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B+C*B=(A+C)*B
						if (B == D) {
							if (addHelper(A, C, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						
					}
				}

				return false;
			}

			bool subHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::Sub, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
					}
				}

				//A - 0 = A
				if (match(R, m_Zero())) {
					res = L;
					ops->algbraicIdent ++;
					return true;
				}
				//A - A = 0
				if (L == R) {
					res = Constant::getNullValue(L->getType());
					ops->algbraicIdent ++;
					return true;
				}

				// A - A * N = A * (1-N)
				{
					Value *N = 0, *M;
					if (match(R, m_Mul(m_Specific(L), m_Value(N))) ) {
						if (subHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, L, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A - N * A = A * (1-N)
				{
					Value *N = 0, *M;
					if (match(R, m_Mul(m_Value(N), m_Specific(L))) ) {
						if (subHelper(ConstantInt::get(L->getType(), 1), N, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, L, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A * N - A= (N-1) * A
				{
					Value *N = 0, *M;
					if (match(L, m_Mul(m_Specific(R), m_Value(N))) ) {
						if (subHelper(N, ConstantInt::get(L->getType(), 1), M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, R);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// N * A - A= (N-1) * A
				{
					Value *N = 0, *M;
					if (match(L, m_Mul(m_Value(N), m_Specific(R))) ) {
						if (subHelper(N, ConstantInt::get(L->getType(), 1), M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, R);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}


				//(A*2) - A = A
				if (match(L, m_Mul(m_Specific(R), m_ConstantInt<2>())) ) {
					res = L;
					ops->algbraicIdent ++;
					return true;
				}
				// A-B-C = A-(B+C) or (A-C)-B
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Sub(m_Value(A), m_Value(B)))) {
						if (addHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (subHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// (A+B) - C ->A+(B-C) or B + (A-C)
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Add(m_Value(A), m_Value(B)))) {
						if (subHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							//errs() << *newInst << "\n";
							res = newInst;
							ops->algbraicIdent++;
							return true;

							//we can split into two cases,  with whether subHelper here to reduce the convergence rate.
						}
						if (subHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, B, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				// A-(B+C) = (A-B)-C or (A-C)-B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Add(m_Value(B), m_Value(C)))) {
						if (subHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (subHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// A-(B-C) = (A-B)+C or (A+C)-B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Sub(m_Value(B), m_Value(C)))) {
						if (subHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Add, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (addHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Sub, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

	
				//Combine
				{
					Value *A = 0, *B = 0, *C = 0, *D = 0, *M;
					if (match(L, m_Mul(m_Value(A), m_Value(B))) && match(R, m_Mul(m_Value(C), m_Value(D)))) {
						// A*B-A*D=A*(B-D)
						if (A == C) {
							if (subHelper(B, D, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, A, M);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B-C*A=A*(B-C)
						if (A == D) {
							if (subHelper(B, C, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, A, M);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B-B*D = (A-D)*B
						if (B == C) {
							if (subHelper(A, D, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						// A*B-C*B=(A-C)*B
						if (B == D) {
							if (subHelper(A, C, M, num-1, ops, ii)) {
								BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
								ii->getParent()->getInstList().insert(ii, newInst);
								res = newInst;
								ops->algbraicIdent++;
								return true;
							}
						}
						
					}
				}

				return false;
			}

			bool mulHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::Mul, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
					}
				}

				//A * 0 = 0
				//0 * A = 0
				if (match(R, m_Zero())) {
					res = R;
					ops->algbraicIdent++;
					return true;
				}
				if (match(L, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}

				//A * 1 = A
				//1 * A = A
				if(match(L, m_One())) {
					res = R;
					ops->algbraicIdent++;
					return true;
				}
				if(match(R, m_One())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				//Associate
				// A*(B*C) = (A*B)*C or (A*C)*B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Mul(m_Value(B), m_Value(C)))) {
						if (mulHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				// (A*B)*C = (A*C)*B or (B*C)*A
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Mul(m_Value(A), m_Value(B)))) {
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Mul, M, A);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				//Distribute
				{

					// A*(B+C) = A*B+A*C
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Add(m_Value(B), m_Value(C)))) {
						if (mulHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Add, M, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, B);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Add, newInst1, M);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				{

					// A*(B-C) = A*B-A*C
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Sub(m_Value(B), m_Value(C)))) {
						if (mulHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Sub, M, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, B);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Sub, newInst1, M);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				{
					// (A+B)*C = A*C+B*C
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Add(m_Value(A), m_Value(B)))) {
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, B, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Add, M, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Add, newInst1, M);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				{
					// (A-B)*C = A*C-B*C
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_Sub(m_Value(A), m_Value(B)))) {
						if (mulHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, B, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Sub, M, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
						if (mulHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst1 = BinaryOperator::Create(Instruction::Mul, A, C);
							BinaryOperator *newInst2 = BinaryOperator::Create(Instruction::Sub, newInst1, M);
							ii->getParent()->getInstList().insert(ii, newInst1);
							ii->getParent()->getInstList().insert(ii, newInst2);
							res = newInst2;
							ops->algbraicIdent++;
							return true;
						}
					}
				}


				if (match(L, m_Power2())) {


				}
				if (match(R, m_Power2())) {
				}

				return false;
			}

			bool sdivHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::SDiv, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
						// else return false;??-> R = 0;
					}
				}

				//0 / A = 0
				if (match(R, m_Zero())) {
					errs() << "the divisor cannot be zero....."<< "\n";
					return false;
				}
				if (match(L, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				//A / 1 = A
				if (match(R, m_One())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}

				// A / A = 1
				if (L == R) {
					res = ConstantInt::get(L->getType(), 1);
					ops->algbraicIdent++;
					return true;
				}

				// A/(B*C) = (A/B)/C or (A/C)/B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Mul(m_Value(B), m_Value(C)))) {
						if (sdivHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::SDiv, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (sdivHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::SDiv, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				// (A/B)/C = A/(B*C) or (A/C)/B
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_SDiv(m_Value(A), m_Value(B)))) {
						if (mulHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::SDiv, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (sdivHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::SDiv, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				//////////////////////////not put inside...to avoid a * b * 4...........infinite recursive.....
				if (match(R, m_Power2())) {

				}
				return false;

			}


			bool udivHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::UDiv, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
						// else return false;??-> R = 0;
					}
				}

				//0 / A = 0
				if (match(R, m_Zero())) {
					errs() << "the divisor cannot be zero....."<< "\n";
					return false;
				}
				if (match(L, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				//A / 1 = A
				if (match(R, m_One())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}

				// A / A = 1
				if (L == R) {
					res = ConstantInt::get(L->getType(), 1);
					ops->algbraicIdent++;
					return true;
				}

				// A/(B*C) = (A/B)/C or (A/C)/B
				{
					Value *A = L, *B = 0, *C = 0, *M;
					if (match(R, m_Mul(m_Value(B), m_Value(C)))) {
						if (udivHelper(A, B, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::UDiv, M, C);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (udivHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::UDiv, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				// (A/B)/C = A/(B*C) or (A/C)/B
				{
					Value *A = 0, *B = 0, *C = R, *M;
					if (match(L, m_UDiv(m_Value(A), m_Value(B)))) {
						if (mulHelper(B, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::UDiv, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
						if (udivHelper(A, C, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::UDiv, M, B);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}

				//////////////////////////not put inside...to avoid a * b * 4...........infinite recursive.....
				if (match(R, m_Power2())) {

				}
				return false;
			}


			bool shlHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::Shl, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
						// else return false;??-> R = 0;
					}
				}
				//A << 0 = A
				if (match(R, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				// (A << N) << R = A << (N+R);
				{
					Value *A = 0, *N = 0, *M;
					if (match(L, m_Shl(m_Value(A), m_Value(N)))) {
						if (addHelper(N, R, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::Shl, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				return false;
			}

			bool lshrHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::LShr, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
						// else return false;??-> R = 0;
					}
				}
				//A >> 0 = A
				if (match(R, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				// (A >> N) >> R = A >> (N+R);
				{
					Value *A = 0, *N = 0, *M;
					if (match(L, m_LShr(m_Value(A), m_Value(N)))) {
						if (addHelper(N, R, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::LShr, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				return false;
			}


			bool ashrHelper(Value *L, Value *R, Value *&res, int num, OptSummary *ops, BasicBlock::iterator &ii) {
				if (!num) {
					return false;
				}
				if (ConstantInt *LC = dyn_cast<ConstantInt>(L)) {
					if (ConstantInt *RC = dyn_cast<ConstantInt>(R)) {
						res = calcOpRes(Instruction::AShr, LC, RC);
						if (res) {
							ops->constantFold++;
							return true;
						}	
						// else return false;??-> R = 0;
					}
				}
				//A >> 0 = A
				if (match(R, m_Zero())) {
					res = L;
					ops->algbraicIdent++;
					return true;
				}
				// (A >> N) >> R = A >> (N+R);
				{
					Value *A = 0, *N = 0, *M;
					if (match(L, m_AShr(m_Value(A), m_Value(N)))) {
						if (addHelper(N, R, M, num-1, ops, ii)) {
							BinaryOperator *newInst = BinaryOperator::Create(Instruction::AShr, A, M);
							ii->getParent()->getInstList().insert(ii, newInst);
							res = newInst;
							ops->algbraicIdent++;
							return true;
						}
					}
				}
				return false;
			}




				bool consecutiveJoin(unsigned op, Value *L, Value *R, BasicBlock::iterator &ii) {
					Instruction::BinaryOps Opcode = (Instruction::BinaryOps)op;
					if (!isa<Constant>(L) ) {
						//errs() << "wo yao mei zi2" << "\n";
						if (llvm::Instruction *LI = dyn_cast<Instruction>(L)) {

							//errs() << "wo yao mei zi3" << "\n";
							if (ConstantInt *RV = dyn_cast<ConstantInt>(R)) {
								//errs() << "wo yao mei zi4" << "\n";
								//unsigned op2 = LI->getOpcode();
								if  (LI->getNumOperands() == 2 && LI->getOpcode() == Opcode) {
									//errs() << "wo yao mei zi5" << "\n";
									Value *LL, *LR; 
									LL = LI->getOperand(0);
									LR = LI->getOperand(1);
									if (ConstantInt *LRC = dyn_cast<ConstantInt>(LR)) {

										//errs() << "wo yao mei zi6" << "\n";
										if (ConstantInt *Res = calcOpRes(op, dyn_cast<ConstantInt>(LR), dyn_cast<ConstantInt>(R))) {
											//errs() << "wo yao mei zi7" << "\n";
											BinaryOperator *newInst = BinaryOperator::Create(Opcode, LL, Res);
											//insert before ii
											ii->getParent()->getInstList().insert(ii, newInst);

											replaceAndErase(newInst, ii);
											return true;
										}
									} else if (ConstantInt *LLC = dyn_cast<ConstantInt>(LL)) {
										if (ConstantInt *Res = calcOpRes(op, dyn_cast<ConstantInt>(LL), dyn_cast<ConstantInt>(R))) {
											//errs() << "wo yao mei zi8" << "\n";
											BinaryOperator *newInst = BinaryOperator::Create(Opcode, LR, Res);
											//insert before ii
											ii->getParent()->getInstList().insert(ii, newInst);

											replaceAndErase(newInst, ii);
											return true;

										}
									}
								} else {
								}
							}
						}
					}
					return false;
				}




				ConstantInt *calcOpRes(unsigned op, ConstantInt *L, ConstantInt *R) {
				//	errs() << "coming inside......................";

					if (L == NULL || R == NULL) {
						return NULL;
					}

					if (op == Instruction::Add) {
						return ConstantInt::get(L->getContext(), L->getValue() + R->getValue());
						//return ConstantExpr::getAdd(cast<ConstantInt>(I->getOperand(0)), cast<ConstantInt>(I->getOperand(1)));
						//return ConstantInt::get(cast<IntegerType>(ii->getType()), L->getValue() + R->getValue());
						//return cast<ConstantInt>(ConstantInt::get(L->getType(), L->getValue() + R->getValue()));
					} else if (op == Instruction::Sub) {
						return ConstantInt::get(L->getContext(), L->getValue() - R->getValue());
					} else if (op == Instruction::Mul) {
						return ConstantInt::get(L->getContext(), L->getValue() * R->getValue());
					} else if (op == Instruction::UDiv) {
						if (!R->isZero()) {
							return ConstantInt::get(L->getContext(), L->getValue().udiv(R->getValue()));
						} else {
							return NULL;
						}
					} else if (op == Instruction::SDiv) {
						if (!R->isZero()) {
							return ConstantInt::get(L->getContext(), L->getValue().sdiv(R->getValue()));
						} else {
							return NULL;
						}
					} else if (op == Instruction::Shl) {
						return ConstantInt::get(L->getContext(), L->getValue().shl(R->getValue()));
					} else if (op == Instruction::AShr) {
						return ConstantInt::get(L->getContext(), L->getValue().ashr(R->getValue()));
					} else if (op == Instruction::LShr) {
						return ConstantInt::get(L->getContext(), L->getValue().lshr(R->getValue()));
					} else {
						errs() << "Constant Fold...........error....**********************\n";
						return NULL;
					}
				}

				// We don't modify the program, so we preserve all analyses
				virtual void getAnalysisUsage(AnalysisUsage &AU) const {
					AU.setPreservesAll();
				}
			};

			// LLVM uses the address of this static member to identify the pass, so the
			// initialization value is unimportant.
			char LocalOpts::ID = 0;

			// Register this pass to be used by language front ends.
			// This allows this pass to be called using the command:
			//    clang -c -Xclang -load -Xclang ./FunctionInfo.so loop.c
			static void registerMyPass(const PassManagerBuilder &,
					PassManagerBase &PM) {
				PM.add(new LocalOpts());
			}
			RegisterStandardPasses
				RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
						registerMyPass);

			// Register the pass name to allow it to be called with opt:
			//    clang -c -emit-llvm loop.c
			//    opt -load ./FunctionInfo.so -function-info loop.bc > /dev/null
			// See http://llvm.org/releases/3.4/docs/WritingAnLLVMPass.html#running-a-pass-with-opt for more info.
			RegisterPass<LocalOpts> X("my-local-opts", "my-local-opts");

	}
