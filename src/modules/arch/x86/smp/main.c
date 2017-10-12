#include <fudge.h>
#include <kernel.h>
#include <kernel/x86/cpu.h>
#include <kernel/x86/gdt.h>
#include <kernel/x86/idt.h>
#include <kernel/x86/tss.h>
#include <kernel/x86/mmu.h>
#include <kernel/x86/arch.h>
#include <modules/system/system.h>
#include <modules/arch/x86/acpi/acpi.h>
#include <modules/arch/x86/cpuid/cpuid.h>
#include <modules/arch/x86/apic/apic.h>
#include <modules/arch/x86/ioapic/ioapic.h>
#include <modules/arch/x86/pit/pit.h>
#include "smp.h"

#define INIT16ADDRESS                   0x00008000
#define INIT32ADDRESS                   0x00008200

static struct arch_tss tss[256];
static struct core cores[256];
static struct spinlock spinlock;
static struct system_node root;
static struct system_node cpus;
static struct list corelist;

static void detect(struct acpi_madt *madt)
{

    unsigned int madttable = (unsigned int)madt + sizeof (struct acpi_madt);
    unsigned int madtend = (unsigned int)madt + madt->base.length;

    while (madttable < madtend)
    {

        struct acpi_madt_entry *entry = (struct acpi_madt_entry *)madttable;

        if (entry->type == 0)
        {

            struct acpi_madt_apic *apic = (struct acpi_madt_apic *)entry;

            if (apic->id)
            {

                apic_sendint(apic->id, APIC_ICR_INIT | APIC_ICR_PHYSICAL | APIC_ICR_ASSERT | APIC_ICR_SINGLE | 0x00); 
                pit_wait(10);
                apic_sendint(apic->id, APIC_ICR_SIPI | APIC_ICR_PHYSICAL | APIC_ICR_ASSERT | APIC_ICR_SINGLE | 0x08); 

            }

        }

        if (entry->type == 1)
        {

/*
            struct acpi_madt_ioapic *ioapic = (struct acpi_madt_ioapic *)entry;
            unsigned int id;
            unsigned int version;
            unsigned int count;

            arch_setmap(8, ioapic->address, ioapic->address, 0x1000);

            id = ioapic_ind(ioapic->address, 0);
            version = ioapic_ind(ioapic->address, 1);
            count = ((version >> 16) & 0xFF) + 1;

            DEBUG(DEBUG_INFO, "SMP IOAPIC");
            debug_write(DEBUG_INFO, "  ", "ioapic id", ioapic->id);
            debug_write(DEBUG_INFO, "  ", "ioapic address", ioapic->address);
            debug_write(DEBUG_INFO, "  ", "ioapic id", id);
            debug_write(DEBUG_INFO, "  ", "ioapic version", version);
            debug_write(DEBUG_INFO, "  ", "ioapic count", count);
*/

        }

        madttable += entry->length;

    }

}

static void copytrampoline16()
{

    unsigned int begin = (unsigned int)smp_begin16;
    unsigned int end = (unsigned int)smp_end16;

    memory_copy((void *)INIT16ADDRESS, (void *)begin, end - begin);

}

static void copytrampoline32()
{

    unsigned int begin = (unsigned int)smp_begin32;
    unsigned int end = (unsigned int)smp_end32;

    memory_copy((void *)INIT32ADDRESS, (void *)begin, end - begin);

}

/* TODO: Remove changing of CR3 register. This is a workaround because existing tasks cant access apic mapped register. */
static struct core *getcore(void)
{

    unsigned int directory = cpu_getcr3();
    unsigned int id;

    cpu_setcr3(ARCH_MMUKERNELADDRESS);

    id = apic_getid();

    cpu_setcr3(directory);

    return &cores[id];

}

static void assign(struct task *task)
{

    struct core *core;

    spinlock_acquire(&spinlock);

    core = corelist.head->data;

/*
    Round robin scheuling.

    list_move(&corelist, &core->item);
    apic_sendint(core->id, APIC_ICR_ASSERT | 0xFE);
*/

    list_move(&core->tasks, &task->state.item);
    spinlock_release(&spinlock);

}

static void registercore(struct core *core)
{

    spinlock_acquire(&spinlock);
    list_add(&corelist, &core->item);
    spinlock_release(&spinlock);

}

void smp_setup(unsigned int stack)
{

    unsigned int id = apic_getid();

    core_init(&cores[id], id, stack);
    arch_configuretss(&tss[id], ARCH_TSS + id, cores[id].sp);
    mmu_setdirectory((struct mmu_directory *)ARCH_MMUKERNELADDRESS);
    mmu_enable();
    registercore(&cores[id]);
    arch_leave(&cores[id]);

}

static unsigned int cpus_read(struct system_node *self, struct system_node *current, struct service_state *state, void *buffer, unsigned int count, unsigned int offset)
{

    char num[FUDGE_NSIZE];

    return memory_read(buffer, count, num, ascii_wvalue(num, FUDGE_NSIZE, corelist.count, 10, 0), offset);

}

void module_init(void)
{

    unsigned int id = apic_getid();
    struct acpi_madt *madt = (struct acpi_madt *)acpi_findheader("APIC");
    struct core *c = arch_getcore();

    core_init(&cores[id], id, c->sp);
    arch_configuretss(&tss[id], ARCH_TSS + id, cores[id].sp);
    registercore(&cores[id]);

    cores[id].task = c->task;

    while (c->tasks.count)
        list_move(&cores[id].tasks, c->tasks.tail);

    arch_setcore(getcore);
    arch_setassign(assign);
    system_initnode(&root, SYSTEM_NODETYPE_GROUP, "smp");
    system_initnode(&cpus, SYSTEM_NODETYPE_NORMAL, "cpus");

    cpus.read = cpus_read;

    system_addchild(&root, &cpus);

    if (!madt)
        return;

    smp_prep(ARCH_KERNELSTACKADDRESS + 2 * ARCH_KERNELSTACKSIZE);
    copytrampoline16();
    copytrampoline32();
    detect(madt);

}

void module_register(void)
{

    system_registernode(&root);

}

void module_unregister(void)
{

    system_unregisternode(&root);

}

