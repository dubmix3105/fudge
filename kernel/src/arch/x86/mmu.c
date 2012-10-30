#include <memory.h>
#include <error.h>
#include <runtime.h>
#include <arch/x86/arch.h>
#include <arch/x86/cpu.h>
#include <arch/x86/idt.h>
#include <arch/x86/isr.h>
#include <arch/x86/mmu.h>

static struct mmu_table kernelTables[8];
static struct mmu_directory runtimeDirectories[32];
static struct mmu_table runtimeTables[32];

static void enable()
{

    cpu_set_cr0(cpu_get_cr0() | 0x80000000);

}

static void set_directory_table(struct mmu_directory *directory, unsigned int frame, struct mmu_table *table, unsigned int tflags)
{

    directory->tables[frame / MMU_TABLE_SLOTS] = (struct mmu_table *)((unsigned int)table | tflags);

}

static void set_table_page(struct mmu_table *table, unsigned int frame, unsigned int page, unsigned int pflags)
{

    table->pages[frame % MMU_PAGE_SLOTS] = (void *)(page | pflags);

}

static void map_memory(struct mmu_directory *directory, struct mmu_table *table, unsigned int paddress, unsigned int vaddress, unsigned int size, unsigned int tflags, unsigned int pflags)
{

    unsigned int frame = vaddress / MMU_PAGE_SIZE;
    unsigned int i;

    memory_clear(table, sizeof (struct mmu_table));

    for (i = 0; i < size / MMU_PAGE_SIZE; i++)
        set_table_page(table, frame + i, paddress + i * MMU_PAGE_SIZE, pflags);

    set_directory_table(directory, frame, table, tflags);

}

void mmu_load_memory(unsigned int index)
{

    cpu_set_cr3((unsigned int)&runtimeDirectories[index]);

}

void mmu_map_kernel_memory(unsigned int index, unsigned int paddress, unsigned int vaddress, unsigned int size)
{

    unsigned int i;

    for (i = 0; i < 32; i++)
        map_memory(&runtimeDirectories[i], &kernelTables[index], paddress, vaddress, size, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE);

}

void mmu_map_user_memory(unsigned int index, unsigned int paddress, unsigned int vaddress, unsigned int size)
{

    map_memory(&runtimeDirectories[index], &runtimeTables[index], paddress, vaddress, size, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE | MMU_TABLE_FLAG_USERMODE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE | MMU_PAGE_FLAG_USERMODE);

}

void mmu_reload_memory()
{

    cpu_set_cr3(cpu_get_cr3());

}

static void handle_interrupt(struct isr_registers *registers)
{

    unsigned int address = cpu_get_cr2();

    error_register(1, address);
    error_register(2, registers->extra);

}

void mmu_setup_arch(unsigned short selector)
{

    idt_set_entry(0x0E, mmu_routine, selector, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
    isr_set_routine(0x0E, handle_interrupt);

    mmu_map_kernel_memory(0, ARCH_KERNEL_BASE, ARCH_KERNEL_BASE, ARCH_KERNEL_SIZE);
    mmu_map_user_memory(1, RUNTIME_TASK_PADDRESS_BASE + 1 * RUNTIME_TASK_ADDRESS_SIZE, RUNTIME_TASK_VADDRESS_BASE, RUNTIME_TASK_ADDRESS_SIZE);
    mmu_load_memory(1);
    enable();

}

