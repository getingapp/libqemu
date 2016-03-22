extern "C" {
#include "qemu-common.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "tcg/tcg.h"
}

#include "libqemu/qemu-lib-external.h"

static uint64_t sign_extend(uint64_t val, unsigned size)
{
	switch (size) {
		case MO_8:  return (val & 0x0000000000000080) ? (0xffffffffffffff00 | val) : val;
		case MO_16: return (val & 0x0000000000008000) ? (0xffffffffffff0000 | val) : val;
		case MO_32: return (val & 0x0000000080000000) ? (0xffffffff00000000 | val) : val;
		case MO_64: return val;
	}
	abort();
}

uint64_t helper_libqemu_ld(CPUArchState* env, uint64_t addr, uint32_t memop, uint32_t idx)
{
    if (libqemu_ld) {
        uint64_t val = libqemu_ld(env, addr, memop, idx);
        return val;
    }
    else {
        printf("mem_read(addr = 0x%08" PRIx32 ", size = %d, idx = %d)\n", (uint32_t) addr, (int) (memop & MO_SIZE), (int) idx);
        uint64_t v = 0;
        switch (memop & MO_BSWAP) {
            case MO_BE:
                switch (memop & MO_SIZE) {
                    case MO_8:  v = helper_ret_ldub_mmu(env, addr, idx, 0); break;
                    case MO_16: v = helper_be_lduw_mmu(env, addr, idx, 0); break;
                    case MO_32: v = helper_be_ldul_mmu(env, addr, idx, 0); break;
                    case MO_64: v = helper_be_ldq_mmu(env, addr, idx, 0); break;
                    default: abort();
                }
                break;
            case MO_LE:
                switch (memop & MO_SIZE) {
                    case MO_8:  v = helper_ret_ldub_mmu(env, addr, idx, 0); break;   
                    case MO_16: v = helper_le_lduw_mmu(env, addr, idx, 0); break;
                    case MO_32: v = helper_le_ldul_mmu(env, addr, idx, 0); break;
                    case MO_64: v = helper_le_ldq_mmu(env, addr, idx, 0); break;
                    default: abort();
                }
                break;
            default:
                abort();
        }

        if (memop & MO_SIGN) {
            v = sign_extend(v, memop & MO_SIZE);
        }
        return v;
    }
}

void helper_libqemu_st(CPUArchState* env, uint64_t addr, uint32_t memop, uint32_t idx, uint64_t val)
{
    if (libqemu_st) {
        libqemu_st(env, addr, memop, idx, val);
    }
    else {
        printf("mem_write(addr = 0x%08" PRIx32 ", size = %d, idx = %d, val = 0x%08" PRIx32 ")\n", (uint32_t) addr, (int) (memop & MO_SIZE), (int) idx, (uint32_t) val);
        switch (memop & MO_BSWAP) {
            case MO_BE:
                switch (memop & MO_SIZE) {
                    case MO_8:  helper_ret_stb_mmu(env, addr, val, idx, 0); break;
                    case MO_16: helper_be_stw_mmu(env, addr, val, idx, 0); break;
                    case MO_32: helper_be_stl_mmu(env, addr, val, idx, 0); break;
                    case MO_64: helper_be_stq_mmu(env, addr, val, idx, 0); break;
                    default: abort();
                }
                break;
            case MO_LE:
                switch (memop & MO_SIZE) {
                    case MO_8:  helper_ret_stb_mmu(env, addr, val, idx, 0); break;   
                    case MO_16: helper_le_stw_mmu(env, addr, val, idx, 0); break;
                    case MO_32: helper_le_stl_mmu(env, addr, val, idx, 0); break;
                    case MO_64: helper_le_stq_mmu(env, addr, val, idx, 0); break;
                    default: abort();
                }
                break;
            default:
                abort();
        }
    }
}


