#include "../include/BogusControlFlow.h"
#include <random>
// Stats

using namespace llvm;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dis(0, 1000);

// Options for the pass
const int defaultObfRate = 100, defaultObfTime = 10;

int checkOpcodeIsInIntLogicCode(unsigned opcode){
    return  opcode == Instruction::Add  || opcode == Instruction::Sub  ||
            opcode == Instruction::Mul  || opcode == Instruction::UDiv ||
            opcode == Instruction::SDiv || opcode == Instruction::URem ||
            opcode == Instruction::SRem || opcode == Instruction::Shl  ||
            opcode == Instruction::LShr || opcode == Instruction::AShr ||
            opcode == Instruction::And  || opcode == Instruction::Or   ||
            opcode == Instruction::Xor;
}

int checkOpcodeIsInFloatLogicCode(unsigned opcode){
    return  opcode == Instruction::FAdd || opcode == Instruction::FSub ||
            opcode == Instruction::FMul || opcode == Instruction::FDiv ||
            opcode == Instruction::FRem;
}

llvm::CmpInst::Predicate ICmpOpCodeList[10] = {
    ICmpInst::ICMP_EQ,
    ICmpInst::ICMP_UGT,
    ICmpInst::ICMP_UGE,
    ICmpInst::ICMP_ULT,
    ICmpInst::ICMP_ULE,
    ICmpInst::ICMP_SGT,
    ICmpInst::ICMP_SGE,
    ICmpInst::ICMP_SLT,
    ICmpInst::ICMP_SLE
};

llvm::CmpInst::Predicate FCmpOpCodeList[10] = {
    FCmpInst::FCMP_OEQ,
    FCmpInst::FCMP_ONE,
    FCmpInst::FCMP_UGT,
    FCmpInst::FCMP_UGE,
    FCmpInst::FCMP_ULT,
    FCmpInst::FCMP_ULE,
    FCmpInst::FCMP_OGT,
    FCmpInst::FCMP_OGE,
    FCmpInst::FCMP_OLT,
    FCmpInst::FCMP_OLE
};

static int ObfTimes = 1;
static int ObfProbRate = 100;

namespace {
    BasicBlock* createalterBasicBlock(BasicBlock * basicBlock, const Twine &  Name = "gen", Function * F = 0){
        ValueToValueMapTy VMap;
        BasicBlock * alter = llvm::CloneBasicBlock (basicBlock, VMap, Name, F);
        BasicBlock::iterator ji = basicBlock->begin();
        for (BasicBlock::iterator i = alter->begin(), e = alter->end() ; i != e; ++i){
            for(User::op_iterator opi = i->op_begin (), ope = i->op_end(); opi != ope; ++opi){
                Value *v = MapValue(*opi, VMap,  RF_None, 0);
                if (v != 0) { *opi = v; }
            }
            if (PHINode *pn = dyn_cast<PHINode>(i)) {
                for (unsigned j = 0, e = pn->getNumIncomingValues(); j != e; ++j) {
                    Value *v = MapValue(pn->getIncomingBlock(j), VMap, RF_None, 0);
                    if (v != 0) { pn->setIncomingBlock(j, cast<BasicBlock>(v)); }
                }
            }
            SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
            i->getAllMetadata(MDs);
            i->setDebugLoc(ji->getDebugLoc());
            ji++;
        }

        for (BasicBlock::iterator i = alter->begin(), e = alter->end() ; i != e; ++i){
            if (i->isBinaryOp()) {
                unsigned opcode = i->getOpcode();
                BinaryOperator *op, *op1 = NULL;

                Twine *var = new Twine("_");
                if (checkOpcodeIsInIntLogicCode(opcode)) {
                    for (int random = dis(gen) % 3; random < 10; ++random) {
                        switch (dis(gen) % 4) {
                        case 1:
                            op  = BinaryOperator::CreateNeg(i->getOperand(0), *var, &*i);
                            op1 = BinaryOperator::Create(Instruction::Add,               op, i->getOperand(1), "gen", &*i);
                            break;
                        case 2:
                            op1 = BinaryOperator::Create(Instruction::Sub, i->getOperand(0), i->getOperand(1),  *var, &*i);
                            op  = BinaryOperator::Create(Instruction::Mul,              op1, i->getOperand(1), "gen", &*i);
                            break;
                        case 3:
                            op  = BinaryOperator::Create(Instruction::Shl, i->getOperand(0), i->getOperand(1),  *var, &*i);
                            break;
                        }
                    }
                }
                if (checkOpcodeIsInFloatLogicCode(opcode)) {
                    for (int random = (int)dis(gen)%11; random < 10; ++random) {
                        switch (dis(gen)%2) {
                        case 1: 
                            op  = BinaryOperator::Create(Instruction::FSub, i->getOperand(0), i->getOperand(1), *var,  &*i);
                            op1 = BinaryOperator::Create(Instruction::FMul,               op, i->getOperand(1), "gen", &*i);
                        }
                    }
                }
                if(opcode == Instruction::ICmp){
                    ICmpInst *currentI = (ICmpInst*)(&i);
                    switch(dis(gen)%3){
                        case 1: currentI->swapOperands(); break;
                        case 2: currentI->setPredicate(ICmpOpCodeList[dis(gen) % 10]); break;
                    }

                }
                if (opcode == Instruction::FCmp) {
                    FCmpInst *currentI = (FCmpInst*)(&i);
                    switch (dis(gen)%3) {
                        case 1: currentI->swapOperands(); break;
                        case 2: currentI->setPredicate(FCmpOpCodeList[dis(gen) % 10]);  break;
                    }
                }
            }
        }
        return alter;
    } // end of createalterBasicBlock()

    void addBogusFlow(BasicBlock * basicBlock, Function &F){

        BasicBlock::iterator i1 = basicBlock->begin();
        if(basicBlock->getFirstNonPHIOrDbgOrLifetime())
            i1 = (BasicBlock::iterator)basicBlock->getFirstNonPHIOrDbgOrLifetime();
        Twine *var;
        var = new Twine("orignal");
        BasicBlock *original = basicBlock->splitBasicBlock(i1, *var);

        Twine * var3 = new Twine("alter");
        BasicBlock *alter = createalterBasicBlock(original, *var3, &F);

        alter->getTerminator()->eraseFromParent();
        basicBlock->getTerminator()->eraseFromParent();

        Value * LHS = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);
        Value * RHS = ConstantFP::get(Type::getFloatTy(F.getContext()), 1.0);

        Twine * var4 = new Twine("condition");
        FCmpInst * condition = new FCmpInst(*basicBlock, FCmpInst::FCMP_TRUE , LHS, RHS, *var4);

        BranchInst::Create(original, alter, (Value *)condition, basicBlock);

        BranchInst::Create(original, alter);

        BasicBlock::iterator i = original->end();

        Twine * var5 = new Twine("originalpart2");
        BasicBlock * originalpart2 = original->splitBasicBlock(--i , *var5);
        original->getTerminator()->eraseFromParent();
        Twine * var6 = new Twine("condition2");
        FCmpInst * condition2 = new FCmpInst(*original, CmpInst::FCMP_TRUE , LHS, RHS, *var6);
        BranchInst::Create(originalpart2, alter, (Value *)condition2, original);
    } 

    void bogus(Function &F) {
        int NumBasicBlocks = 0;
        if (ObfProbRate < 0 || ObfProbRate > 100) {
            ObfProbRate = defaultObfRate;
        }
        ObfTimes = ObfTimes <= 0 ? defaultObfTime : ObfTimes;

        int NumObfTimes = ObfTimes;
        do {
            std::list<BasicBlock *> basicBlocks;
            for (Function::iterator i = F.begin(); i != F.end(); ++i) {
                basicBlocks.push_back(&*i);
            }

            while (!basicBlocks.empty()) {
                NumBasicBlocks ++;
                if (dis(gen)%100 <= ObfProbRate) {
                    BasicBlock *basicBlock = basicBlocks.front();
                    addBogusFlow(basicBlock, F);
                }
                basicBlocks.pop_front();
            }
        } while (--NumObfTimes > 0);
    }

    bool insertOpaquePredict(Module &M){
        Twine * varX = new Twine("x");
        Value * x1 =ConstantInt::get(Type::getInt32Ty(M.getContext()), 0, false);

        GlobalVariable 	* x = new GlobalVariable(
            M,
            Type::getInt32Ty(M.getContext()),
            false,
            GlobalValue::CommonLinkage,
            (Constant * )x1,
            *varX
        );

        std::vector<Instruction*> toEdit, toDelete;
        BinaryOperator *op = NULL;
        LoadInst *opX;
        ICmpInst *condition;

        for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
            for (Function::iterator fi = mi->begin(), fe = mi->end(); fi != fe; ++fi) {
                Instruction * tbb= fi->getTerminator();
                if (tbb->getOpcode() == Instruction::Br) {
                    BranchInst * br = (BranchInst *)(tbb);
                    if (br->isConditional()) {
                        FCmpInst * cond = (FCmpInst *)br->getCondition();
                        unsigned opcode = cond->getOpcode();
                        if(opcode == Instruction::FCmp){
                            if (cond->getPredicate() == FCmpInst::FCMP_TRUE){
                                toDelete.push_back(cond);
                                toEdit.push_back(tbb);
                            }
                        }
                    }
                }
            }
        }
        for (std::vector<Instruction*>::iterator i =toEdit.begin();i!=toEdit.end();++i) {
            llvm::IntegerType * int32 = Type::getInt32Ty(M.getContext());
            opX = new LoadInst(int32, (Value *)x, "", (*i));
            
            op = BinaryOperator::Create(Instruction::Mul,       (Value *)opX,                      (Value *)opX, "", (*i));

            condition = new ICmpInst((*i), ICmpInst::ICMP_EQ,    op, ConstantInt::get(int32, 0, false));

            BranchInst::Create(
                ((BranchInst*)*i)->getSuccessor(0),
                ((BranchInst*)*i)->getSuccessor(1),
                (Value *) condition,
                ((BranchInst*)*i)->getParent()
            );

            (*i)->eraseFromParent();
        }
        for (std::vector<Instruction*>::iterator i = toDelete.begin();i != toDelete.end();++i){
            (*i)->eraseFromParent();
        }
        return true;
    }

    struct MyBogusControlFlow : PassInfoMixin<MyBogusControlFlow> {
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            if (ObfTimes <= 0) {
                return PreservedAnalyses::all();;
            }
            if ( !((ObfProbRate > 0) && (ObfProbRate <= 100))) {
                return PreservedAnalyses::all();;
            }
            bogus(F);
            insertOpaquePredict(*F.getParent());
            return PreservedAnalyses::all();
        }

        static bool isRequired() { return true; }
    };

    struct BogusControlFlow : public FunctionPass {
        static char ID; 
        BogusControlFlow() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F){
            if (ObfTimes <= 0) {return false;}
            if ( !((ObfProbRate > 0) && (ObfProbRate <= 100))) {return false;}
            bogus(F);
            insertOpaquePredict(*F.getParent());
            return true;
        }
    };
}

#include<iostream>
//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getBogusControlFlowPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "BogusFlow",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback (
                [] (StringRef Name,
                    FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>
                ) {
                    if (Name == "BogusFlow") {
                        FPM.addPass(MyBogusControlFlow());
                        return true;
                    }
                    return false;
                }
            );
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getBogusControlFlowPluginInfo();
}

char BogusControlFlow::ID = 0;
static RegisterPass<BogusControlFlow> X(
    "boguscf",
    "inserting bogus control flow",
    true, 
    false
);
