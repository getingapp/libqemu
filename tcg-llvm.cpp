/*
 * S2E Selective Symbolic Execution Framework
 *
 * Copyright (c) 2010, Dependable Systems Laboratory, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Dependable Systems Laboratory, EPFL nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE DEPENDABLE SYSTEMS LABORATORY, EPFL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Currently maintained by:
 *    Volodymyr Kuznetsov <vova.kuznetsov@epfl.ch>
 *    Vitaly Chipounov <vitaly.chipounov@epfl.ch>
 *
 * All contributors are listed in the S2E-AUTHORS file.
 */


#include "libqemu/cxx11-compat.h"

extern "C" {
#include "tcg.h"
//#include "tcg-plugin-api.h"
}

#include <libqemu/tcg-llvm.h>
#include <libqemu/passes/CpuArchStructInfo.h>

extern "C" {
#include "config.h"
#include "qemu-common.h"
#include "disas/disas.h"

	//#if defined(CONFIG_SOFTMMU)

	//#include "../../softmmu_defs.h"


// static void *qemu_ld_helpers[5] = {
//     (void*) __ldb_mmu,
//     (void*) __ldw_mmu,
//     (void*) __ldl_mmu,
//     (void*) __ldq_mmu,
//     (void*) __ldq_mmu,
// };
//
// static void *qemu_st_helpers[5] = {
//     (void*) __stb_mmu,
//     (void*) __stw_mmu,
//     (void*) __stl_mmu,
//     (void*) __stq_mmu,
//     (void*) __stq_mmu,
// };

// #endif

}

#include <llvm/IR/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Threading.h>

#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>

#include <iostream>
#include <sstream>

using llvm::legacy::FunctionPassManager;
using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::ExecutionEngine;
using llvm::Function;
using llvm::Value;
using llvm::BasicBlock;
using llvm::IntegerType;
using llvm::PointerType;
using llvm::ConstantInt;
using llvm::dyn_cast;
using llvm::JITMemoryManager;
using llvm::GlobalValue;
using llvm::StringRef;
using llvm::getGlobalContext;
using llvm::InitializeNativeTarget;
using llvm::DataLayout;
using llvm::createReassociatePass;
using llvm::createConstantPropagationPass;
using llvm::createInstructionCombiningPass;
using llvm::createGVNPass;
using llvm::createDeadStoreEliminationPass;
using llvm::createCFGSimplificationPass;
using llvm::createPromoteMemoryToRegisterPass;
using llvm::isa;
using llvm::Constant;
using llvm::cast;
using llvm::FunctionType;
using llvm::sys::DynamicLibrary;
using llvm::ArrayRef;
using llvm::APInt;
using llvm::ReturnInst;
using llvm::raw_string_ostream;
using llvm::DataLayoutPass;
namespace Intrinsic = llvm::Intrinsic;



//TODO: Hack to make stuff compile
static const bool execute_llvm = false;

//#undef NDEBUG

extern "C" {
//    TCGLLVMContext* tcg_llvm_ctx = 0;

    /* These data is accessible from generated code */
//    TCGLLVMRuntime tcg_llvm_runtime = {
//        0, 0, {0,0,0}
//#ifndef CONFIG_S2E
//        , 0, 0, 0
//#endif
//    };

}



class TJITMemoryManager;

class TCGLLVMContextPrivate {
public:
    LLVMContext& m_context;
private:
    IRBuilder<> m_builder;
public:
    /* Current m_module */
    Module *m_module;
private:

    /* JIT engine */
    TJITMemoryManager *m_jitMemoryManager;
public:
    ExecutionEngine *m_executionEngine;
private:
    llvm::Type *m_archcpuPtrType;
    std::unique_ptr<StructInfo> m_cpuArchStructInfo;
    /* Function pass manager (used for optimizing the code) */
    FunctionPassManager *m_functionPassManager;

    Function* m_qemu_ld_helpers[5];
    Function* m_qemu_st_helpers[5];

    /* Count of generated translation blocks */
    int m_tbCount;

    /* XXX: The following members are "local" to generateCode method */

    /* TCGContext for current translation block */
    TCGContext* m_tcgContext;

    /* Function for current translation block */
    Function *m_tbFunction;

    /* Current temp m_values */
    Value* m_values[TCG_MAX_TEMPS];

    /* Pointers to in-memory versions of globals or local temps */
    Value* m_memValuesPtr[TCG_MAX_TEMPS];

    /* For reg-based globals, store argument number,
     * for mem-based globals, store base value index */
    int m_globalsIdx[TCG_MAX_TEMPS];

    std::map<TCGLabel *, BasicBlock*>  m_labels;

    void replaceEnvInstructionsWith(
        llvm::Value *newEnv,
        llvm::Value *oldEnv,
        llvm::Value *envUse,
        int offset,
        std::list<llvm::Instruction *>& eraseList);
    void replaceEnv(llvm::Function &f);
    void replaceEntryBB(llvm::Function &f);

public:
    TCGLLVMContextPrivate();
    ~TCGLLVMContextPrivate();

    void deleteExecutionEngine() {
        if (m_executionEngine) {
            delete m_executionEngine;
            m_executionEngine = NULL;
        }
    }

    FunctionPassManager *getFunctionPassManager() const {
        return m_functionPassManager;
    }

    /* Shortcuts */
    llvm::Type* intType(int w) { return IntegerType::get(m_context, w); }
    llvm::Type* intPtrType(int w) { return PointerType::get(intType(w), 0); }
    llvm::Type* wordType() { return intType(TCG_TARGET_REG_BITS); }
    llvm::Type* wordType(int bits) { return intType(bits); }
    llvm::Type* wordPtrType() { return intPtrType(TCG_TARGET_REG_BITS); }

    void adjustTypeSize(unsigned target, Value **v1) {
        Value *va = *v1;
        if (target == 32) {
            if (va->getType() == intType(64)) {
                *v1 = m_builder.CreateTrunc(va, intType(target));
            } else if (va->getType() != intType(32)) {
                assert(false);
            }
        }
    }

    //This handles the special case of symbolic values
    //assigned to the program counter register
    Value* handleSymbolicPcAssignment(Value *orig) {
        return orig;
    }

    void adjustTypeSize(unsigned target, Value **v1, Value **v2) {
        adjustTypeSize(target, v1);
        adjustTypeSize(target, v2);
    }

    llvm::Type* tcgType(int type) {
        return type == TCG_TYPE_I64 ? intType(64) : intType(32);
    }

    llvm::Type* tcgPtrType(int type) {
        return type == TCG_TYPE_I64 ? intPtrType(64) : intPtrType(32);
    }

    /* Helpers */
    Value* getValue(int idx);
    void setValue(int idx, Value *v);
    void delValue(int idx);

    Value* getPtrForValue(int idx);
    void delPtrForValue(int idx);
    void initGlobalsAndLocalTemps();
    unsigned getValueBits(int idx);

    void invalidateCachedMemory();

    uint64_t toInteger(Value *v) const {
        if (ConstantInt *cste = dyn_cast<ConstantInt>(v)) {
            return *cste->getValue().getRawData();
        }
        llvm::errs() << *v << '\n';
        assert(false && "Not a constant");
	return 0;
    }

    void initializeHelpers();

    BasicBlock* getLabel(TCGLabel *lbl);
    void delLabel(TCGLabel *lbl);
    void startNewBasicBlock(BasicBlock *bb = NULL);

    /* Code generation */
    Value* generateQemuMemOp(bool ld, Value *value, Value *addr,
                             int mem_index, int bits);

    void generateTraceCall(uintptr_t pc);
    int generateOperation(int opc, const TCGArg *args);

    inline Value *generateCondition(TCGCond cond, Value *arg1, Value *arg2);
    Value *generateAndC(TCGArg arg1, TCGArg arg2);

    void generateCode(TCGContext *s, TranslationBlock *tb);
};

/* Custom JITMemoryManager in order to capture the size of
 * the last generated function */
class TJITMemoryManager: public JITMemoryManager {
    JITMemoryManager* m_base;
    ptrdiff_t m_lastFunctionSize;
public:
    TJITMemoryManager():
        m_base(JITMemoryManager::CreateDefaultMemManager()),
        m_lastFunctionSize(0) {}
    ~TJITMemoryManager() { delete m_base; }

    ptrdiff_t getLastFunctionSize() const { return m_lastFunctionSize; }

    uint8_t *startFunctionBody(const Function *F, uintptr_t &ActualSize) {
        m_lastFunctionSize = 0;
        return m_base->startFunctionBody(F, ActualSize);
    }
    void endFunctionBody(const Function *F, uint8_t *FunctionStart,
                                uint8_t *FunctionEnd) {
        m_lastFunctionSize = FunctionEnd - FunctionStart;
        m_base->endFunctionBody(F, FunctionStart, FunctionEnd);
    }

    void setMemoryWritable() { m_base->setMemoryWritable(); }
    void setMemoryExecutable() { m_base->setMemoryExecutable(); }
    void setPoisonMemory(bool poison) { m_base->setPoisonMemory(poison); }
    void AllocateGOT() { m_base->AllocateGOT(); }
    uint8_t *getGOTBase() const { return m_base->getGOTBase(); }
    //void SetDlsymTable(void *ptr) { m_base->SetDlsymTable(ptr); }
    //void *getDlsymTable() const { return m_base->getDlsymTable(); }
    uint8_t *allocateStub(const GlobalValue* F, unsigned StubSize,
                                unsigned Alignment) {
        return m_base->allocateStub(F, StubSize, Alignment);
    }
    uint8_t *allocateSpace(intptr_t Size, unsigned Alignment) {
        return m_base->allocateSpace(Size, Alignment);
    }
    uint8_t *allocateGlobal(uintptr_t Size, unsigned Alignment) {
        return m_base->allocateGlobal(Size, Alignment);
    }
    uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                                 unsigned SectionID, StringRef SectionName) {
        return m_base->allocateCodeSection(Size, Alignment, SectionID, SectionName);
    }
    uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment,
                                 unsigned SectionID, StringRef SectionName, bool isReadOnly) {
        return m_base->allocateDataSection(Size, Alignment, SectionID, SectionName, isReadOnly);
    }
    void *getPointerToNamedFunction(const std::string &Name,
                                    bool AbortOnFailure = true) {
        return m_base->getPointerToNamedFunction(Name, AbortOnFailure);
    }
    //void deallocateMemForFunction(const Function *F) {
    //    m_base->deallocateMemForFunction(F);
    //}

    virtual void deallocateFunctionBody(void *Body) {
        m_base->deallocateFunctionBody(Body);
    }

//     uint8_t* startExceptionTable(const Function* F, uintptr_t &ActualSize) {
// //        return m_base->startExceptionTable(F, ActualSize);
//     }
//     void endExceptionTable(const Function *F, uint8_t *TableStart,
//                                  uint8_t *TableEnd, uint8_t* FrameRegister) {
//         m_base->endExceptionTable(F, TableStart, TableEnd, FrameRegister);
//     }
    // virtual void deallocateExceptionTable(void *Body) {
    //     m_base->deallocateExceptionTable(Body);
    // }
	virtual bool finalizeMemory(std::string *ErrMsg) {return m_base->finalizeMemory(ErrMsg);}
    bool CheckInvariants(std::string &ErrorStr) {
        return m_base->CheckInvariants(ErrorStr);
    }
    size_t GetDefaultCodeSlabSize() {
        return m_base->GetDefaultCodeSlabSize();
    }
    size_t GetDefaultDataSlabSize() {
        return m_base->GetDefaultDataSlabSize();
    }
    size_t GetDefaultStubSlabSize() {
        return m_base->GetDefaultStubSlabSize();
    }
    unsigned GetNumCodeSlabs() { return m_base->GetNumCodeSlabs(); }
    unsigned GetNumDataSlabs() { return m_base->GetNumDataSlabs(); }
    unsigned GetNumStubSlabs() { return m_base->GetNumStubSlabs(); }
};

extern const char _binary_helpers_bcm_start;
extern const char _binary_helpers_bcm_end;

TCGLLVMContextPrivate::TCGLLVMContextPrivate()
    : m_context(getGlobalContext()), m_builder(m_context), m_tbCount(0),
      m_tcgContext(NULL), m_tbFunction(NULL)
{
    std::memset(m_values, 0, sizeof(m_values));
    std::memset(m_memValuesPtr, 0, sizeof(m_memValuesPtr));
    std::memset(m_globalsIdx, 0, sizeof(m_globalsIdx));

    InitializeNativeTarget();

    llvm::SMDiagnostic smerror;
    m_module = llvm::ParseIR( 
        llvm::MemoryBuffer::getMemBuffer( 
            StringRef(&_binary_helpers_bcm_start, &_binary_helpers_bcm_end - &_binary_helpers_bcm_start)),
            smerror,
            m_context);

    m_archcpuPtrType = m_module->getGlobalVariable("cpu_type_anchor", false)->getType();
    m_cpuArchStructInfo = StructInfo::getFromGlobalPointer(m_module, "cpu_type_anchor");

//    m_module = new Module("tcg-llvm", m_context);

    m_jitMemoryManager = new TJITMemoryManager();

    std::string error;
    m_executionEngine = ExecutionEngine::createJIT(
            m_module, &error, m_jitMemoryManager);
    if(m_executionEngine == NULL) {
        std::cerr << "Unable to create LLVM JIT: " << error << std::endl;
        exit(1);
    }

    m_functionPassManager = new FunctionPassManager(m_module);
    m_functionPassManager->add(
            new DataLayoutPass(*m_executionEngine->getDataLayout()));

    m_functionPassManager->add(createReassociatePass());
    m_functionPassManager->add(createConstantPropagationPass());
    m_functionPassManager->add(createInstructionCombiningPass());
    m_functionPassManager->add(createGVNPass());
    m_functionPassManager->add(createDeadStoreEliminationPass());
    m_functionPassManager->add(createCFGSimplificationPass());
    m_functionPassManager->add(createPromoteMemoryToRegisterPass());

    //m_functionPassManager->add(new SelectRemovalPass());

    m_functionPassManager->doInitialization();
}

TCGLLVMContextPrivate::~TCGLLVMContextPrivate()
{
    delete m_functionPassManager;

    // the following line will also delete
    // m_moduleProvider, m_module and all its functions
    if (m_executionEngine) {
        delete m_executionEngine;
    }
}

void TCGLLVMContextPrivate::initializeHelpers()
{
//     m_helperTraceMemoryAccess =
//             m_module->getFunction("tcg_llvm_trace_memory_access");
//
//     m_helperTraceInstruction =
//             m_module->getFunction("tcg_llvm_trace_instruction");
//
//     m_helperForkAndConcretize =
//             m_module->getFunction("tcg_llvm_fork_and_concretize");
//
//     m_helperMakeSymbolic =
//             m_module->getFunction("tcg_llvm_make_symbolic");
//     m_helperGetValue =
//             m_module->getFunction("tcg_llvm_get_value");
//
     m_qemu_ld_helpers[0] = m_module->getFunction("_helper_ldb_mmu");
     m_qemu_ld_helpers[1] = m_module->getFunction("_helper_ldw_mmu");
     m_qemu_ld_helpers[2] = m_module->getFunction("_helper_ldl_mmu");
     m_qemu_ld_helpers[3] = m_module->getFunction("_helper_ldq_mmu");
     m_qemu_ld_helpers[4] = m_module->getFunction("_helper_ldq_mmu");

     m_qemu_st_helpers[0] = m_module->getFunction("_helper_stb_mmu");
     m_qemu_st_helpers[1] = m_module->getFunction("_helper_stw_mmu");
     m_qemu_st_helpers[2] = m_module->getFunction("_helper_stl_mmu");
     m_qemu_st_helpers[3] = m_module->getFunction("_helper_stq_mmu");
     m_qemu_st_helpers[4] = m_module->getFunction("_helper_stq_mmu");
//
//     assert(m_helperTraceMemoryAccess);
// // XXX: is this really not needed on ARM?
// #ifndef TARGET_ARM
//     assert(m_helperMakeSymbolic);
// #endif
//     assert(m_helperGetValue);
//     for(int i = 0; i < 5; ++i) {
//         assert(m_qemu_ld_helpers[i]);
//         assert(m_qemu_st_helpers[i]);
//     }
}

Value* TCGLLVMContextPrivate::getPtrForValue(int idx)
{
    TCGContext *s = m_tcgContext;
    TCGTemp &temp = s->temps[idx];

    assert(idx < s->nb_globals || s->temps[idx].temp_local);
    
    if(m_memValuesPtr[idx] == NULL) {
        assert(idx < s->nb_globals);

        if(temp.fixed_reg) {
            Value *v = m_builder.CreateConstGEP1_32(
                    m_tbFunction->arg_begin(), m_globalsIdx[idx]);
            m_memValuesPtr[idx] = m_builder.CreatePointerCast(
                    v, tcgPtrType(temp.type)
#ifndef NDEBUG
                    , StringRef(temp.name) + "_ptr"
#endif
                    );

        } else {
            Value *v = getValue(m_globalsIdx[idx]);
            assert(v->getType() == wordType());

            v = m_builder.CreateAdd(v, ConstantInt::get(
                            wordType(), temp.mem_offset));
            m_memValuesPtr[idx] =
                m_builder.CreateIntToPtr(v, tcgPtrType(temp.type)
#ifndef NDEBUG
                        , StringRef(temp.name) + "_ptr"
#endif
                        );
        }
    }

    return m_memValuesPtr[idx];
}

inline void TCGLLVMContextPrivate::delValue(int idx)
{
    /* XXX
    if(m_values[idx] && m_values[idx]->use_empty()) {
        if(!isa<Instruction>(m_values[idx]) ||
                !cast<Instruction>(m_values[idx])->getParent())
            delete m_values[idx];
    }
    */
    m_values[idx] = NULL;
}

inline void TCGLLVMContextPrivate::delPtrForValue(int idx)
{
    /* XXX
    if(m_memValuesPtr[idx] && m_memValuesPtr[idx]->use_empty()) {
        if(!isa<Instruction>(m_memValuesPtr[idx]) ||
                !cast<Instruction>(m_memValuesPtr[idx])->getParent())
            delete m_memValuesPtr[idx];
    }
    */
    m_memValuesPtr[idx] = NULL;
}

unsigned TCGLLVMContextPrivate::getValueBits(int idx)
{
    switch (m_tcgContext->temps[idx].type) {
        case TCG_TYPE_I32: return 32;
        case TCG_TYPE_I64: return 64;
        default: assert(false && "Unknown size");
    }
    return 0;
}

Value* TCGLLVMContextPrivate::getValue(int idx)
{
    if(m_values[idx] == NULL) {
        if(idx < m_tcgContext->nb_globals) {
            m_values[idx] = m_builder.CreateLoad(getPtrForValue(idx)
#ifndef NDEBUG
                    , StringRef(m_tcgContext->temps[idx].name) + "_v"
#endif
                    );
        } else if(m_tcgContext->temps[idx].temp_local) {
            m_values[idx] = m_builder.CreateLoad(getPtrForValue(idx));
#ifndef NDEBUG
            std::ostringstream name;
            name << "loc" << (idx - m_tcgContext->nb_globals) << "_v";
            m_values[idx]->setName(name.str());
#endif
        } else {
            // Temp value was not previousely assigned
            assert(false); // XXX: or return zero constant ?
        }
    }

    return m_values[idx];
}

void TCGLLVMContextPrivate::setValue(int idx, Value *v)
{
    delValue(idx);
    m_values[idx] = v;

    if(!v->hasName() && !isa<Constant>(v)) {
#ifndef NDEBUG
        if(idx < m_tcgContext->nb_globals)
            v->setName(StringRef(m_tcgContext->temps[idx].name) + "_v");
        if(m_tcgContext->temps[idx].temp_local) {
            std::ostringstream name;
            name << "loc" << (idx - m_tcgContext->nb_globals) << "_v";
            v->setName(name.str());
        } else {
            std::ostringstream name;
            name << "tmp" << (idx - m_tcgContext->nb_globals) << "_v";
            v->setName(name.str());
        }
#endif
    }

    if(idx < m_tcgContext->nb_globals) {
        // We need to save a global copy of a value
        m_builder.CreateStore(v, getPtrForValue(idx));

        if(m_tcgContext->temps[idx].fixed_reg) {
            /* Invalidate all dependent global vals and pointers */
            for(int i=0; i<m_tcgContext->nb_globals; ++i) {
                if(i != idx && !m_tcgContext->temps[idx].fixed_reg &&
                                    m_globalsIdx[i] == idx) {
                    delValue(i);
                    delPtrForValue(i);
                }
            }
        }
    } else if(m_tcgContext->temps[idx].temp_local) {
        // We need to save an in-memory copy of a value
        m_builder.CreateStore(v, getPtrForValue(idx));
    }
}

void TCGLLVMContextPrivate::initGlobalsAndLocalTemps()
{
    TCGContext *s = m_tcgContext;

    int reg_to_idx[TCG_TARGET_NB_REGS];
    for(int i=0; i<TCG_TARGET_NB_REGS; ++i)
        reg_to_idx[i] = -1;

    int argNumber = 0;
    for(int i=0; i<s->nb_globals; ++i) {
        if(s->temps[i].fixed_reg) {
            // This global is in fixed host register. We are
            // mapping such registers to function arguments
            m_globalsIdx[i] = argNumber++;
            reg_to_idx[s->temps[i].reg] = i;

        } else {
            // This global is in memory at (mem_reg + mem_offset).
            // Base value is not known yet, so just store mem_reg
            m_globalsIdx[i] = s->temps[i].mem_reg;
        }
    }

    // Map mem_reg to index for memory-based globals
    for(int i=0; i<s->nb_globals; ++i) {
        if(!s->temps[i].fixed_reg) {
            assert(reg_to_idx[m_globalsIdx[i]] >= 0);
            m_globalsIdx[i] = reg_to_idx[m_globalsIdx[i]];
        }
    }

    // Allocate local temps
    for(int i=s->nb_globals; i<TCG_MAX_TEMPS; ++i) {
        if(s->temps[i].temp_local) {
            std::ostringstream pName;
            pName << "loc_" << (i - s->nb_globals) << "ptr";
            m_memValuesPtr[i] = m_builder.CreateAlloca(
                tcgType(s->temps[i].type), 0, pName.str());
        }
    }
}

inline BasicBlock* TCGLLVMContextPrivate::getLabel(TCGLabel *lbl)
{
    if(m_labels.find(lbl) == m_labels.end()) {
        std::ostringstream bbName;
        bbName << "label_" << lbl->id;
        m_labels.insert(std::make_pair(lbl, BasicBlock::Create(m_context, bbName.str())));
    }

    return m_labels[lbl];
}

inline void TCGLLVMContextPrivate::delLabel(TCGLabel *lbl)
{
    /* XXX
    if(m_labels[idx] && m_labels[idx]->use_empty() &&
            !m_labels[idx]->getParent())
        delete m_labels[idx];
    */
    m_labels.erase(lbl);
}

void TCGLLVMContextPrivate::startNewBasicBlock(BasicBlock *bb)
{
    if(!bb)
        bb = BasicBlock::Create(m_context);
    else
        assert(bb->getParent() == 0);

    if(!m_builder.GetInsertBlock()->getTerminator())
        m_builder.CreateBr(bb);

    m_tbFunction->getBasicBlockList().push_back(bb);
    m_builder.SetInsertPoint(bb);

    /* Invalidate all temps */
    for(int i=0; i<TCG_MAX_TEMPS; ++i)
        delValue(i);

    /* Invalidate all pointers to globals */
    for(int i=0; i<m_tcgContext->nb_globals; ++i)
        delPtrForValue(i);
}

inline Value* TCGLLVMContextPrivate::generateQemuMemOp(bool ld,
        Value *value, Value *addr, int mem_index, int bits)
{
    assert(addr->getType() == intType(TARGET_LONG_BITS));
    assert(ld || value->getType() == intType(bits));
    assert(TCG_TARGET_REG_BITS == 64); //XXX

    //TOOD: Continue debugging here

#ifdef CONFIG_SOFTMMU

#if  defined(CONFIG_LLVM)
#if defined (CONFIG_S2E)
    if (execute_llvm) {
#endif
    uintptr_t helperFunc = ld ? (uint64_t) m_qemu_ld_helpers[bits>>4]:
                           (uint64_t) m_qemu_st_helpers[bits>>4];


    std::vector<Value*> argValues;
    argValues.reserve(3);
    argValues.push_back(addr);
    if(!ld)
        argValues.push_back(value);
    argValues.push_back(ConstantInt::get(intType(TARGET_LONG_BITS), mem_index));

    std::vector<llvm::Type*> argTypes;
    argTypes.reserve(3);
    for(int i=0; i<(ld?2:3); ++i)
        argTypes.push_back(argValues[i]->getType());

    llvm::Type* helperFunctionPtrTy = PointerType::get(
            FunctionType::get(
                    ld ? intType(bits) : llvm::Type::getVoidTy(m_context),
                    argTypes, false),
            0);

    Value* funcAddr = m_builder.CreateIntToPtr(
            ConstantInt::get(wordType(), helperFunc),
            helperFunctionPtrTy);
    return m_builder.CreateCall(funcAddr, ArrayRef<Value*>(argValues));
#else //CONFIG_LLVM
#error CONFIG_LLVM must be active if CONFIG_SOFTMMU is active to compile memory operations generation as
#endif //CONFIG_LLVM

#else // CONFIG_SOFTMMU
    addr = m_builder.CreateZExt(addr, wordType());
    addr = m_builder.CreateIntToPtr(addr, intPtrType(bits));
    if(ld) {
        return m_builder.CreateLoad(addr);
    } else {
        m_builder.CreateStore(value, addr);
        return NULL;
    }
#endif // CONFIG_SOFTMMU
	
	//TODO: Added during make-compile rush
	return NULL;
}

void TCGLLVMContextPrivate::generateTraceCall(uintptr_t pc)
{
#ifdef CONFIG_S2E
#if 0
    if (pc == 0x000f56e7 || pc == 0x000f52ff || pc == 0xf530c) {
         m_builder.CreateCall(m_helperTraceInstruction);
    }
#endif
#endif
}

Value *TCGLLVMContextPrivate::generateCondition(TCGCond cond, Value *arg1, Value *arg2)
{
    switch (cond)
    {
    case TCG_COND_NEVER:  return ConstantInt::getFalse(m_builder.getContext());
    case TCG_COND_ALWAYS: return ConstantInt::getTrue(m_builder.getContext());
    case TCG_COND_EQ:     return m_builder.CreateICmpEQ(arg1, arg2);
    case TCG_COND_NE:     return m_builder.CreateICmpNE(arg1, arg2);
            /* signed */
    case TCG_COND_LT:     return m_builder.CreateICmpSLT(arg1, arg2);
    case TCG_COND_GE:     return m_builder.CreateICmpSGE(arg1, arg2);
    case TCG_COND_LE:     return m_builder.CreateICmpSLE(arg1, arg2);
    case TCG_COND_GT:     return m_builder.CreateICmpSGT(arg1, arg2);
            /* unsigned */
    case TCG_COND_LTU:    return m_builder.CreateICmpULT(arg1, arg2);
    case TCG_COND_GEU:    return m_builder.CreateICmpUGE(arg1, arg2);
    case TCG_COND_LEU:    return m_builder.CreateICmpULE(arg1, arg2);
    case TCG_COND_GTU:    return m_builder.CreateICmpUGT(arg1, arg2);
    default:
        llvm::errs() << "ERROR: Unknown condition code " << cond << " in TCG code" << '\n';
        tcg_abort();
        return 0;
    }
}

int TCGLLVMContextPrivate::generateOperation(int opc, const TCGArg *args)
{
    Value *v;
    TCGOpDef &def = tcg_op_defs[opc];
    int nb_args = def.nb_args;

    switch(opc) {
    case INDEX_op_insn_start:
        break;

    /* predefined ops */
    case INDEX_op_discard:
        delValue(args[0]);
        break;

    case INDEX_op_call:
        {
            int nb_oargs = args[0] >> 16;
            int nb_iargs = args[0] & 0xffff;
            nb_args = nb_oargs + nb_iargs + def.nb_cargs + 1;

            //int flags = args[nb_oargs + nb_iargs + 1];
            //assert((flags & TCG_CALL_TYPE_MASK) == TCG_CALL_TYPE_STD);

            std::vector<Value*> argValues;
            std::vector<llvm::Type*> argTypes;
            argValues.reserve(nb_iargs-1);
            argTypes.reserve(nb_iargs-1);
            for(int i=0; i < nb_iargs-1; ++i) {
                TCGArg arg = args[nb_oargs + i + 1];
                if(arg != TCG_CALL_DUMMY_ARG) {
                    Value *v = getValue(arg);
                    argValues.push_back(v);
                    argTypes.push_back(v->getType());
                }
            }

            assert(nb_oargs == 0 || nb_oargs == 1);
            llvm::Type* retType = nb_oargs == 0 ?
                llvm::Type::getVoidTy(m_context) : wordType(getValueBits(args[1]));

            uintptr_t helperAddrC = args[nb_oargs + nb_iargs + 1];
            Value* result;

            if (!execute_llvm) {
                //Generate this in S2E mode
                assert(helperAddrC);

                const char *helperName = tcg_find_helper(m_tcgContext, helperAddrC);
                assert(helperName);

                std::string funcName = std::string("helper_") + helperName;
                Function* helperFunc = m_module->getFunction(funcName);
                if(!helperFunc) {
                    helperFunc = Function::Create(
                            FunctionType::get(retType, argTypes, false),
                            Function::PrivateLinkage, funcName, m_module);
                    m_executionEngine->addGlobalMapping(helperFunc,
                                                        (void*) helperAddrC);
                    /* XXX: Why do we need this ? */
                    DynamicLibrary::AddSymbol(funcName, (void*) helperAddrC);
                }

                result = m_builder.CreateCall(helperFunc,
                                              ArrayRef<Value*>(argValues));
            } else { //if (!execute_llvm)
                //Generate this in LLVM mode
                llvm::Type* helperFunctionPtrTy = PointerType::get(
                        FunctionType::get(retType, argTypes, false), 0);

                Value* funcAddr = m_builder.CreateIntToPtr(
                		//TODO: sizeof ugly here
                        ConstantInt::get(intType(sizeof(tcg_target_ulong) * 8), helperAddrC),
                        helperFunctionPtrTy);

                result = m_builder.CreateCall(funcAddr,
                                    ArrayRef<Value*>(argValues));
            }


            /* Invalidate in-memory values because
             * function might have changed them */
            for(int i=0; i<m_tcgContext->nb_globals; ++i)
                delValue(i);

            for(int i=m_tcgContext->nb_globals; i<TCG_MAX_TEMPS; ++i)
                if(m_tcgContext->temps[i].temp_local)
                    delValue(i);

            /* Invalidate all pointers to globals */
            for(int i=0; i<m_tcgContext->nb_globals; ++i)
                delPtrForValue(i);

            if(nb_oargs == 1)
                setValue(args[1], result);
        }
        break;

    case INDEX_op_br:
        m_builder.CreateBr(getLabel(arg_label(args[0])));
        startNewBasicBlock();
        break;

#define __OP_BRCOND_C(tcg_cond, cond)                               \
            case tcg_cond:                                          \
                v = m_builder.CreateICmp ## cond(                   \
                        getValue(args[0]), getValue(args[1]));      \
            break;

#define __OP_BRCOND(opc_name, bits)                                 \
    case opc_name: {                                                \
        assert(getValue(args[0])->getType() == intType(bits));      \
        assert(getValue(args[1])->getType() == intType(bits));      \
        switch(args[2]) {                                           \
            __OP_BRCOND_C(TCG_COND_EQ,   EQ)                        \
            __OP_BRCOND_C(TCG_COND_NE,   NE)                        \
            __OP_BRCOND_C(TCG_COND_LT,  SLT)                        \
            __OP_BRCOND_C(TCG_COND_GE,  SGE)                        \
            __OP_BRCOND_C(TCG_COND_LE,  SLE)                        \
            __OP_BRCOND_C(TCG_COND_GT,  SGT)                        \
            __OP_BRCOND_C(TCG_COND_LTU, ULT)                        \
            __OP_BRCOND_C(TCG_COND_GEU, UGE)                        \
            __OP_BRCOND_C(TCG_COND_LEU, ULE)                        \
            __OP_BRCOND_C(TCG_COND_GTU, UGT)                        \
            default:                                                \
                tcg_abort();                                        \
        }                                                           \
        BasicBlock* bb = BasicBlock::Create(m_context);             \
        m_builder.CreateCondBr(v, getLabel(arg_label(args[3])), bb);           \
        startNewBasicBlock(bb);                                     \
    } break;

    __OP_BRCOND(INDEX_op_brcond_i32, 32)

#if TCG_TARGET_REG_BITS == 64
    __OP_BRCOND(INDEX_op_brcond_i64, 64)
#endif

#undef __OP_BRCOND_C
#undef __OP_BRCOND

    case INDEX_op_set_label:
        assert(getLabel(arg_label(args[0]))->getParent() == 0);
        startNewBasicBlock(getLabel(arg_label(args[0])));
        break;

    case INDEX_op_movi_i32:
        setValue(args[0], ConstantInt::get(intType(32), args[1]));
        break;

    case INDEX_op_mov_i32:
        // Move operation may perform truncation of the value
        assert(getValue(args[1])->getType() == intType(32) ||
                getValue(args[1])->getType() == intType(64));
        setValue(args[0],
                m_builder.CreateTrunc(getValue(args[1]), intType(32)));
        break;

#if TCG_TARGET_REG_BITS == 64
    case INDEX_op_movi_i64:
        setValue(args[0], ConstantInt::get(intType(64), args[1]));
        break;

    case INDEX_op_mov_i64:
        assert(getValue(args[1])->getType() == intType(64));
        setValue(args[0], getValue(args[1]));
        break;
#endif

    /* size extensions */
#define __EXT_OP(opc_name, truncBits, opBits, signE )               \
    case opc_name:                                                  \
        /*                                                          \
        assert(getValue(args[1])->getType() == intType(opBits) ||   \
               getValue(args[1])->getType() == intType(truncBits)); \
        */                                                          \
        setValue(args[0], m_builder.Create ## signE ## Ext(         \
                m_builder.CreateTrunc(                              \
                    getValue(args[1]), intType(truncBits)),         \
                intType(opBits)));                                  \
        break;

    __EXT_OP(INDEX_op_ext8s_i32,   8, 32, S)
    __EXT_OP(INDEX_op_ext8u_i32,   8, 32, Z)
    __EXT_OP(INDEX_op_ext16s_i32, 16, 32, S)
    __EXT_OP(INDEX_op_ext16u_i32, 16, 32, Z)

#if TCG_TARGET_REG_BITS == 64
    __EXT_OP(INDEX_op_ext8s_i64,   8, 64, S)
    __EXT_OP(INDEX_op_ext8u_i64,   8, 64, Z)
    __EXT_OP(INDEX_op_ext16s_i64, 16, 64, S)
    __EXT_OP(INDEX_op_ext16u_i64, 16, 64, Z)
    __EXT_OP(INDEX_op_ext32s_i64, 32, 64, S)
    __EXT_OP(INDEX_op_ext32u_i64, 32, 64, Z)
#endif

#undef __EXT_OP

    /* load/store */
#define __LD_OP(opc_name, memBits, regBits, signE)                  \
    case opc_name:                                                  \
        assert(getValue(args[1])->getType() == wordType());         \
        v = m_builder.CreateAdd(getValue(args[1]),                  \
                    ConstantInt::get(wordType(), args[2]));         \
        v = m_builder.CreateIntToPtr(v, intPtrType(memBits));       \
        v = m_builder.CreateLoad(v);                                \
        setValue(args[0], m_builder.Create ## signE ## Ext(         \
                    v, intType(regBits)));                          \
        break;

//#ifdef TARGET_ARM

#define __ST_OP(opc_name, memBits, regBits)                         \
    case opc_name:  {                                                 \
        assert(getValue(args[0])->getType() == intType(regBits));   \
        assert(getValue(args[1])->getType() == wordType());         \
        Value* valueToStore = getValue(args[0]);                    \
                                                                    \
/*        if (TARGET_LONG_BITS == memBits && !execute_llvm            \
            && args[1] == 0                                         \
            && args[2] == offsetof(CPUARMState, regs[15])) {        \
            valueToStore = handleSymbolicPcAssignment(valueToStore);\
        }    */                                                       \
                                                                    \
        v = m_builder.CreateAdd(getValue(args[1]),                  \
                    ConstantInt::get(wordType(), args[2]));         \
        v = m_builder.CreateIntToPtr(v, intPtrType(memBits));       \
        m_builder.CreateStore(m_builder.CreateTrunc(                \
                valueToStore, intType(memBits)), v);           \
    } break;

    __LD_OP(INDEX_op_ld8u_i32,   8, 32, Z)
    __LD_OP(INDEX_op_ld8s_i32,   8, 32, S)
    __LD_OP(INDEX_op_ld16u_i32, 16, 32, Z)
    __LD_OP(INDEX_op_ld16s_i32, 16, 32, S)
    __LD_OP(INDEX_op_ld_i32,    32, 32, Z)

    __ST_OP(INDEX_op_st8_i32,   8, 32)
    __ST_OP(INDEX_op_st16_i32, 16, 32)
    __ST_OP(INDEX_op_st_i32,   32, 32)

#if TCG_TARGET_REG_BITS == 64
    __LD_OP(INDEX_op_ld8u_i64,   8, 64, Z)
    __LD_OP(INDEX_op_ld8s_i64,   8, 64, S)
    __LD_OP(INDEX_op_ld16u_i64, 16, 64, Z)
    __LD_OP(INDEX_op_ld16s_i64, 16, 64, S)
    __LD_OP(INDEX_op_ld32u_i64, 32, 64, Z)
    __LD_OP(INDEX_op_ld32s_i64, 32, 64, S)
    __LD_OP(INDEX_op_ld_i64,    64, 64, Z)

    __ST_OP(INDEX_op_st8_i64,   8, 64)
    __ST_OP(INDEX_op_st16_i64, 16, 64)
    __ST_OP(INDEX_op_st32_i64, 32, 64)
    __ST_OP(INDEX_op_st_i64,   64, 64)
#endif

#undef __LD_OP
#undef __ST_OP

    /* arith */
#define __ARITH_OP(opc_name, op, bits)                              \
    case opc_name: {                                                \
        Value *v1 = getValue(args[1]);                              \
        Value *v2 = getValue(args[2]);                              \
        adjustTypeSize(bits, &v1, &v2);                             \
        assert(v1->getType() == intType(bits));                     \
        assert(v2->getType() == intType(bits));                     \
        setValue(args[0], m_builder.Create ## op(v1, v2));          \
    } break;

#define __ARITH_OP_DIV2(opc_name, signE, bits)                      \
    case opc_name:                                                  \
        assert(getValue(args[2])->getType() == intType(bits));      \
        assert(getValue(args[3])->getType() == intType(bits));      \
        assert(getValue(args[4])->getType() == intType(bits));      \
        v = m_builder.CreateShl(                                    \
                m_builder.CreateZExt(                               \
                    getValue(args[3]), intType(bits*2)),            \
                m_builder.CreateZExt(                               \
                    ConstantInt::get(intType(bits), bits),          \
                    intType(bits*2)));                              \
        v = m_builder.CreateOr(v,                                   \
                m_builder.CreateZExt(                               \
                    getValue(args[2]), intType(bits*2)));           \
        setValue(args[0], m_builder.Create ## signE ## Div(         \
                v, getValue(args[4])));                             \
        setValue(args[1], m_builder.Create ## signE ## Rem(         \
                v, getValue(args[4])));                             \
        break;

#define __ARITH_OP_ROT(opc_name, op1, op2, bits)                    \
    case opc_name:                                                  \
        assert(getValue(args[1])->getType() == intType(bits));      \
        assert(getValue(args[2])->getType() == intType(bits));      \
        v = m_builder.CreateSub(                                    \
                ConstantInt::get(intType(bits), bits),              \
                getValue(args[2]));                                 \
        setValue(args[0], m_builder.CreateOr(                       \
                m_builder.Create ## op1 (                           \
                    getValue(args[1]), getValue(args[2])),          \
                m_builder.Create ## op2 (                           \
                    getValue(args[1]), v)));                        \
        break;

#define __ARITH_OP_I(opc_name, op, i, bits)                         \
    case opc_name:                                                  \
        assert(getValue(args[1])->getType() == intType(bits));      \
        setValue(args[0], m_builder.Create ## op(                   \
                    ConstantInt::get(intType(bits), i),             \
                    getValue(args[1])));                            \
        break;

#define __ARITH_OP_BSWAP(opc_name, sBits, bits)                     \
    case opc_name: {                                                \
        assert(getValue(args[1])->getType() == intType(bits));      \
        llvm::Type* Tys[] = { intType(sBits) };                     \
        Function *bswap = Intrinsic::getDeclaration(m_module,       \
                Intrinsic::bswap, ArrayRef<llvm::Type*>(Tys,1));                          \
        v = m_builder.CreateTrunc(getValue(args[1]),intType(sBits));\
        setValue(args[0], m_builder.CreateZExt(                     \
                m_builder.CreateCall(bswap, v), intType(bits)));    \
        } break;


    __ARITH_OP(INDEX_op_add_i32, Add, 32)
    __ARITH_OP(INDEX_op_sub_i32, Sub, 32)
    __ARITH_OP(INDEX_op_mul_i32, Mul, 32)

#ifdef TCG_TARGET_HAS_div_i32
    __ARITH_OP(INDEX_op_div_i32,  SDiv, 32)
    __ARITH_OP(INDEX_op_divu_i32, UDiv, 32)
    __ARITH_OP(INDEX_op_rem_i32,  SRem, 32)
    __ARITH_OP(INDEX_op_remu_i32, URem, 32)
#else
    __ARITH_OP_DIV2(INDEX_op_div2_i32,  S, 32)
    __ARITH_OP_DIV2(INDEX_op_divu2_i32, U, 32)
#endif

    __ARITH_OP(INDEX_op_and_i32, And, 32)
    __ARITH_OP(INDEX_op_or_i32,   Or, 32)
    __ARITH_OP(INDEX_op_xor_i32, Xor, 32)

    __ARITH_OP(INDEX_op_shl_i32,  Shl, 32)
    __ARITH_OP(INDEX_op_shr_i32, LShr, 32)
    __ARITH_OP(INDEX_op_sar_i32, AShr, 32)

    __ARITH_OP_ROT(INDEX_op_rotl_i32, Shl, LShr, 32)
    __ARITH_OP_ROT(INDEX_op_rotr_i32, LShr, Shl, 32)

    __ARITH_OP_I(INDEX_op_not_i32, Xor, (uint64_t) -1, 32)
    __ARITH_OP_I(INDEX_op_neg_i32, Sub, 0, 32)

    __ARITH_OP_BSWAP(INDEX_op_bswap16_i32, 16, 32)
    __ARITH_OP_BSWAP(INDEX_op_bswap32_i32, 32, 32)

#if TCG_TARGET_REG_BITS == 64
    __ARITH_OP(INDEX_op_add_i64, Add, 64)
    __ARITH_OP(INDEX_op_sub_i64, Sub, 64)
    __ARITH_OP(INDEX_op_mul_i64, Mul, 64)

#ifdef TCG_TARGET_HAS_div_i64
    __ARITH_OP(INDEX_op_div_i64,  SDiv, 64)
    __ARITH_OP(INDEX_op_divu_i64, UDiv, 64)
    __ARITH_OP(INDEX_op_rem_i64,  SRem, 64)
    __ARITH_OP(INDEX_op_remu_i64, URem, 64)
#else
    __ARITH_OP_DIV2(INDEX_op_div2_i64,  S, 64)
    __ARITH_OP_DIV2(INDEX_op_divu2_i64, U, 64)
#endif

    __ARITH_OP(INDEX_op_and_i64, And, 64)
    __ARITH_OP(INDEX_op_or_i64,   Or, 64)
    __ARITH_OP(INDEX_op_xor_i64, Xor, 64)

    __ARITH_OP(INDEX_op_shl_i64,  Shl, 64)
    __ARITH_OP(INDEX_op_shr_i64, LShr, 64)
    __ARITH_OP(INDEX_op_sar_i64, AShr, 64)

    __ARITH_OP_ROT(INDEX_op_rotl_i64, Shl, LShr, 64)
    __ARITH_OP_ROT(INDEX_op_rotr_i64, LShr, Shl, 64)

    __ARITH_OP_I(INDEX_op_not_i64, Xor, (uint64_t) -1, 64)
    __ARITH_OP_I(INDEX_op_neg_i64, Sub, 0, 64)

    __ARITH_OP_BSWAP(INDEX_op_bswap16_i64, 16, 64)
    __ARITH_OP_BSWAP(INDEX_op_bswap32_i64, 32, 64)
    __ARITH_OP_BSWAP(INDEX_op_bswap64_i64, 64, 64)
#endif

#undef __ARITH_OP_BSWAP
#undef __ARITH_OP_I
#undef __ARITH_OP_ROT
#undef __ARITH_OP_DIV2
#undef __ARITH_OP
        
    case INDEX_op_qemu_st_i32: {
        const int bitsize = (1 << (args[2] | MO_SIZE)) << 3;
        generateQemuMemOp(false,
            m_builder.CreateIntCast(
                getValue(args[0]), intType(bitsize), false), 
            getValue(args[1]), args[3], bitsize); 
        break;
    }
    case INDEX_op_qemu_st_i64: {
        //const int bitsize = (1 << (args[2] | MO_SIZE)) << 3;
        tcg_abort(); //TODO: Implement
        break;
    }
    case INDEX_op_qemu_ld_i32: {
        const int bitsize = (1 << (args[2] | MO_SIZE)) << 3;
        v = generateQemuMemOp(true, NULL,
            getValue(args[1]), args[3], bitsize);
        if (args[2] | MO_SIGN) {
            setValue(args[0], m_builder.CreateSExt(
                v, intType(std::max(TARGET_LONG_BITS, bitsize))));
        } 
        else {
            setValue(args[0], m_builder.CreateZExt(
                v, intType(std::max(TARGET_LONG_BITS, bitsize))));
        }
        break;
    }
    case INDEX_op_qemu_ld_i64: {
        //const int bitsize = (1 << (args[2] | MO_SIZE)) << 3;
        tcg_abort(); //TODO: Implement
        break;
    }

    case INDEX_op_exit_tb:
        m_builder.CreateRet(ConstantInt::get(wordType(), args[0]));
        break;

    case INDEX_op_goto_tb:
#ifdef CONFIG_S2E
        if (!execute_llvm) {
            m_builder.CreateStore(ConstantInt::get(intType(8), args[0]),
                    m_builder.CreateIntToPtr(ConstantInt::get(wordType(),
                        (uint64_t) &tcg_llvm_runtime.goto_tb),
                    intPtrType(8)));
        }
#endif
        /* XXX: tb linking is disabled */
        break;

    case INDEX_op_deposit_i32: {
        //llvm::errs() << *m_tbFunction << "\n";
        Value *arg1 = getValue(args[1]);
        //llvm::errs() << "arg1=" << *arg1 << "\n";
        //arg1 = m_builder.CreateTrunc(arg1, intType(32));


        Value *arg2 = getValue(args[2]);
        //llvm::errs() << "arg2=" << *arg2 << "\n";
        arg2 = m_builder.CreateTrunc(arg2, intType(32));

        uint32_t ofs = args[3];
        uint32_t len = args[4];

        if (ofs == 0 && len == 32) {
            setValue(args[0], arg2);
            break;
        }

        uint32_t mask = (1u << len) - 1;
        Value *t1, *ret;
        if (ofs + len < 32) {
            t1 = m_builder.CreateAnd(arg2, APInt(32, mask));
            t1 = m_builder.CreateShl(t1, APInt(32, ofs));
        } else {
            t1 = m_builder.CreateShl(arg2, APInt(32, ofs));
        }

        ret = m_builder.CreateAnd(arg1, APInt(32, ~(mask << ofs)));
        ret = m_builder.CreateOr(ret, t1);
        setValue(args[0], ret);
    }
    break;
#if TCG_TARGET_REG_BITS == 64
    case INDEX_op_deposit_i64: {
        Value *arg1 = getValue(args[1]);
        Value *arg2 = getValue(args[2]);
        arg2 = m_builder.CreateTrunc(arg2, intType(64));

        uint32_t ofs = args[3];
        uint32_t len = args[4];

        if (0 == ofs && 64 == len) {
            setValue(args[0], arg2);
            break;
        }

        uint64_t mask = (1u << len) - 1;
        Value *t1, *ret;

        if (ofs + len < 64) {
            t1 = m_builder.CreateAnd(arg2, APInt(64, mask));
            t1 = m_builder.CreateShl(t1, APInt(64, ofs));
        } else {
            t1 = m_builder.CreateShl(arg2, APInt(64, ofs));
        }

        ret = m_builder.CreateAnd(arg1, APInt(64, ~(mask << ofs)));
        ret = m_builder.CreateOr(ret, t1);
        setValue(args[0], ret);
    }
    break;
#endif
    case INDEX_op_setcond_i32: {
        Value *result = generateCondition(static_cast<TCGCond>(args[3]), getValue(args[1]), getValue(args[2]));
        setValue(args[0], m_builder.CreateSExt(result, intType(32)));
        break;
    }
    case INDEX_op_add2_i32: {
        Value *al = getValue(args[2]);
        Value *ah = getValue(args[3]);
        Value *bl = getValue(args[4]);
        Value *bh = getValue(args[5]);

        Value *a = m_builder.CreateOr(
                m_builder.CreateShl(
                        m_builder.CreateZExt(ah, intType(64)),
                        ConstantInt::get(intType(64), 32)),
                m_builder.CreateZExt(al, intType(64)));
        Value *b = m_builder.CreateOr(
                m_builder.CreateShl(
                    m_builder.CreateZExt(bh, intType(64)),
                    ConstantInt::get(intType(64), 32)),
                m_builder.CreateZExt(bl, intType(64)));

        Value *result = m_builder.CreateAdd(a, b);
        Value *rl = m_builder.CreateTrunc(result, intType(32));
        Value *rh = m_builder.CreateTrunc(
                m_builder.CreateLShr(result, ConstantInt::get(intType(64), 32)),
                intType(32));
        setValue(args[0], rl);
        setValue(args[1], rh);
        break;
    }
    case INDEX_op_movcond_i32: {
        Value *t0 = m_builder.CreateSExt(
                generateCondition(static_cast<TCGCond>(args[5]), getValue(args[1]), getValue(args[2])),
                intType(32));
        Value *v1 = getValue(args[3]);
        Value *v2 = getValue(args[4]);

        Value *not_t0 = m_builder.CreateNeg(t0);
        Value *t1 = m_builder.CreateAnd(v1, not_t0);
        Value *ret = m_builder.CreateAnd(v2, t0);
        setValue(args[0], m_builder.CreateOr(ret, t1));
        break;
    }

    default:
        std::cerr << "ERROR: unknown TCG micro operation '"
                  << def.name << "'" << std::endl;
        tcg_abort();
        break;
    }

    return nb_args;
}

void TCGLLVMContextPrivate::generateCode(TCGContext *s, TranslationBlock *tb)
{
	/* Create new function for current translation block */
    /* TODO: compute the checksum of the tb to see if we can reuse some code */
    std::ostringstream fName;
    fName << "tcg-llvm-tb-" << (m_tbCount++) << "-" << std::hex << tb->pc;

    /*
    if(m_tbFunction)
        m_tbFunction->eraseFromParent();
    */
    FunctionType *tbFunctionType = FunctionType::get(
        wordType(),
        std::vector<llvm::Type*>(1, dyn_cast<llvm::PointerType>(m_archcpuPtrType)->getElementType()), false);

    m_tbFunction = Function::Create(tbFunctionType,
            Function::PrivateLinkage, fName.str(), m_module);
    BasicBlock *basicBlock = BasicBlock::Create(m_context,
            "entry", m_tbFunction);
    m_builder.SetInsertPoint(basicBlock);

    m_tcgContext = s;

    /* Prepare globals and temps information */
    initGlobalsAndLocalTemps();

    /* Generate code for each opc */
    const TCGArg *args = s->gen_opparam_buf;
    TCGOp *op;
    for ( int opc_idx = s->gen_first_op_idx; opc_idx >= 0; opc_idx = op->next) {
	op = &s->gen_op_buf[opc_idx];
        TCGOpcode opc = op->opc;

        if(opc == INDEX_op_insn_start) {
//            // volatile store of current OPC index
//            m_builder.CreateStore(ConstantInt::get(wordType(), opc_idx),
//                m_builder.CreateIntToPtr(
//                    ConstantInt::get(wordType(),
//                        (uint64_t) &tcg_llvm_runtime.last_opc_index),
//                    wordPtrType()),
//                true);
//            // volatile store of current PC
//            m_builder.CreateStore(ConstantInt::get(wordType(), args[0]),
//                m_builder.CreateIntToPtr(
//                    ConstantInt::get(wordType(),
//                        (uint64_t) &tcg_llvm_runtime.last_pc),
//                    wordPtrType()),
//                true);
        }

        generateTraceCall(tb->pc);
        args += generateOperation(opc, args);
    }

    /* Finalize function */
    if(!isa<ReturnInst>(m_tbFunction->back().back()))
        m_builder.CreateRet(ConstantInt::get(wordType(), 0));

    /* Clean up unused m_values */
    for(int i=0; i<TCG_MAX_TEMPS; ++i)
        delValue(i);

    /* Delete pointers after deleting values */
    for(int i=0; i<TCG_MAX_TEMPS; ++i)
        delPtrForValue(i);

    m_labels.clear(); 
   // for(int i=0; i<TCG_MAX_LABELS; ++i)
   //     delLabel(i);

#ifndef NDEBUG
    verifyFunction(*m_tbFunction);
#endif

    //KLEE will optimize the function later
    //m_functionPassManager->run(*m_tbFunction);
    
    assert(tb->opaque);

    static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function = m_tbFunction;

    if(execute_llvm) {// || qemu_loglevel_mask(CPU_LOG_LLVM_ASM)) {
        static_cast<TCGPluginTBData *>(tb->opaque)->llvm_tc_ptr = (uint8_t*)
                m_executionEngine->getPointerToFunction(m_tbFunction);
        static_cast<TCGPluginTBData *>(tb->opaque)->llvm_tc_end = 
            static_cast<TCGPluginTBData *>(tb->opaque)->llvm_tc_ptr +
                m_jitMemoryManager->getLastFunctionSize();
    } else {
        static_cast<TCGPluginTBData *>(tb->opaque)->llvm_tc_ptr = 0;
        static_cast<TCGPluginTBData *>(tb->opaque)->llvm_tc_end = 0;
    }

    replaceEnv(*m_tbFunction);
    replaceEntryBB(*m_tbFunction);
    

//    std::string fcnString;
//    raw_string_ostream ss(fcnString);

//    ss << *m_tbFunction;
//    std::cout << "LLVM Function: " << fcnString << std::endl;
    /*
#ifdef DEBUG_DISAS
    if (unlikely(qemu_loglevel_mask(CPU_LOG_TB_OP))) {
        qemu_log("OP:\n");
        tcg_dump_ops(s, logfile);
        qemu_log("\n");
    }
#endif

    if(qemu_loglevel_mask(CPU_LOG_LLVM_IR)) {
        std::string fcnString;
        llvm::raw_string_ostream s(fcnString);
        s << *m_tbFunction;
        qemu_log("OUT (LLVM IR):\n");
        qemu_log("%s", s.str().c_str());
        qemu_log("\n");
        qemu_log_flush();
    } */
}

void TCGLLVMContextPrivate::replaceEntryBB(llvm::Function &f)
{
    llvm::BasicBlock *bb = f.begin();
    llvm::BranchInst *branch = dyn_cast<llvm::BranchInst>(bb->getTerminator());
    assert(branch);
    branch->getSuccessor(0)->eraseFromParent();
    bb->eraseFromParent();
}


void TCGLLVMContextPrivate::replaceEnvInstructionsWith(
        llvm::Value *newEnv,
        llvm::Value *oldEnv,
        llvm::Value *envUse,
        int offset,
        std::list<llvm::Instruction *>& eraseList)
{
    //Data flows through binary operations
    if (llvm::BinaryOperator *binOp = dyn_cast<llvm::BinaryOperator>(envUse)) {
        llvm::ConstantInt *op = dyn_cast<llvm::ConstantInt>(binOp->getOperand(1));
        if (!op)  {
            op = dyn_cast<ConstantInt>(binOp->getOperand(0));
            assert(op); //One of the operands has to be constant
        }

        switch (binOp->getOpcode())  {
            case llvm::BinaryOperator::Add: offset += op->getSExtValue(); break;
            case llvm::BinaryOperator::Sub: offset -= op->getSExtValue(); break;
            default: assert(false); //This operator not implemented yet
        }

        for ( Value::use_iterator useI = binOp->use_begin(), useE = binOp->use_end(); useI != useE; ++useI )
        {
            replaceEnvInstructionsWith(newEnv, binOp, *useI, offset, eraseList);
        }

        eraseList.push_back(binOp);
    }
    //Data flows through cast instruction
    else if (llvm::CastInst *cast = dyn_cast<llvm::CastInst>(envUse)) {
        //Cast does not change the offset, just recurse
        for ( Value::use_iterator useI = cast->use_begin(), useE = cast->use_end(); useI != useE; ++useI )
        {
            replaceEnvInstructionsWith(newEnv, cast, *useI, offset, eraseList);
        }

        eraseList.push_back(cast);
    }
    //Load instructions are a dataflow sink
    else if (llvm::LoadInst *load = dyn_cast<llvm::LoadInst>(envUse)) {
        llvm::SmallVector<unsigned, 10> indices;
        if (!m_cpuArchStructInfo->findMember(offset, indices)) {
            llvm::errs() << "Error finding member at offset " << offset << '\n';
            exit(1);
        }
        std::string memberName = m_cpuArchStructInfo->getMemberName(indices);

        llvm::SmallVector<Value *, 11> gepIndices;
        gepIndices.push_back(ConstantInt::get(llvm::Type::getInt32Ty(newEnv->getContext()), 0));
        for (unsigned index : indices) {
            gepIndices.push_back(ConstantInt::get(llvm::Type::getInt32Ty(newEnv->getContext()), index));
        }

        llvm::GetElementPtrInst *gep = llvm::GetElementPtrInst::Create(newEnv, gepIndices, memberName + "_ptr", load);
        llvm::LoadInst *newLoad = new llvm::LoadInst(gep, memberName, load);
        if (newLoad->getType()->isPointerTy())  {
            llvm::CastInst *cast = llvm::CastInst::CreatePointerCast(newLoad, load->getType(), memberName, load);
            load->replaceAllUsesWith(cast);
        }
        else {
            assert(newLoad->getType() == load->getType()); //Replacement needs to have the same type as previous value
            load->replaceAllUsesWith(newLoad);
        }

        LLVMContext& ctx = newLoad->getContext();
        newLoad->setMetadata("tcg-llvm.env_access.indices", llvm::MDNode::get(ctx, gepIndices));
        newLoad->setMetadata("tcg-llvm.env_access.member_name", llvm::MDNode::get(ctx, llvm::MDString::get(ctx, memberName)));

        eraseList.push_back(load);
    }
    //Store instructions are a dataflow sink
    else if (llvm::StoreInst *store = dyn_cast<llvm::StoreInst>(envUse)) {
        llvm::SmallVector<unsigned, 10> indices;
        if (!m_cpuArchStructInfo->findMember(offset, indices)) {
            llvm::errs() << "Error finding member at offset " << offset << '\n';
            exit(1);
        }
        std::string memberName = m_cpuArchStructInfo->getMemberName(indices);

        llvm::SmallVector<Value *, 11> gepIndices;
        gepIndices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(newEnv->getContext()), 0));
        for (unsigned index : indices) {
            gepIndices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(newEnv->getContext()), index));
        }

        llvm::GetElementPtrInst *gep = llvm::GetElementPtrInst::Create(newEnv, gepIndices, memberName + "_ptr", store);
        llvm::Value *storeValue = store->getValueOperand();
        if (gep->getType()->getElementType()->isPointerTy())  {
            storeValue = new llvm::IntToPtrInst(storeValue, gep->getType()->getElementType(), storeValue->getName(), store);
        }
        llvm::StoreInst *newStore = new llvm::StoreInst(storeValue, gep, store);

        llvm::LLVMContext& ctx = newStore->getContext();
        newStore->setMetadata("tcg-llvm.env_access.indices", llvm::MDNode::get(ctx, gepIndices));
        newStore->setMetadata("tcg-llvm.env_access.member_name", llvm::MDNode::get(ctx, llvm::MDString::get(ctx, memberName)));

        eraseList.push_back(store);
    }
    //Call instructions are a dataflow sink
    else if (llvm::CallInst *call = dyn_cast<llvm::CallInst>(envUse)) {
        for ( unsigned i = 0; i < call->getNumArgOperands(); ++i )
        {
            llvm::Value *op = call->getArgOperand(i);
            if (op == oldEnv)  {
                llvm::CastInst *cast = llvm::CastInst::CreatePointerCast(newEnv, op->getType(), op->getName(), call);
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

void TCGLLVMContextPrivate::replaceEnv(Function &f)
{
    std::list<llvm::Instruction *> eraseList;

    llvm::Argument *envPtr = f.arg_begin();

    for ( llvm::GlobalVariable::use_iterator useI = envPtr->use_begin(), useE = envPtr->use_end(); useI != useE; ++useI )
    {
        //Go over first gep and load of env pointer
        llvm::GetElementPtrInst *gep = dyn_cast<llvm::GetElementPtrInst>(*useI);
        if (!gep || gep->getNumIndices() != 1 || gep->getNumUses() != 1) {
            assert(false);
            continue;
        }
        llvm::ConstantInt *idx = dyn_cast<llvm::ConstantInt>(gep->idx_begin());
        if (!idx || idx->getZExtValue() != 0) {
            assert(false);
            continue;
        }

        llvm::CastInst *cast = dyn_cast<llvm::CastInst>(*gep->use_begin());
        if (!cast || cast->getNumUses() != 1) {
            assert(false);
            continue;
        }
        llvm::LoadInst *loadInst = dyn_cast<llvm::LoadInst>(*cast->use_begin());
        if (!loadInst) {
            assert(false);
            continue;
        }

        //Now replace following uses
        std::list<llvm::Instruction *> uses;
        for ( llvm::Instruction::use_iterator useI2 = loadInst->use_begin(), useE2 = loadInst->use_end(); useI2 != useE2; ++useI2 )
        {
            llvm::Instruction *useInst = dyn_cast<llvm::Instruction>(*useI2);
            if (!useInst) {
                assert(useInst && "Use is not an instruction");
                continue;
            }
            uses.push_back(useInst);
        }

        for ( llvm::Instruction *useInst : uses )
        {
            replaceEnvInstructionsWith(envPtr, loadInst, useInst, 0, eraseList);
        }

        eraseList.push_back(loadInst);
        eraseList.push_back(cast);
        eraseList.push_back(gep);
    }

    for ( llvm::Instruction *inst : eraseList )
    {
        inst->eraseFromParent();
    }
}


/***********************************/
/* External interface for C++ code */

TCGLLVMContext::TCGLLVMContext()
        : m_private(new TCGLLVMContextPrivate())
{
}

TCGLLVMContext::~TCGLLVMContext()
{
    delete m_private;
}

FunctionPassManager* TCGLLVMContext::getFunctionPassManager() const
{
    return m_private->getFunctionPassManager();
}

void TCGLLVMContext::deleteExecutionEngine()
{
    m_private->deleteExecutionEngine();
}

LLVMContext& TCGLLVMContext::getLLVMContext()
{
    return m_private->m_context;
}

Module* TCGLLVMContext::getModule()
{
    return m_private->m_module;
}

ExecutionEngine* TCGLLVMContext::getExecutionEngine()
{
    return m_private->m_executionEngine;
}

#ifdef CONFIG_S2E
void TCGLLVMContext::initializeHelpers()
{
    return m_private->initializeHelpers();
}
#endif

void TCGLLVMContext::generateCode(TCGContext *s, TranslationBlock *tb)
{
    assert(tb->opaque == NULL);
//    assert(static_cast<TCGPluginTBData *>(tb->opaque)->tcg_llvm_context == NULL);
//    assert(static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function == NULL);
    
    TCGPluginTBData *plugin_data = new TCGPluginTBData();
    tb->opaque = static_cast<void *>(plugin_data);
    plugin_data->tcg_llvm_context = this;

    m_private->generateCode(s, tb);
}

/*****************************/
/* Functions for QEMU c code */

TCGLLVMContext* tcg_llvm_initialize()
{
    // if (!llvm_start_multithreaded()) {
    //     fprintf(stderr, "Could not initialize LLVM threading\n");
    //     exit(-1);
    // }
    return new TCGLLVMContext;
}

void tcg_llvm_close(TCGLLVMContext *l)
{
    delete l;
}

void tcg_llvm_gen_code(TCGLLVMContext *l, TCGContext *s, TranslationBlock *tb)
{
    l->generateCode(s, tb);
}

void tcg_llvm_tb_alloc(TranslationBlock *tb)
{
    assert(tb->opaque);
    static_cast<TCGPluginTBData *>(tb->opaque)->tcg_llvm_context = NULL;
    static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function = NULL;
}

void tcg_llvm_tb_free(TranslationBlock *tb)
{
    assert(tb->opaque);
    if(static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function) {
        static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function->eraseFromParent();
    }
}

const char* tcg_llvm_get_func_name(TranslationBlock *tb)
{
    static char buf[64];
    if(static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function) {
        strncpy(buf, static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function->getName().str().c_str(), sizeof(buf));
    } else {
        buf[0] = 0;
    }
    return buf;
}


