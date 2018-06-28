/*
 * Nordic Semiconductor nRF51 SoC
 * http://infocenter.nordicsemi.com/pdf/nRF51_RM_v3.0.1.pdf
 *
 * Copyright 2018 Joel Stanley <joel@jms.id.au>
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "hw/arm/arm.h"
#include "hw/sysbus.h"
#include "hw/or-irq.h"
#include "hw/boards.h"
#include "hw/devices.h"
#include "hw/misc/unimp.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "qemu/log.h"
#include "cpu.h"
#include "crypto/random.h"

#include "hw/arm/nrf51_soc.h"
#include "hw/char/nrf51_uart.h"

#define IOMEM_BASE      0x40000000
#define IOMEM_SIZE      0x20000000

#define FICR_BASE       0x10000000
#define FICR_SIZE       0x000000fc

/* TODO: Flash size should be defined by the board. Microbit uses 256KB */
#define FLASH_BASE      0x00000000
#define FLASH_SIZE      (256 * 1024)

/* TODO: Flash size should be defined by the board. Microbit uses 16KB */
#define SRAM_BASE       0x20000000
#define SRAM_SIZE       (16 * 1024)

/*
static uint64_t clock_read(void *opaque, hwaddr addr, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " [%u]\n", __func__, addr, size);
    return 1;
}

static void clock_write(void *opaque, hwaddr addr, uint64_t data, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " <- 0x%" PRIx64 " [%u]\n", __func__, addr, data, size);
}


static const MemoryRegionOps clock_ops = {
    .read = clock_read,
    .write = clock_write
};

static uint64_t nvmc_read(void *opaque, hwaddr addr, unsigned int size)
{
    qemu_log_mask(LOG_TRACE, "%s: 0x%" HWADDR_PRIx " [%u]\n", __func__, addr, size);
    return 1;
}

static void nvmc_write(void *opaque, hwaddr addr, uint64_t data, unsigned int size)
{
    qemu_log_mask(LOG_TRACE, "%s: 0x%" HWADDR_PRIx " <- 0x%" PRIx64 " [%u]\n", __func__, addr, data, size);
}


static const MemoryRegionOps nvmc_ops = {
    .read = nvmc_read,
    .write = nvmc_write
};

static uint64_t rng_read(void *opaque, hwaddr addr, unsigned int size)
{
    uint64_t r = 0;

    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " [%u]\n", __func__, addr, size);

    switch (addr) {
    case 0x508:
        qcrypto_random_bytes((uint8_t *)&r, 1, NULL);
        break;
    default:
        r = 1;
        break;
    }
    return r;
}

static void rng_write(void *opaque, hwaddr addr, uint64_t data, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " <- 0x%" PRIx64 " [%u]\n", __func__, addr, data, size);
}


static const MemoryRegionOps rng_ops = {
    .read = rng_read,
    .write = rng_write
};
*/
static void nrf51_soc_realize(DeviceState *dev_soc, Error **errp)
{
    NRF51State *s = NRF51_SOC(dev_soc);
    Error *err = NULL;
//    DeviceState *dev;

    if (!s->board_memory) {
        error_setg(errp, "memory property was not set");
        return;
    }

    object_property_set_link(OBJECT(&s->cpu), OBJECT(&s->container), "memory",
            &err);
    object_property_set_bool(OBJECT(&s->cpu), true, "realized", &err);

    memory_region_add_subregion_overlap(&s->container, 0, s->board_memory, -1);
/*
    create_unimplemented_device("nrf51_soc.uart", UART_BASE, UART_SIZE);
    create_unimplemented_device("nrf51_soc.clock", IOMEM_BASE, 0x1000);
*/

    memory_region_init_ram(&s->flash, OBJECT(s), "nrf51.flash", FLASH_SIZE, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    memory_region_set_readonly(&s->flash, true);
    memory_region_add_subregion(&s->container, FLASH_BASE, &s->flash);

    memory_region_init_ram(&s->sram, NULL, "nrf51.sram", SRAM_SIZE, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    memory_region_add_subregion(&s->container, SRAM_BASE, &s->sram);
/*
    dev = DEVICE(&s->uart);
    qdev_prop_set_chr(dev, "chardev", serial_hd(0));
    object_property_set_bool(OBJECT(&s->uart), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
*/
    create_unimplemented_device("nrf51_soc.io", IOMEM_BASE, IOMEM_SIZE);
    create_unimplemented_device("nrf51_soc.ficr", FICR_BASE, FICR_SIZE);
    create_unimplemented_device("nrf51_soc.private", 0xF0000000, 0x10000000);
    create_unimplemented_device("nrf51_soc.uart", UART_BASE, UART_SIZE);
}

static void nrf51_soc_init(Object *obj)
{
    NRF51State *s = NRF51_SOC(obj);
//    SysBusDevice *busdev;
//    DeviceState *dev;
    Object *orgate;
    //DeviceState *dev;

    memory_region_init(&s->container, obj, "nrf51-container", UINT64_MAX);

    /* TODO: implement a cortex m0 and update this */
    object_initialize(&s->cpu, sizeof(s->cpu), TYPE_ARMV7M);
    object_property_add_child(OBJECT(s), "armv7m", OBJECT(&s->cpu), &error_abort);
    qdev_set_parent_bus(DEVICE(&s->cpu), sysbus_get_default());
    qdev_prop_set_string(DEVICE(&s->cpu), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));
    qdev_prop_set_uint32(DEVICE(&s->cpu), "num-irq", 96);

    orgate = object_new(TYPE_OR_IRQ);
    object_property_set_bool(orgate, true, "realized", &error_fatal);
  //  dev = DEVICE(orgate);
//    qdev_connect_gpio_out(dev, 0, qdev_get_gpio_in(DEVICE(&s->cpu), 2));

//    nrf51_uart_create(UART_BASE,
//                      qdev_get_gpio_in(DEVICE(&s->cpu), 2),
//                      serial_hd(0));
/*
    dev = DEVICE(&s->uart);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, UART_BASE);
    sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(DEVICE(&s->cpu), 2));

    object_initialize(&s->uart, sizeof(s->uart), TYPE_NRF51_UART);
    qdev_set_parent_bus(DEVICE(&s->uart), sysbus_get_default());

    memory_region_init_io(&s->clock, NULL, &clock_ops, NULL, "nrf51_soc.clock", 0x1000);
    memory_region_add_subregion_overlap(get_system_memory(), IOMEM_BASE, &s->clock, -1);

    memory_region_init_io(&s->nvmc, NULL, &nvmc_ops, NULL, "nrf51_soc.nvmc", 0x1000);
    memory_region_add_subregion_overlap(get_system_memory(), 0x4001E000, &s->nvmc, -1);

    memory_region_init_io(&s->rng, NULL, &rng_ops, NULL, "nrf51_soc.rng", 0x1000);
    memory_region_add_subregion_overlap(get_system_memory(), 0x4000D000, &s->rng, -1);
*/
}

static Property nrf51_soc_properties[] = {
    DEFINE_PROP_LINK("memory", NRF51State, board_memory, TYPE_MEMORY_REGION,
                     MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void nrf51_soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = nrf51_soc_realize;
    dc->props = nrf51_soc_properties;
}

static const TypeInfo nrf51_soc_info = {
    .name          = TYPE_NRF51_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NRF51State),
    .instance_init = nrf51_soc_init,
    .class_init    = nrf51_soc_class_init,
};

static void nrf51_soc_types(void)
{
    type_register_static(&nrf51_soc_info);
}
type_init(nrf51_soc_types)
