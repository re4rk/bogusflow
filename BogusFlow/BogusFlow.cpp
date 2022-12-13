//=============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//    Visits all functions in a module, prints their names and the number of
//    arguments via stderr. Strictly speaking, this is an analysis pass (i.e.
//    the functions are not modified). However, in order to keep things simple
//    there's no 'print' method here (every analysis pass should implement it).
//
// USAGE:
//    1. Legacy PM
//      opt -enable-new-pm=0 -load libHelloWorld.dylib -legacy-hello-world -disable-output `\`
//        <input-llvm-file>
//    2. New PM
//      opt -load-pass-plugin=libHelloWorld.dylib -passes="hello-world" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "../include/BogusControlFlow.h"

using namespace llvm;

namespace {

struct HelloWorld : PassInfoMixin<HelloWorld> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    std::list<BasicBlock *> basicBlocks;
    for (Function::iterator i=F.begin();i!=F.end();++i) {
      basicBlocks.push_back(&*i);
      BasicBlock *basicBlock = basicBlocks.front();
      addBogusFlow(basicBlock, F);
    }
    doF(*F.getParent());

    return PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

struct LegacyHelloWorld : public FunctionPass {
  static char ID;
  LegacyHelloWorld() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override {
    std::list<BasicBlock *> basicBlocks;
    for (Function::iterator i=F.begin();i!=F.end();++i) {
      basicBlocks.push_back(&*i);
      BasicBlock *basicBlock = basicBlocks.front();
    }
    return false;
  }
};


} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION,
    "bogusflow",
    LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback (
        [] ( StringRef Name,
            FunctionPassManager &FPM,
            ArrayRef<PassBuilder::PipelineElement>
        ) {
          if (Name == "bogus-flow") {
            FPM.addPass(HelloWorld());
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
  return getHelloWorldPluginInfo();
}

char LegacyHelloWorld::ID = 0;

static RegisterPass<LegacyHelloWorld>
    X("legacy-bogus-flow", "Bogus Flow Pass",
      true, 
      false
    );
