#ifndef _LIBQEMU_REGISTERINFO_H
#define _LIBQEMU_REGISTERINFO_H

#include <stdlib.h>

#define MAX_NUM_REGISTER_INDICES 10

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct RegisterIndices
{
    size_t count;
    unsigned indices[MAX_NUM_REGISTER_INDICES];
} RegisterIndices;

typedef struct RegisterInfo
{
    const char* name;
    const char* qemu_name;
    size_t offset;
    RegisterIndices indices;
} RegisterInfo;

RegisterInfo const* libqemu_get_register_info_by_name(const char* name);
RegisterInfo const* libqemu_get_register_info_by_offset(size_t offset);
RegisterInfo const* libqemu_get_register_info_by_indices(RegisterIndices const* indices);
RegisterInfo const* libqemu_get_register_info_pc(void);
RegisterInfo const* libqemu_get_register_info_sp(void);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* _LIBQEMU_REGISTERINFO_H */
