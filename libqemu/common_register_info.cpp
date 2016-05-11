#include "libqemu/register_info.h"

#include "qemu-common.h"
#include "cpu.h"
#include "libqemu/tcg-llvm.h"
#include "libqemu/qemu-lib-external.h"
#include "libqemu/passes/CpuArchStructInfo.h"

#include <llvm/IR/Module.h>

extern RegisterInfo REGISTER_INFO[];
extern size_t REGISTER_INFO_NUM_ELEMENTS;

void initRegisterInfo(void)
{
    llvm::Module* mod = llvm::unwrap(libqemu_get_module());
    assert(mod && "libqemu needs to be initialized before this code");
    std::unique_ptr<StructInfo> cpuArchStructInfo = StructInfo::getFromGlobalPointer(mod, "cpu_type_anchor");

    for (size_t i = 0; i < REGISTER_INFO_NUM_ELEMENTS; ++i) {
        llvm::SmallVector<unsigned, MAX_NUM_REGISTER_INDICES> vecIndices;
        cpuArchStructInfo->findMember(REGISTER_INFO[i].offset, vecIndices);
        REGISTER_INFO[i].indices.count = vecIndices.size();
        for (size_t j = 0; j < REGISTER_INFO[i].indices.count; ++j) {
            REGISTER_INFO[i].indices.indices[j] = vecIndices[j];
        }
    }
}

static std::map<std::string, RegisterInfo const*> initNameMap(void)
{
    std::map<std::string, RegisterInfo const*> cache;
    for (size_t i = 0; i < REGISTER_INFO_NUM_ELEMENTS; ++i) {
        cache.insert(std::make_pair(REGISTER_INFO[i].name, &REGISTER_INFO[i]));
    }
    return cache;
}

static std::map<size_t, RegisterInfo const*> initOffsetMap(void)
{
    std::map<size_t, RegisterInfo const*> cache;
    for (size_t i = 0; i < REGISTER_INFO_NUM_ELEMENTS; ++i) {
        cache.insert(std::make_pair(REGISTER_INFO[i].offset, &REGISTER_INFO[i]));
    }
    return cache;
}

struct RegisterIndicesLess
{
    bool operator()(RegisterIndices const* a, RegisterIndices const* b) {
        if (a->count < b->count) {
            return true;
        }

        for (size_t i = 0; i < a->count; ++i) {
            if (a->indices[i] < b->indices[i]) {
                return true;
            }
        }

        return false;
    }
};

static std::map<RegisterIndices const*, RegisterInfo const*, RegisterIndicesLess> initIndicesMap(void)
{
    std::map<RegisterIndices const*, RegisterInfo const*, RegisterIndicesLess> cache;
    for (size_t i = 0; i < REGISTER_INFO_NUM_ELEMENTS; ++i) {
        cache.insert(std::make_pair(&REGISTER_INFO[i].indices, &REGISTER_INFO[i]));
    }
    return cache;
}

RegisterInfo const* libqemu_get_register_info_by_name(const char* name)
{
    static std::map<std::string, RegisterInfo const*> cache = initNameMap();
    auto itr = cache.find(name);
    if (itr != cache.end()) {
        return itr->second;
    }
    return nullptr;
}

RegisterInfo const* libqemu_get_register_info_by_offset(size_t offset)
{
    static std::map<size_t, RegisterInfo const*> cache = initOffsetMap();
    auto itr = cache.find(offset);
    if (itr != cache.end()) {
        return itr->second;
    }
    return nullptr;

}

RegisterInfo const* libqemu_get_register_info_by_indices(RegisterIndices const* indices)
{
    static std::map<RegisterIndices const*, RegisterInfo const*, RegisterIndicesLess> cache = initIndicesMap();
    auto itr = cache.find(indices);
    if (itr != cache.end()) {
        return itr->second;
    }
    return nullptr;
}
