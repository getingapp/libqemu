#include "libqemu/register_info.h"

#include "qemu-common.h"
#include "cpu.h"
#include "libqemu/tcg-llvm.h"
#include "libqemu/qemu-lib-external.h"
#include "libqemu/passes/CpuArchStructInfo.h"

#include <llvm/IR/Module.h>

#define I386_REG_INFO(name, struct_field) \
    {name, #struct_field, offsetof(CPUX86State, struct_field), {0, {0}}}

RegisterInfo REGISTER_INFO[] = {};
size_t REGISTER_INFO_NUM_ELEMENTS = sizeof(REGISTER_INFO) / sizeof(RegisterInfo);

RegisterInfo const* libqemu_get_register_info_pc(void)
{
    /* TODO: stub */
    return nullptr;
}

RegisterInfo const* libqemu_get_register_info_sp(void)
{
    /* TODO: stub */
    return nullptr;
}
