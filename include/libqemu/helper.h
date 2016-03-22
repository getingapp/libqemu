#if defined(CONFIG_LIBQEMU)
DEF_HELPER_4(libqemu_ld, i64 /* val */, env, i64 /* addr */, i32 /* memop */, i32 /* idx */)
DEF_HELPER_5(libqemu_st, void, env, i64 /* addr */, i32 /* memop */, i32 /* idx */, i64 /* val */)
#endif /* defined(CONFIG_LIBQEMU) */
