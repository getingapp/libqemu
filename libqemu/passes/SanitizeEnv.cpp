#include "SanitizeEnv.h"
#include "CpuArchStructInfo.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include "llvm/DebugInfo.h"
#include <llvm/IR/DataLayout.h>

#include "llvm/Transforms/Utils/Cloning.h"

#include <unordered_map>
#include <list>
#include <cassert>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

using namespace llvm;

char SanitizeEnvPass::ID = 0;
static RegisterPass<SanitizeEnvPass> X("s2esanitizeenv", "Convert untyped accesses to env "
        "to typed CPUArchStruct accesses", false, false);

SanitizeEnvPass::SanitizeEnvPass() : llvm::ModulePass(ID)  {
}

static void replaceEnvInstructionsWith(
        Value *newEnv,
        Value *oldEnv,
        Value *envUse,
        int offset,
        std::list<Instruction *>& eraseList,
        CpuArchStructInfoPass& cpuStructInfo)
{
    //Data flows through binary operations
    if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(envUse)) {
        ConstantInt *op = dyn_cast<ConstantInt>(binOp->getOperand(1));
        if (!op)  {
            op = dyn_cast<ConstantInt>(binOp->getOperand(0));
            assert(op); //One of the operands has to be constant
        }

        switch (binOp->getOpcode())  {
            case BinaryOperator::Add: offset += op->getSExtValue(); break;
            case BinaryOperator::Sub: offset -= op->getSExtValue(); break;
            default: assert(false); //This operator not implemented yet
        }

        for ( Value::use_iterator useI = binOp->use_begin(), useE = binOp->use_end(); useI != useE; ++useI )
        {
            replaceEnvInstructionsWith(newEnv, binOp, *useI, offset, eraseList, cpuStructInfo);
        }

        eraseList.push_back(binOp);
    }
    //Data flows through cast instruction
    else if (CastInst *cast = dyn_cast<CastInst>(envUse)) {
        //Cast does not change the offset, just recurse
        for ( Value::use_iterator useI = cast->use_begin(), useE = cast->use_end(); useI != useE; ++useI )
        {
            replaceEnvInstructionsWith(newEnv, cast, *useI, offset, eraseList, cpuStructInfo);
        }

        eraseList.push_back(cast);
    }
    //Load instructions are a dataflow sink
    else if (LoadInst *load = dyn_cast<LoadInst>(envUse)) {
        SmallVector<unsigned, 10> indices;
        if (!cpuStructInfo.findMember(offset, indices)) {
            llvm::errs() << "Error finding member at offset " << offset << '\n';
            exit(1);
        }
        std::string memberName = cpuStructInfo.getMemberName(indices);

        SmallVector<Value *, 11> gepIndices;
        gepIndices.push_back(ConstantInt::get(Type::getInt32Ty(newEnv->getContext()), 0));
        for (unsigned index : indices) {
            gepIndices.push_back(ConstantInt::get(Type::getInt32Ty(newEnv->getContext()), index));
        }

        GetElementPtrInst *gep = GetElementPtrInst::Create(newEnv, gepIndices, memberName + "_ptr", load);
        LoadInst *newLoad = new LoadInst(gep, memberName, load);
        if (newLoad->getType()->isPointerTy())  {
            CastInst *cast = CastInst::CreatePointerCast(newLoad, load->getType(), memberName, load);
            load->replaceAllUsesWith(cast);
        }
        else {
            assert(newLoad->getType() == load->getType()); //Replacement needs to have the same type as previous value
            load->replaceAllUsesWith(newLoad);
        }

        LLVMContext& ctx = newLoad->getContext();
        newLoad->setMetadata("tcg-llvm.env_access.indices", MDNode::get(ctx, gepIndices));
        newLoad->setMetadata("tcg-llvm.env_access.member_name", MDNode::get(ctx, MDString::get(ctx, memberName)));

        eraseList.push_back(load);
    }
    //Store instructions are a dataflow sink
    else if (StoreInst *store = dyn_cast<StoreInst>(envUse)) {
        SmallVector<unsigned, 10> indices;
        if (!cpuStructInfo.findMember(offset, indices)) {
            llvm::errs() << "Error finding member at offset " << offset << '\n';
            exit(1);
        }
        std::string memberName = cpuStructInfo.getMemberName(indices);

        SmallVector<Value *, 11> gepIndices;
        gepIndices.push_back(ConstantInt::get(Type::getInt32Ty(newEnv->getContext()), 0));
        for (unsigned index : indices) {
            gepIndices.push_back(ConstantInt::get(Type::getInt32Ty(newEnv->getContext()), index));
        }

        GetElementPtrInst *gep = GetElementPtrInst::Create(newEnv, gepIndices, memberName + "_ptr", store);
        Value *storeValue = store->getValueOperand();
        if (gep->getType()->getElementType()->isPointerTy())  {
            storeValue = new IntToPtrInst(storeValue, gep->getType()->getElementType(), storeValue->getName(), store);
        }
        StoreInst *newStore = new StoreInst(storeValue, gep, store);

        LLVMContext& ctx = newStore->getContext();
        newStore->setMetadata("tcg-llvm.env_access.indices", MDNode::get(ctx, gepIndices));
        newStore->setMetadata("tcg-llvm.env_access.member_name", MDNode::get(ctx, MDString::get(ctx, memberName)));

        eraseList.push_back(store);
    }
    //Call instructions are a dataflow sink
    else if (CallInst *call = dyn_cast<CallInst>(envUse)) {
        for ( unsigned i = 0; i < call->getNumArgOperands(); ++i )
        {
            Value *op = call->getArgOperand(i);
            if (op == oldEnv)  {
                CastInst *cast = CastInst::CreatePointerCast(newEnv, op->getType(), op->getName(), call);
                call->setArgOperand(i, cast);
                break;
            }
        }
    }
    else {
        llvm::errs() << "Don't know how to handle " << *envUse << '\n';
        assert(false);
    }
}

static void replaceEnv(Function *oldFunction, Function *newFunction, Value *oldArg, CpuArchStructInfoPass& cpuStructInfo)
{
    std::list<Instruction *> eraseList;

    Argument *newEnvPtr = newFunction->arg_begin();

    for ( GlobalVariable::use_iterator useI = oldArg->use_begin(), useE = oldArg->use_end(); useI != useE; ++useI )
    {
        //Go over first gep and load of env pointer
        GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*useI);
        assert(gep);
        assert(gep->getNumIndices() == 1);
        ConstantInt *idx = dyn_cast<ConstantInt>(gep->idx_begin());
        if (!idx || idx->getZExtValue() != 0) {
            assert(false);
        }

        assert(gep->getNumUses() == 1);
        LoadInst *loadInst = dyn_cast<LoadInst>(*gep->use_begin());
        assert(loadInst);

        //Now replace following uses
        std::list<Instruction *> uses;
        for ( Instruction::use_iterator useI2 = loadInst->use_begin(), useE2 = loadInst->use_end(); useI2 != useE2; ++useI2 )
        {
            Instruction *useInst = dyn_cast<Instruction>(*useI2);
            assert(useInst && "Use is not an instruction");
            uses.push_back(useInst);
        }

        for (Instruction *useInst : uses )
        {
            replaceEnvInstructionsWith(newEnvPtr, loadInst, useInst, 0, eraseList, cpuStructInfo);
        }

        eraseList.push_back(loadInst);
        eraseList.push_back(gep);
    }

    for (Instruction *inst : eraseList)
    {
        inst->eraseFromParent();
    }
}

bool SanitizeEnvPass::runOnModule(Module &mod) {
	LLVMContext& ctx = mod.getContext();
	assert(mod.getNamedGlobal("env"));
	Type* envPtrType = mod.getNamedGlobal("env")->getType()->getElementType();
	assert(envPtrType);

	CpuArchStructInfoPass& cpuStructInfo = getAnalysis<CpuArchStructInfoPass>();

	SmallVector<Type *, 1> argTypes;
	argTypes.push_back(envPtrType);
	FunctionType *functionType = FunctionType::get(Type::getInt64Ty(ctx), argTypes, false);

	std::list<Function *> eraseList;
    for ( Function& f : mod )
    {
        if (!f.getName().startswith("tcg-llvm-tb-")) {
            continue;
        }

        assert(f.arg_size() == 1);

        std::string newFunctionName("transformed-" + f.getName().str());
        Function *newFunction = dyn_cast<Function>(mod.getOrInsertFunction(newFunctionName, functionType));

        //Dummy global variable that takes references to the old function's argument while we remodel the function
        GlobalVariable *dummyArg = new GlobalVariable(Type::getInt64Ty(ctx), false, GlobalValue::CommonLinkage, nullptr, "dummy-" + f.getName());

        ValueToValueMapTy vMap;
        SmallVector<ReturnInst*, 10> returns;
        ClonedCodeInfo codeInfo;

        vMap.insert(std::make_pair(f.arg_begin(), dummyArg));

        CloneFunctionInto(newFunction, &f, vMap, false, returns);

        replaceEnv(&f, newFunction, dummyArg, cpuStructInfo);

        eraseList.push_back(&f);
//        dummyArg->eraseFromParent();
    }

    for (Function *f : eraseList)
    {
        f->eraseFromParent();
    }

    std::vector<std::string> fixFunctions = {"s2e_dummyTbFunction", "helper_s2e_tcg_execution_handler", "helper_set_cp15", "helper_get_cp15"};

    for (std::string const& fixFunction : fixFunctions )
    {
        Function *func = mod.getFunction(fixFunction);

        func->deleteBody();
    }

    return true;
}

void SanitizeEnvPass::getAnalysisUsage(AnalysisUsage& usage) const {
    usage.addPreserved<CpuArchStructInfoPass>();
	usage.addRequired<CpuArchStructInfoPass>();
}

