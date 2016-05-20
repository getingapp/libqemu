#include "libqemu/passes/CpuArchStructInfo.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include <llvm/IR/DataLayout.h>

#include "llvm/Transforms/Utils/Cloning.h"

#include <unordered_map>
#include <list>
#include <cassert>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <libqemu/cxx11-compat.h>

extern "C" {
#include "cpu.h"
}

using namespace llvm;

char CpuArchStructInfoPass::ID = 0;
static RegisterPass<CpuArchStructInfoPass> X("cpuarchstructinfo", "Get information about the CPUArchStruct", false, true);
static DITypeIdentifierMap typeIdentifierMap;

std::unique_ptr<StructInfo> StructInfo::getFromGlobalPointer(Module *module, llvm::StringRef name)
{
    GlobalVariable *var = module->getGlobalVariable(name, false);
    if (!var || !var->getType() || !var->getType()->isPointerTy()) {
        assert(false);
        llvm::errs() << "StructInfo: Cannot get global variable " << name << ", or it is not a pointer." << '\n';
        return nullptr;
    }
    PointerType *varDeref = dyn_cast<PointerType>(var->getType()->getElementType());
    if (!varDeref || !varDeref->getElementType()) 
    {
        assert(false);
        llvm::errs() << "StructInfo: Pointer not valid." << '\n';
        return nullptr;
    }

    StructType *structType = dyn_cast<StructType>(varDeref->getElementType());
    if (!structType) {
        assert(false);
        llvm::errs() << "StructInfo: Cannot get struct type." << '\n';
        return nullptr;
    }

    NamedMDNode *mdCuNodes = module->getNamedMetadata("llvm.dbg.cu");
    if (!mdCuNodes) {
        assert(false);
        llvm::errs() << "StructInfo: Cannot find metadata." << '\n';
        return nullptr;
    }

    std::shared_ptr<DITypeIdentifierMap> typeIdentifierMap(new DITypeIdentifierMap(generateDITypeIdentifierMap(mdCuNodes)));
    DICompositeType *diStructType = nullptr;
    for ( unsigned i = 0; i < mdCuNodes->getNumOperands() && !diStructType; ++i )
    {
        DICompileUnit diCu(mdCuNodes->getOperand(i));

        for ( unsigned j = 0; j < diCu.getGlobalVariables().getNumElements(); ++j )
        {
            DIGlobalVariable diGlobalVar(diCu.getGlobalVariables().getElement(j));
            if (diGlobalVar.getName() != name)  {
                continue;
            }

            //Go through pointers until we reach a structure
            DIType diStructType(diGlobalVar.getType().resolve(*typeIdentifierMap));
            while (diStructType.isDerivedType() && !diStructType.isCompositeType()) {
                diStructType = std::unique_ptr<DIDerivedType>(new DIDerivedType(diStructType))->getTypeDerivedFrom().resolve(*typeIdentifierMap);
            }

            if (!diStructType.isCompositeType()) {
                llvm::errs() << "StructInfo: Global variable " << name << " does not point to a composite type: " << diStructType.getName() << '\n';
                assert(false);
                return nullptr;
            }

            return std::unique_ptr<StructInfo>(new StructInfo(
                module, 
                structType, 
                new DICompositeType(diStructType), 
                typeIdentifierMap));
        }
    }

    assert(false);
    llvm::errs() << "StructInfo: Did not find global variable " << name << " in debug information." << '\n';
    return nullptr;

}

static std::unique_ptr<StructInfo> getCpuArchStructInfo(Module *module)
{
    GlobalVariable *env = module->getGlobalVariable("cpuarchstruct_type_anchor", false);
    assert(env);
    assert(env->getType() && env->getType()->isPointerTy());
    assert(env->getType()->getElementType() && env->getType()->getElementType()->isPointerTy());
    PointerType *envDeref = dyn_cast<PointerType>(env->getType()->getElementType());
    assert(envDeref && envDeref->getElementType() && envDeref->getElementType()->isStructTy());

    StructType *structType = dyn_cast<StructType>(envDeref->getElementType());
    assert(structType);

    NamedMDNode *mdCuNodes = module->getNamedMetadata("llvm.dbg.cu");
    if (!mdCuNodes) {
        return nullptr;
    }
    
    std::shared_ptr<DITypeIdentifierMap> typeIdentifierMap(new DITypeIdentifierMap(generateDITypeIdentifierMap(mdCuNodes)));
     

    DICompositeType *diStructType = nullptr;
    for ( unsigned i = 0; i < mdCuNodes->getNumOperands() && !diStructType; ++i )
    {
        DICompileUnit diCu(mdCuNodes->getOperand(i));

        for ( unsigned j = 0; j < diCu.getGlobalVariables().getNumElements(); ++j )
        {
            DIGlobalVariable diGlobalVar(diCu.getGlobalVariables().getElement(j));
            if (diGlobalVar.getName() != "cpuarchstruct_type_anchor")  {
                continue;
            }

            assert(diGlobalVar.getType().resolve(*typeIdentifierMap).isDerivedType());
            DIDerivedType diEnvPtrType(diGlobalVar.getType().resolve(*typeIdentifierMap));
            assert(diEnvPtrType.getTypeDerivedFrom().resolve(*typeIdentifierMap).isCompositeType());
            return std::unique_ptr<StructInfo>(new StructInfo(module, structType, new DICompositeType(diEnvPtrType.getTypeDerivedFrom().resolve(*typeIdentifierMap)), typeIdentifierMap));
        }
    }

    llvm::errs() << "WARNING: Debug information for struct CPUArchState not found" << '\n';
    return nullptr;
}

StructInfo::StructInfo(Module *module, StructType *structType, DICompositeType *diStructType, std::shared_ptr<DITypeIdentifierMap> typeIdentifierMap) :
        m_module(module),
        m_dataLayout(module),
        m_structType(structType),
        m_debugInfoStructType(diStructType),
        m_typeIdentifierMap(typeIdentifierMap)
{
}

bool StructInfo::findMember(int offset, SmallVectorImpl<unsigned>& indices)
{
   llvm::Type *curType = m_structType;

    offset += ENV_OFFSET;

    bool foundType;
    do
    {
        foundType = false;

        if (StructType *structType = dyn_cast<StructType>(curType)) {
            StructLayout const *layout = m_dataLayout.getStructLayout(structType);
            unsigned index = layout->getElementContainingOffset(offset);
            curType = structType->getContainedType(index);
            offset -= layout->getElementOffset(index);
            indices.push_back(index);
            foundType = true;
        }

        while (ArrayType *arrayType = dyn_cast<ArrayType>(curType))
        {
            unsigned elementSize = m_dataLayout.getTypeSizeInBits(arrayType->getElementType()) / 8;
            unsigned index = offset / elementSize;
            curType = arrayType->getElementType();
            offset -= index * elementSize;
            indices.push_back(index);
            foundType = true;
        }
    } while (foundType);

    if (offset != 0) {
        assert(false && "Offset pointing in the middle of a basic data type");
        return false;
    }

    return true;
}

bool StructInfo::findMember(llvm::StringRef name, SmallVectorImpl<unsigned>& indices)
{
//    Type *curType = m_structType;

    for (unsigned i = 0; i < m_debugInfoStructType->getTypeArray()->getNumOperands(); ++i)
    {
        DIDerivedType diMemberType(m_debugInfoStructType->getTypeArray().getElement(i));
        if (diMemberType.getName() == name) {
            indices.push_back(i);
            return true;
        }
    }

    return false;
}

std::string StructInfo::getName()
{
    if (m_debugInfoStructType) {
        return m_debugInfoStructType->getName();
    }
    else {
        return "<no name>";
    }
}

std::string StructInfo::getMemberName(ArrayRef<unsigned> indices)
{
    if (!m_debugInfoStructType) {
        return "<no name>";
    }

    llvm::Type *curType = m_structType;
    DIType diType = *m_debugInfoStructType;
    std::stringstream ss;

    ss << getName();

    for (unsigned index : indices)
    {
        if (StructType *structType = dyn_cast<StructType>(curType)) {
            if (diType.isDerivedType() && !diType.isCompositeType()) {
                diType = DIDerivedType(diType).getTypeDerivedFrom().resolve(*m_typeIdentifierMap); 
            }
            if (!diType.isCompositeType()) {
                llvm::errs() << "ERROR: C type is a StructType, but DWARF debug information is not a composite type." << '\n';
                assert(false);
                exit(1);
            }
            DICompositeType diCompositeType(diType);
            DIDerivedType diMemberType(diCompositeType.getTypeArray().getElement(index));

            ss << "." << diMemberType.getName().str();

            curType = structType->getContainedType(index);
            diType = diMemberType.getTypeDerivedFrom().resolve(*m_typeIdentifierMap);
        }
        else if (ArrayType *arrayType = dyn_cast<ArrayType>(curType)) {
            assert(diType.isDerivedType());
            //TODO
            DIDerivedType diDerivedType(diType);
            ss << "[" << std::dec << index << "]";
            curType = arrayType->getElementType();
            diType = diDerivedType.getTypeDerivedFrom().resolve(*m_typeIdentifierMap);
        }
        else {
            assert(false && "Offset pointing in the middle of a basic data type");
            return "<error>";
        }
    }

    return ss.str();
}

llvm::Type* StructInfo::getMemberType(ArrayRef<unsigned> indices)
{
    llvm::Type *curType = m_structType;

    for (unsigned index : indices)
    {
        if (StructType *structType = dyn_cast<StructType>(curType)) {
            curType = structType->getContainedType(index);
        }
        else if (ArrayType *arrayType = dyn_cast<ArrayType>(curType)) {
            curType = arrayType->getElementType();
        }
        else {
            assert(false && "Offset pointing in the middle of a basic data type");
            return nullptr;
        }
    }

    return curType;
}


CpuArchStructInfoPass::CpuArchStructInfoPass() :
        llvm::ModulePass(ID),
        m_cpuArchStructInfo(nullptr)
{
}

bool CpuArchStructInfoPass::runOnModule(Module &mod)
{
    m_cpuArchStructInfo = getCpuArchStructInfo(&mod);
    return false;
}

void CpuArchStructInfoPass::getAnalysisUsage(AnalysisUsage& usage) const {
	usage.setPreservesAll();
}

bool CpuArchStructInfoPass::findMember(unsigned offset, llvm::SmallVectorImpl<unsigned>& indices) {
    auto itr = m_cachedOffsetLookups.find(offset);
    if (itr != m_cachedOffsetLookups.end()) {
        for ( unsigned index : itr->second ) {
            indices.push_back(index);
        }
        return true;
    }

    assert(m_cpuArchStructInfo);
    if (m_cpuArchStructInfo->findMember(offset, indices)) {
        SmallVectorImpl<unsigned>& cachedIndices = m_cachedOffsetLookups[offset];
        for (unsigned index : indices) {
            cachedIndices.push_back(index);
        }
        return true;
    }

    return false;
}

bool CpuArchStructInfoPass::findMember(llvm::StringRef name, llvm::SmallVectorImpl<unsigned>& indices) {
    auto itr = m_cachedNameLookups.find(name);
    if (itr != m_cachedNameLookups.end()) {
        for ( unsigned index : itr->second ) {
            indices.push_back(index);
        }
        return true;
    }

    assert(m_cpuArchStructInfo);
    if (m_cpuArchStructInfo->findMember(name, indices)) {
        SmallVectorImpl<unsigned>& cachedIndices = m_cachedNameLookups[name];
        for (unsigned index : indices) {
            cachedIndices.push_back(index);
        }
        return true;
    }

    return false;
}

