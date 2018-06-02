/*
 * ARMV6M CPU object
 *
 * Copyright (c) 2018 Red Hat, Inc.
 *
 * Based on include/hw/arm/armv7m.h (written by Peter Maydell
 * <peter.maydell@linaro.org>),
 * Copyright (c) 2017 Linaro Ltd.
 *
 * This code is licensed under the GPL version 2 or later.
 */

#ifndef HW_ARM_ARMV6M_H
#define HW_ARM_ARMV6M_H

#include "hw/sysbus.h"
#include "hw/intc/armv7m_nvic.h"

#define TYPE_ARMV6M "armv6m"
#define ARMV6M(obj) OBJECT_CHECK(ARMv6MState, (obj), TYPE_ARMV6M)

/* ARMV6M container object.
 * + Unnamed GPIO input lines: external IRQ lines for the NVIC
 * + Named GPIO output SYSRESETREQ: signalled for guest AIRCR.SYSRESETREQ
 * + Property "cpu-type": CPU type to instantiate
 * + Property "num-irq": number of external IRQ lines
 * + Property "memory": MemoryRegion defining the physical address space
 *   that CPU accesses see. (The NVIC and other CPU-internal devices will be
 *   automatically layered on top of this view.)
 */
typedef struct {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    NVICState nvic;
    ARMCPU *cpu;

    /* MemoryRegion we pass to the CPU, with our devices layered on
     * top of the ones the board provides in board_memory.
     */
    MemoryRegion container;

    /* Properties */
    char *cpu_type;
    /* MemoryRegion the board provides to us (with its devices, RAM, etc) */
    MemoryRegion *board_memory;
} ARMv6MState;

#endif
