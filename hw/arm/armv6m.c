/*
 * ARMV6M System emulation.
 *
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Based on hw/arm/armv7m.c (written by Paul Brook),
 * Copyright (c) 2006-2007 CodeSourcery.
 *
 * This code is licensed under the GPL.
 */

#include "qemu/osdep.h"
#include "hw/arm/armv6m.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "hw/loader.h"
#include "elf.h"
#include "sysemu/qtest.h"
#include "qemu/error-report.h"
#include "exec/address-spaces.h"

static void armv6m_instance_init(Object *obj)
{
    ARMv6MState *s = ARMV6M(obj);

    /* Can't init the cpu here, we don't yet know which model to use */

    memory_region_init(&s->container, obj, "armv6m-container", UINT64_MAX);

    object_initialize(&s->nvic, sizeof(s->nvic), TYPE_NVIC);
    qdev_set_parent_bus(DEVICE(&s->nvic), sysbus_get_default());
    object_property_add_alias(obj, "num-irq",
                              OBJECT(&s->nvic), "num-irq", &error_abort);
}

static void armv6m_realize(DeviceState *dev, Error **errp)
{
    ARMv6MState *s = ARMV6M(dev);
    SysBusDevice *sbd;
    Error *err = NULL;

    if (!s->board_memory) {
        error_setg(errp, "memory property was not set");
        return;
    }

    memory_region_add_subregion_overlap(&s->container, 0, s->board_memory, -1);

    s->cpu = ARM_CPU(object_new(s->cpu_type));

    object_property_set_link(OBJECT(s->cpu), OBJECT(&s->container), "memory",
                             &error_abort);
    object_property_set_bool(OBJECT(s->cpu), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* Note that we must realize the NVIC after the CPU */
    object_property_set_bool(OBJECT(&s->nvic), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* Alias the NVIC's input and output GPIOs as our own so the board
     * code can wire them up. (We do this in realize because the
     * NVIC doesn't create the input GPIO array until realize.)
     */
    qdev_pass_gpios(DEVICE(&s->nvic), dev, NULL);
    qdev_pass_gpios(DEVICE(&s->nvic), dev, "SYSRESETREQ");

    /* Wire the NVIC up to the CPU */
    sbd = SYS_BUS_DEVICE(&s->nvic);
    sysbus_connect_irq(sbd, 0,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_IRQ));
    s->cpu->env.nvic = &s->nvic;

    memory_region_add_subregion(&s->container, 0xe000e000,
                                sysbus_mmio_get_region(sbd, 0));
}

static Property armv6m_properties[] = {
    DEFINE_PROP_STRING("cpu-type", ARMv6MState, cpu_type),
    DEFINE_PROP_LINK("memory", ARMv6MState, board_memory, TYPE_MEMORY_REGION,
                     MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void armv6m_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = armv6m_realize;
    dc->props = armv6m_properties;
}

static const TypeInfo armv6m_info = {
    .name = TYPE_ARMV6M,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ARMv6MState),
    .instance_init = armv6m_instance_init,
    .class_init = armv6m_class_init,
};

static void armv6m_reset(void *opaque)
{
    ARMCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

static void armv6m_load_kernel(ARMCPU *cpu, const char *kernel_filename, int mem_size)
{
    int image_size;
    uint64_t entry;
    uint64_t lowaddr;
    int big_endian;
    AddressSpace *as;
    CPUState *cs = CPU(cpu);

#ifdef TARGET_WORDS_BIGENDIAN
    big_endian = 1;
#else
    big_endian = 0;
#endif

    if (!kernel_filename && !qtest_enabled()) {
        error_report("Guest image must be specified (using -kernel)");
        exit(1);
    }

    as = cpu_get_address_space(cs, ARMASIdx_NS);

    if (kernel_filename) {
        image_size = load_elf_as(kernel_filename, NULL, NULL, &entry, &lowaddr,
                                 NULL, big_endian, EM_ARM, 1, 0, as);
        if (image_size < 0) {
            entry = 0;
            image_size = load_targphys_hex_as(kernel_filename, &entry, as);
        }
        if (image_size < 0) {
            image_size = load_image_targphys_as(kernel_filename, 0,
                                                mem_size, as);
            lowaddr = 0;
        }
        if (image_size < 0) {
            error_report("Could not load kernel '%s'", kernel_filename);
            exit(1);
        }
    }

    /* CPU objects (unlike devices) are not automatically reset on system
     * reset, so we must always register a handler to do so. Unlike
     * A-profile CPUs, we don't need to do anything special in the
     * handler to arrange that it starts correctly.
     * This is arguably the wrong place to do this, but it matches the
     * way A-profile does it. Note that this means that every M profile
     * board must call this function!
     */
    qemu_register_reset(armv6m_reset, cpu);
}

DeviceState *armv6m_init(MemoryRegion *system_memory, int mem_size,
                         int num_irq, const char *kernel_filename,
                         const char *cpu_type)
{
    DeviceState *armv6m;

    armv6m = qdev_create(NULL, TYPE_ARMV6M);
    qdev_prop_set_uint32(armv6m, "num-irq", num_irq);
    qdev_prop_set_string(armv6m, "cpu-type", cpu_type);
    object_property_set_link(OBJECT(armv6m), OBJECT(system_memory),
                             "memory", &error_abort);
    /* This will exit with an error if the user passed us a bad cpu_type */
    qdev_init_nofail(armv6m);

    armv6m_load_kernel(ARM_CPU(first_cpu), kernel_filename, mem_size);
    return armv6m;
}

static void armv6m_register_types(void)
{
    type_register_static(&armv6m_info);
}

type_init(armv6m_register_types)
