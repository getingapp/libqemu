#include <libqemu/qemu-lib.h>



extern "C" {
#include "qemu-common.h"
#include <exec/cpu-common.h>
#include <sysemu/cpus.h>
#include <qemu/main-loop.h>
#include <qemu/error-report.h>
#include <qemu.h>
//#include <hw/boards.h>

//extern int singlestep;
//extern unsigned long mmap_min_addr;
//extern THREAD CPUState *thread_cpu;
//extern unsigned long reserved_va;
}

int singlestep;
unsigned long mmap_min_addr;
THREAD CPUState *thread_cpu;
unsigned long reserved_va;

int libqemu_init(void)
{
    const char *cpu_model = (const char *) NULL;
//    CPUArchState *env;
    CPUState *cpu;

    error_set_progname("libqemu");
    qemu_init_exec_dir("/tmp");

    module_call_init(MODULE_INIT_QOM);
    if (cpu_model == NULL) {
#if defined(TARGET_I386)
#ifdef TARGET_X86_64
        cpu_model = "qemu64";
#else
        cpu_model = "qemu32";
#endif
#elif defined(TARGET_ARM)
        cpu_model = "any";
#elif defined(TARGET_UNICORE32)
        cpu_model = "any";
#elif defined(TARGET_M68K)
        cpu_model = "any";
#elif defined(TARGET_SPARC)
#ifdef TARGET_SPARC64
        cpu_model = "TI UltraSparc II";
#else
        cpu_model = "Fujitsu MB86904";
#endif
#elif defined(TARGET_MIPS)
#if defined(TARGET_ABI_MIPSN32) || defined(TARGET_ABI_MIPSN64)
        cpu_model = "5KEf";
#else
        cpu_model = "24Kf";
#endif
#elif defined TARGET_OPENRISC
        cpu_model = "or1200";
#elif defined(TARGET_PPC)
# ifdef TARGET_PPC64
        cpu_model = "POWER7";
# else
        cpu_model = "750";
# endif
#elif defined TARGET_SH4
        cpu_model = TYPE_SH7785_CPU;
#else
        cpu_model = "any";
#endif
    }
    tcg_exec_init(0);
    /* NOTE: we need to init the CPU at this stage to get
       qemu_host_page_size */
    cpu = cpu_init(cpu_model);
    if (!cpu) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(EXIT_FAILURE);
    }
//    env = (struct CPUArchState *) cpu->env_ptr;
    cpu_reset(cpu);

//    thread_cpu = cpu;
//
//    runstate_init();
//
//    if (qcrypto_init(&err) < 0) {
//        error_report("cannot initialize crypto: %s", error_get_pretty(err));
//        exit(1);
//    }
//    rtc_clock = QEMU_CLOCK_HOST;
//
//    QLIST_INIT (&vm_change_state_head);
//    os_setup_early_signal_handling();
//
//    module_call_init(MODULE_INIT_MACHINE);
//    machine_class = find_default_machine();
//    cpu_model = NULL;
//    snapshot = 0;
//    cyls = heads = secs = 0;
//    translation = BIOS_ATA_TRANSLATION_AUTO;
//
//    nb_nics = 0;
//
//    bdrv_init_with_whitelist();
//
//    autostart = 1;
//    set_memory_options(&ram_slots, &maxram_size, machine_class);
//
//    loc_set_none();
//
//    os_daemonize();
//
//    if (qemu_init_main_loop(&main_loop_err)) {
//        error_report_err(main_loop_err);
//        exit(1);
//    }
//
//    current_machine = MACHINE(object_new(object_class_get_name(
//                          OBJECT_CLASS(machine_class))));
//    if (machine_help_func(qemu_get_machine_opts(), current_machine)) {
//        exit(0);
//    }
//    object_property_add_child(object_get_root(), "machine",
//                              OBJECT(current_machine), &error_abort);
//    cpu_exec_init_all();
//
//    if (machine_class->hw_version) {
//        qemu_set_hw_version(machine_class->hw_version);
//    }
//
//    /* Init CPU def lists, based on config
//     * - Must be called after all the qemu_read_config_file() calls
//     * - Must be called before list_cpus()
//     * - Must be called before machine->init()
//     */
//    cpudef_init();
//
//    if (cpu_model && is_help_option(cpu_model)) {
//        list_cpus(stdout, &fprintf, cpu_model);
//        exit(0);
//    }
//
//    /* Open the logfile at this point and set the log mask if necessary.
//     */
//    if (log_file) {
//        qemu_set_log_filename(log_file);
//    }
//
//    if (log_mask) {
//        int mask;
//        mask = qemu_str_to_log_mask(log_mask);
//        if (!mask) {
//            qemu_print_log_usage(stdout);
//            exit(1);
//        }
//        qemu_set_log(mask);
//    }
//
//    if (!is_daemonized()) {
//        if (!trace_init_backends(trace_events, trace_file)) {
//            exit(1);
//        }
//    }
//
//    /* If no data_dir is specified then try to find it relative to the
//       executable path.  */
//    if (data_dir_idx < ARRAY_SIZE(data_dir)) {
//        data_dir[data_dir_idx] = os_find_datadir();
//        if (data_dir[data_dir_idx] != NULL) {
//            data_dir_idx++;
//        }
//    }
//    /* If all else fails use the install path specified when building. */
//    if (data_dir_idx < ARRAY_SIZE(data_dir)) {
//        data_dir[data_dir_idx++] = CONFIG_QEMU_DATADIR;
//    }
//
//    smp_parse(qemu_opts_find(qemu_find_opts("smp-opts"), NULL));
//
//    machine_class->max_cpus = machine_class->max_cpus ?: 1; /* Default to UP */
//    if (max_cpus > machine_class->max_cpus) {
//        error_report("Number of SMP CPUs requested (%d) exceeds max CPUs "
//                     "supported by machine '%s' (%d)", max_cpus,
//                     machine_class->name, machine_class->max_cpus);
//        exit(1);
//    }
//
//    /*
//     * Get the default machine options from the machine if it is not already
//     * specified either by the configuration file or by the command line.
//     */
//    if (machine_class->default_machine_opts) {
//        qemu_opts_set_defaults(qemu_find_opts("machine"),
//                               machine_class->default_machine_opts, 0);
//    }
//
//    qemu_opts_foreach(qemu_find_opts("device"),
//                      default_driver_check, NULL, NULL);
//    qemu_opts_foreach(qemu_find_opts("global"),
//                      default_driver_check, NULL, NULL);
//
//    if (!vga_model && !default_vga) {
//        vga_interface_type = VGA_DEVICE;
//    }
//    if (!has_defaults || machine_class->no_serial) {
//        default_serial = 0;
//    }
//    if (!has_defaults || machine_class->no_parallel) {
//        default_parallel = 0;
//    }
//    if (!has_defaults || !machine_class->use_virtcon) {
//        default_virtcon = 0;
//    }
//    if (!has_defaults || !machine_class->use_sclp) {
//        default_sclp = 0;
//    }
//    if (!has_defaults || machine_class->no_floppy) {
//        default_floppy = 0;
//    }
//    if (!has_defaults || machine_class->no_cdrom) {
//        default_cdrom = 0;
//    }
//    if (!has_defaults || machine_class->no_sdcard) {
//        default_sdcard = 0;
//    }
//    if (!has_defaults) {
//        default_monitor = 0;
//        default_net = 0;
//        default_vga = 0;
//    }
//
//    if (is_daemonized()) {
//        /* According to documentation and historically, -nographic redirects
//         * serial port, parallel port and monitor to stdio, which does not work
//         * with -daemonize.  We can redirect these to null instead, but since
//         * -nographic is legacy, let's just error out.
//         * We disallow -nographic only if all other ports are not redirected
//         * explicitly, to not break existing legacy setups which uses
//         * -nographic _and_ redirects all ports explicitly - this is valid
//         * usage, -nographic is just a no-op in this case.
//         */
//        if (display_type == DT_NOGRAPHIC
//            && (default_parallel || default_serial
//                || default_monitor || default_virtcon)) {
//            error_report("-nographic cannot be used with -daemonize");
//            exit(1);
//        }
//#ifdef CONFIG_CURSES
//        if (display_type == DT_CURSES) {
//            error_report("curses display cannot be used with -daemonize");
//            exit(1);
//        }
//#endif
//    }
//
//    if (display_type == DT_NOGRAPHIC) {
//        if (default_parallel)
//            add_device_config(DEV_PARALLEL, "null");
//        if (default_serial && default_monitor) {
//            add_device_config(DEV_SERIAL, "mon:stdio");
//        } else if (default_virtcon && default_monitor) {
//            add_device_config(DEV_VIRTCON, "mon:stdio");
//        } else if (default_sclp && default_monitor) {
//            add_device_config(DEV_SCLP, "mon:stdio");
//        } else {
//            if (default_serial)
//                add_device_config(DEV_SERIAL, "stdio");
//            if (default_virtcon)
//                add_device_config(DEV_VIRTCON, "stdio");
//            if (default_sclp) {
//                add_device_config(DEV_SCLP, "stdio");
//            }
//            if (default_monitor)
//                monitor_parse("stdio", "readline", false);
//        }
//    } else {
//        if (default_serial)
//            add_device_config(DEV_SERIAL, "vc:80Cx24C");
//        if (default_parallel)
//            add_device_config(DEV_PARALLEL, "vc:80Cx24C");
//        if (default_monitor)
//            monitor_parse("vc:80Cx24C", "readline", false);
//        if (default_virtcon)
//            add_device_config(DEV_VIRTCON, "vc:80Cx24C");
//        if (default_sclp) {
//            add_device_config(DEV_SCLP, "vc:80Cx24C");
//        }
//    }
//
//#if defined(CONFIG_VNC)
//    if (!QTAILQ_EMPTY(&(qemu_find_opts("vnc")->head))) {
//        display_remote++;
//    }
//#endif
//    if (display_type == DT_DEFAULT && !display_remote) {
//#if defined(CONFIG_GTK)
//        display_type = DT_GTK;
//#elif defined(CONFIG_SDL) || defined(CONFIG_COCOA)
//        display_type = DT_SDL;
//#elif defined(CONFIG_VNC)
//        vnc_parse("localhost:0,to=99,id=default", &error_abort);
//        show_vnc_port = 1;
//#else
//        display_type = DT_NONE;
//#endif
//    }
//
//    if ((no_frame || alt_grab || ctrl_grab) && display_type != DT_SDL) {
//        error_report("-no-frame, -alt-grab and -ctrl-grab are only valid "
//                     "for SDL, ignoring option");
//    }
//    if (no_quit && (display_type != DT_GTK && display_type != DT_SDL)) {
//        error_report("-no-quit is only valid for GTK and SDL, "
//                     "ignoring option");
//    }
//
//#if defined(CONFIG_GTK)
//    if (display_type == DT_GTK) {
//        early_gtk_display_init(request_opengl);
//    }
//#endif
//#if defined(CONFIG_SDL)
//    if (display_type == DT_SDL) {
//        sdl_display_early_init(request_opengl);
//    }
//#endif
//    if (request_opengl == 1 && display_opengl == 0) {
//#if defined(CONFIG_OPENGL)
//        error_report("OpenGL is not supported by the display");
//#else
//        error_report("OpenGL support is disabled");
//#endif
//        exit(1);
//    }
//
//    page_size_init();
//    socket_init();
//
//    if (qemu_opts_foreach(qemu_find_opts("object"),
//                          object_create,
//                          object_create_initial, NULL)) {
//        exit(1);
//    }
//
//    if (qemu_opts_foreach(qemu_find_opts("chardev"),
//                          chardev_init_func, NULL, NULL)) {
//        exit(1);
//    }
//
//#ifdef CONFIG_VIRTFS
//    if (qemu_opts_foreach(qemu_find_opts("fsdev"),
//                          fsdev_init_func, NULL, NULL)) {
//        exit(1);
//    }
//#endif
//
//    if (pid_file && qemu_create_pidfile(pid_file) != 0) {
//        error_report("could not acquire pid file: %s", strerror(errno));
//        exit(1);
//    }
//
//    if (qemu_opts_foreach(qemu_find_opts("device"),
//                          device_help_func, NULL, NULL)) {
//        exit(0);
//    }
//
//    machine_opts = qemu_get_machine_opts();
//    if (qemu_opt_foreach(machine_opts, machine_set_property, current_machine,
//                         NULL)) {
//        object_unref(OBJECT(current_machine));
//        exit(1);
//    }
//
//    configure_accelerator(current_machine);
//
//    if (qtest_chrdev) {
//        Error *local_err = NULL;
//        qtest_init(qtest_chrdev, qtest_log, &local_err);
//        if (local_err) {
//            error_report_err(local_err);
//            exit(1);
//        }
//    }
//
//    machine_opts = qemu_get_machine_opts();
//    kernel_filename = qemu_opt_get(machine_opts, "kernel");
//    initrd_filename = qemu_opt_get(machine_opts, "initrd");
//    kernel_cmdline = qemu_opt_get(machine_opts, "append");
//    bios_name = qemu_opt_get(machine_opts, "firmware");
//
//    opts = qemu_opts_find(qemu_find_opts("boot-opts"), NULL);
//    if (opts) {
//        Error *local_err = NULL;
//
//        boot_order = qemu_opt_get(opts, "order");
//        if (boot_order) {
//            validate_bootdevices(boot_order, &local_err);
//            if (local_err) {
//                error_report_err(local_err);
//                exit(1);
//            }
//        }
//
//        boot_once = qemu_opt_get(opts, "once");
//        if (boot_once) {
//            validate_bootdevices(boot_once, &local_err);
//            if (local_err) {
//                error_report_err(local_err);
//                exit(1);
//            }
//        }
//
//        boot_menu = qemu_opt_get_bool(opts, "menu", boot_menu);
//        boot_strict = qemu_opt_get_bool(opts, "strict", false);
//    }
//
//    if (!boot_order) {
//        boot_order = machine_class->default_boot_order;
//    }
//
//    if (!kernel_cmdline) {
//        kernel_cmdline = "";
//        current_machine->kernel_cmdline = (char *)kernel_cmdline;
//    }
//
//    linux_boot = (kernel_filename != NULL);
//
//    if (!linux_boot && *kernel_cmdline != '\0') {
//        error_report("-append only allowed with -kernel option");
//        exit(1);
//    }
//
//    if (!linux_boot && initrd_filename != NULL) {
//        error_report("-initrd only allowed with -kernel option");
//        exit(1);
//    }
//
//    if (!linux_boot && qemu_opt_get(machine_opts, "dtb")) {
//        error_report("-dtb only allowed with -kernel option");
//        exit(1);
//    }
//
//    if (semihosting_enabled() && !semihosting_get_argc() && kernel_filename) {
//        /* fall back to the -kernel/-append */
//        semihosting_arg_fallback(kernel_filename, kernel_cmdline);
//    }
//
//    os_set_line_buffering();
//
//#ifdef CONFIG_SPICE
//    /* spice needs the timers to be initialized by this point */
//    qemu_spice_init();
//#endif
//
//    cpu_ticks_init();
//    if (icount_opts) {
//        if (kvm_enabled() || xen_enabled()) {
//            error_report("-icount is not allowed with kvm or xen");
//            exit(1);
//        }
//        configure_icount(icount_opts, &error_abort);
//        qemu_opts_del(icount_opts);
//    }
//
//    /* clean up network at qemu process termination */
//    atexit(&net_cleanup);
//
//    if (net_init_clients() < 0) {
//        exit(1);
//    }
//
//    if (qemu_opts_foreach(qemu_find_opts("object"),
//                          object_create,
//                          object_create_delayed, NULL)) {
//        exit(1);
//    }
//
//#ifdef CONFIG_TPM
//    if (tpm_init() < 0) {
//        exit(1);
//    }
//#endif
//
//    /* init the bluetooth world */
//    if (foreach_device_config(DEV_BT, bt_parse))
//        exit(1);
//
//    if (!xen_enabled()) {
//        /* On 32-bit hosts, QEMU is limited by virtual address space */
//        if (ram_size > (2047 << 20) && HOST_LONG_BITS == 32) {
//            error_report("at most 2047 MB RAM can be simulated");
//            exit(1);
//        }
//    }
//
//    blk_mig_init();
//    ram_mig_init();
//
//    /* If the currently selected machine wishes to override the units-per-bus
//     * property of its default HBA interface type, do so now. */
//    if (machine_class->units_per_default_bus) {
//        override_max_devs(machine_class->block_default_type,
//                          machine_class->units_per_default_bus);
//    }
//
//    /* open the virtual block devices */
//    if (snapshot || replay_mode != REPLAY_MODE_NONE) {
//        qemu_opts_foreach(qemu_find_opts("drive"), drive_enable_snapshot,
//                          NULL, NULL);
//    }
//    if (qemu_opts_foreach(qemu_find_opts("drive"), drive_init_func,
//                          &machine_class->block_default_type, NULL)) {
//        exit(1);
//    }
//
//    default_drive(default_cdrom, snapshot, machine_class->block_default_type, 2,
//                  CDROM_OPTS);
//    default_drive(default_floppy, snapshot, IF_FLOPPY, 0, FD_OPTS);
//    default_drive(default_sdcard, snapshot, IF_SD, 0, SD_OPTS);
//
//    parse_numa_opts(machine_class);
//
//    if (qemu_opts_foreach(qemu_find_opts("mon"),
//                          mon_init_func, NULL, NULL)) {
//        exit(1);
//    }
//
//    if (foreach_device_config(DEV_SERIAL, serial_parse) < 0)
//        exit(1);
//    if (foreach_device_config(DEV_PARALLEL, parallel_parse) < 0)
//        exit(1);
//    if (foreach_device_config(DEV_VIRTCON, virtcon_parse) < 0)
//        exit(1);
//    if (foreach_device_config(DEV_SCLP, sclp_parse) < 0) {
//        exit(1);
//    }
//    if (foreach_device_config(DEV_DEBUGCON, debugcon_parse) < 0)
//        exit(1);
//
//    /* If no default VGA is requested, the default is "none".  */
//    if (default_vga) {
//        if (machine_class->default_display) {
//            vga_model = machine_class->default_display;
//        } else if (cirrus_vga_available()) {
//            vga_model = "cirrus";
//        } else if (vga_available()) {
//            vga_model = "std";
//        }
//    }
//    if (vga_model) {
//        select_vgahw(vga_model);
//    }
//
//    if (watchdog) {
//        i = select_watchdog(watchdog);
//        if (i > 0)
//            exit (i == 1 ? 1 : 0);
//    }
//
//    if (machine_class->compat_props) {
//        qdev_prop_register_global_list(machine_class->compat_props);
//    }
//    qemu_add_globals();
//
//    /* This checkpoint is required by replay to separate prior clock
//       reading from the other reads, because timer polling functions query
//       clock values from the log. */
//    replay_checkpoint(CHECKPOINT_INIT);
//    qdev_machine_init();
//
//    current_machine->ram_size = ram_size;
//    current_machine->maxram_size = maxram_size;
//    current_machine->ram_slots = ram_slots;
//    current_machine->boot_order = boot_order;
//    current_machine->cpu_model = cpu_model;
//
//    machine_class->init(current_machine);
//
//    realtime_init();
//
//    audio_init();
//
//    cpu_synchronize_all_post_init();
//
//    numa_post_machine_init();
//
//    if (qemu_opts_foreach(qemu_find_opts("fw_cfg"),
//                          parse_fw_cfg, fw_cfg_find(), NULL) != 0) {
//        exit(1);
//    }
//
//    /* init USB devices */
//    if (usb_enabled()) {
//        if (foreach_device_config(DEV_USB, usb_parse) < 0)
//            exit(1);
//    }
//
//    /* Check if IGD GFX passthrough. */
//    igd_gfx_passthru();
//
//    /* init generic devices */
//    if (qemu_opts_foreach(qemu_find_opts("device"),
//                          device_init_func, NULL, NULL)) {
//        exit(1);
//    }
//
//    /* Did we create any drives that we failed to create a device for? */
//    drive_check_orphaned();
//
//    net_check_clients();
//
//    if (boot_once) {
//        Error *local_err = NULL;
//        qemu_boot_set(boot_once, &local_err);
//        if (local_err) {
//            error_report("%s", error_get_pretty(local_err));
//            exit(1);
//        }
//        qemu_register_reset(restore_boot_order, g_strdup(boot_order));
//    }
//
//    ds = init_displaystate();
//
//    /* init local displays */
//    switch (display_type) {
//    case DT_NOGRAPHIC:
//        (void)ds;	/* avoid warning if no display is configured */
//        break;
//#if defined(CONFIG_CURSES)
//    case DT_CURSES:
//        curses_display_init(ds, full_screen);
//        break;
//#endif
//#if defined(CONFIG_SDL)
//    case DT_SDL:
//        sdl_display_init(ds, full_screen, no_frame);
//        break;
//#elif defined(CONFIG_COCOA)
//    case DT_SDL:
//        cocoa_display_init(ds, full_screen);
//        break;
//#endif
//#if defined(CONFIG_GTK)
//    case DT_GTK:
//        gtk_display_init(ds, full_screen, grab_on_hover);
//        break;
//#endif
//    default:
//        break;
//    }
//
//    /* must be after terminal init, SDL library changes signal handlers */
//    os_setup_signal_handling();
//
//#ifdef CONFIG_VNC
//    /* init remote displays */
//    qemu_opts_foreach(qemu_find_opts("vnc"),
//                      vnc_init_func, NULL, NULL);
//    if (show_vnc_port) {
//        char *ret = vnc_display_local_addr("default");
//        printf("VNC server running on '%s'\n", ret);
//        g_free(ret);
//    }
//#endif
//#ifdef CONFIG_SPICE
//    if (using_spice) {
//        qemu_spice_display_init();
//    }
//#endif
//
//    if (foreach_device_config(DEV_GDB, gdbserver_start) < 0) {
//        exit(1);
//    }
//
//    qdev_machine_creation_done();
//
//    /* TODO: once all bus devices are qdevified, this should be done
//     * when bus is created by qdev.c */
//    qemu_register_reset(qbus_reset_all_fn, sysbus_get_default());
//    qemu_run_machine_init_done_notifiers();
//
//    if (rom_check_and_register_reset() != 0) {
//        error_report("rom check and register reset failed");
//        exit(1);
//    }
//
//    replay_start();
//
//    /* This checkpoint is required by replay to separate prior clock
//       reading from the other reads, because timer polling functions query
//       clock values from the log. */
//    replay_checkpoint(CHECKPOINT_RESET);
//    qemu_system_reset(VMRESET_SILENT);
//    register_global_state();
//    if (loadvm) {
//        if (load_vmstate(loadvm) < 0) {
//            autostart = 0;
//        }
//    }
//
//    qdev_prop_check_globals();
//    if (vmstate_dump_file) {
//        /* dump and exit */
//        dump_vmstate_json_to_file(vmstate_dump_file);
//        return 0;
//    }
//
//    if (incoming) {
//        Error *local_err = NULL;
//        qemu_start_incoming_migration(incoming, &local_err);
//        if (local_err) {
//            error_report("-incoming %s: %s", incoming,
//                         error_get_pretty(local_err));
//            error_free(local_err);
//            exit(1);
//        }
//    } else if (autostart) {
//        vm_start();
//    }
//
//    os_setup_post();
//
//    if (is_daemonized()) {
//        if (!trace_init_backends(trace_events, trace_file)) {
//            exit(1);
//        }
//    }
//
//    main_loop();
//    replay_disable_events();
//
//    bdrv_close_all();
//    pause_all_vcpus();
//    res_free();
//#ifdef CONFIG_TPM
//    tpm_cleanup();
//#endif
    return 0;
}
