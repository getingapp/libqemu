#ifndef _QEMU_LIB_H
#define _QEMU_LIB_H

#ifdef __cplusplus
extern "C" {
#endif 

typedef uint64_t libqemu_load_handler(void *env, uint64_t ptr, unsigned size, bool is_signed, bool is_code);
typedef void libqemu_store_handler(void *env, uint64_t ptr, unsigned size, bool is_signed, bool is_code, uint64_t val);
extern libqemu_load_handler *libqemu_ld;
extern libqemu_store_handler *libqemu_st;

#ifdef __cplusplus
}
#endif

#endif /* _QEMU_LIB_H */
