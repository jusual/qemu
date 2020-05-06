/*
 * pcie_host.c
 * utility functions for pci express host bridge.
 *
 * Copyright (c) 2009 Isaku Yamahata <yamahata at valinux co jp>
 *                    VA Linux Systems Japan K.K.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pcie_host.h"
#include "qemu/module.h"
#include "exec/address-spaces.h"
#include "trace.h"
/*
static void pcie_host_data_write(void *opaque, hwaddr mmcfg_addr,
                                  uint64_t val, unsigned len)
{
    trace_pcie_host_data_write();
}

static uint64_t pcie_host_data_read(void *opaque,
                                     hwaddr mmcfg_addr,
                                     unsigned len)
{
    trace_pcie_host_data_read();
    return ~0x0;
}

static const MemoryRegionOps pcie_host_ops = {
    .read = pcie_host_data_read,
    .write = pcie_host_data_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};
*/
static void pcie_host_init(Object *obj)
{
    PCIExpressHost *e = PCIE_HOST_BRIDGE(obj);
    void *ptr;
    Error *err;

    e->base_addr = PCIE_BASE_ADDR_UNMAPPED;
//    memory_region_init_allones(&e->mmio, OBJECT(e), "pcie-mmcfg-mmio", PCIE_MMCFG_SIZE_MAX);
//    memory_region_init_io(&e->mmio, OBJECT(e), &pcie_host_ops, e, "pcie-mmcfg-mmio", PCIE_MMCFG_SIZE_MAX);
    memory_region_init_rom(&e->mmio, OBJECT(e), "pcie-mmcfg-rom", PCIE_MMCFG_SIZE_MAX, &err);
    ptr = memory_region_get_ram_ptr(&e->mmio);
    memset(ptr, 0xff, e->mmio.size);
}

void pcie_host_mmcfg_unmap(PCIExpressHost *e)
{
    if (e->base_addr != PCIE_BASE_ADDR_UNMAPPED) {
        memory_region_del_subregion(get_system_memory(), &e->mmio);
        e->base_addr = PCIE_BASE_ADDR_UNMAPPED;
    }
}

void pcie_host_mmcfg_init(PCIExpressHost *e, uint32_t size)
{
    assert(!(size & (size - 1)));       /* power of 2 */
    assert(size >= PCIE_MMCFG_SIZE_MIN);
    assert(size <= PCIE_MMCFG_SIZE_MAX);
    e->size = size;
    memory_region_set_size(&e->mmio, e->size);
}

void pcie_host_mmcfg_map(PCIExpressHost *e, hwaddr addr,
                         uint32_t size)
{
    pcie_host_mmcfg_init(e, size);
    e->base_addr = addr;
    memory_region_add_subregion(get_system_memory(), e->base_addr,
                                        &e->mmio); //priority?
}

void pcie_host_mmcfg_update(PCIExpressHost *e,
                            int enable,
                            hwaddr addr,
                            uint32_t size)
{
    memory_region_transaction_begin();
    pcie_host_mmcfg_unmap(e);
    if (enable) {
        pcie_host_mmcfg_map(e, addr, size);
    }
    memory_region_transaction_commit();
}

static const TypeInfo pcie_host_type_info = {
    .name = TYPE_PCIE_HOST_BRIDGE,
    .parent = TYPE_PCI_HOST_BRIDGE,
    .abstract = true,
    .instance_size = sizeof(PCIExpressHost),
    .instance_init = pcie_host_init,
};

static void pcie_host_register_types(void)
{
    type_register_static(&pcie_host_type_info);
}

type_init(pcie_host_register_types)
