#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"



/**
 * This pass converts accesses to the env pointer
 * (which is passed to every basic block function) to
 * properly typed CPUArchStruct accesses.
 */
  struct SanitizeEnvPass : public llvm::ModulePass {
    static char ID;

    SanitizeEnvPass();

    virtual bool runOnModule(llvm::Module &mod);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& usage) const;
  private:
  };
