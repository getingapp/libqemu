#include "libqemu/register_info.h"

#include "qemu-common.h"
#include "cpu.h"
#include "libqemu/tcg-llvm.h"
#include "libqemu/qemu-lib-external.h"
#include "libqemu/passes/CpuArchStructInfo.h"

#include <llvm/IR/Module.h>

#define ARM_REG_INFO(name, struct_field) \
    {name, #struct_field, offsetof(CPUARMState, struct_field), {0, {0}}}

RegisterInfo REGISTER_INFO[] = {
    ARM_REG_INFO("r0", regs[0]),
    ARM_REG_INFO("r1", regs[1]),
    ARM_REG_INFO("r2", regs[2]),
    ARM_REG_INFO("r3", regs[3]),
    ARM_REG_INFO("r4", regs[4]),
    ARM_REG_INFO("r5", regs[5]),
    ARM_REG_INFO("r6", regs[6]),
    ARM_REG_INFO("r7", regs[7]),
    ARM_REG_INFO("r8", regs[8]),
    ARM_REG_INFO("r9", regs[9]),
    ARM_REG_INFO("r10", regs[10]),
    ARM_REG_INFO("r11", regs[11]),
    ARM_REG_INFO("r12", regs[12]),
    ARM_REG_INFO("sp", regs[13]),
    ARM_REG_INFO("lr", regs[14]),
    ARM_REG_INFO("pc", regs[15]),
    ARM_REG_INFO("spsr", spsr)};
size_t REGISTER_INFO_NUM_ELEMENTS = sizeof(REGISTER_INFO) / sizeof(RegisterInfo);

RegisterInfo const* libqemu_get_register_info_pc(void)
{
    return libqemu_get_register_info_by_offset(offsetof(CPUARMState, regs[15]));
}

RegisterInfo const* libqemu_get_register_info_sp(void)
{
    return libqemu_get_register_info_by_offset(offsetof(CPUARMState, regs[13]));
}
