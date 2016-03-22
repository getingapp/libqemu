/*
 *  Libqemu accessor function support
 *
 * Generate inline load/store functions for one data size.
 *
 * Generate a store function as well as signed and unsigned loads.
 *
 * Not used directly but included from cpu_ldst.h.
 *
 *  Copyright (c) 2015 Jonas Zaddach
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#if DATA_SIZE == 8
#define SHIFT 3
#define SUFFIX q
#define USUFFIX q
#define DATA_TYPE uint64_t
#elif DATA_SIZE == 4
#define SHIFT 2
#define SUFFIX l
#define USUFFIX l
#define DATA_TYPE uint32_t
#elif DATA_SIZE == 2
#define SHIFT 1
#define SUFFIX w
#define USUFFIX uw
#define DATA_TYPE uint16_t
#define DATA_STYPE int16_t
#elif DATA_SIZE == 1
#define SHIFT 0
#define SUFFIX b
#define USUFFIX ub
#define DATA_TYPE uint8_t
#define DATA_STYPE int8_t
#else
#error unsupported data size
#endif

#if DATA_SIZE == 8
#define RES_TYPE uint64_t
#else
#define RES_TYPE uint32_t
#endif

#if defined(CODE_ACCESS)
#define CODE_ACCESS_FLAG MO_CODE
#else /* defined(CODE_ACCESS) */
#define CODE_ACCESS_FLAG 0
#endif /* defined(CODE_ACCESS) */

static inline RES_TYPE
glue(glue(cpu_ld, USUFFIX), MEMSUFFIX)(CPUArchState *env, target_ulong ptr)
{
    return (RES_TYPE) helper_libqemu_ld(env, ptr, SHIFT | CODE_ACCESS_FLAG, CPU_MMU_INDEX);
}

static inline RES_TYPE
glue(glue(glue(cpu_ld, USUFFIX), MEMSUFFIX), _ra)(CPUArchState *env,
                                                  target_ulong ptr,
                                                  uintptr_t retaddr)
{
    return (RES_TYPE) helper_libqemu_ld(env, ptr, SHIFT | CODE_ACCESS_FLAG, CPU_MMU_INDEX);
}

#if DATA_SIZE <= 2
static inline int
glue(glue(cpu_lds, SUFFIX), MEMSUFFIX)(CPUArchState *env, target_ulong ptr)
{
    return (RES_TYPE) helper_libqemu_ld(env, ptr, SHIFT | CODE_ACCESS_FLAG | MO_SIGN, CPU_MMU_INDEX);
}

static inline int
glue(glue(glue(cpu_lds, SUFFIX), MEMSUFFIX), _ra)(CPUArchState *env,
                                                  target_ulong ptr,
                                                  uintptr_t retaddr)
{
    return (RES_TYPE) helper_libqemu_ld(env, ptr, SHIFT | CODE_ACCESS_FLAG | MO_SIGN, CPU_MMU_INDEX);
}
#endif

#ifndef CODE_ACCESS
static inline void
glue(glue(cpu_st, SUFFIX), MEMSUFFIX)(CPUArchState *env, target_ulong ptr,
                                      RES_TYPE v)
{
    helper_libqemu_st(env, ptr, SHIFT, CPU_MMU_INDEX, v);
}

static inline void
glue(glue(glue(cpu_st, SUFFIX), MEMSUFFIX), _ra)(CPUArchState *env,
                                                  target_ulong ptr,
                                                  RES_TYPE v,
                                                  uintptr_t retaddr)
{
    helper_libqemu_st(env, ptr, SHIFT, CPU_MMU_INDEX, v);
}
#endif

#undef RES_TYPE
#undef DATA_TYPE
#undef DATA_STYPE
#undef SUFFIX
#undef USUFFIX
#undef DATA_SIZE
#undef SHIFT
#undef IS_CODE_ACCESS
