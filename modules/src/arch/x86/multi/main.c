#include <memory.h>
#include <vfs.h>
#include <binary.h>
#include <runtime.h>
#include <syscall.h>
#include <arch/x86/arch.h>
#include <arch/x86/isr.h>
#include <arch/x86/mmu.h>
#include <arch/x86/syscall.h>
#include <arch/x86/multi/multi.h>

static struct multi_task *tasks[MULTI_TASK_SLOTS];

static struct multi_task *get_task(unsigned int index)
{

    if (!index || index >= MULTI_TASK_SLOTS)
        return 0;

    return tasks[index] = (struct multi_task *)(MULTI_TASK_BASE + index * MMU_PAGE_SIZE * 8);

}

static unsigned int get_task_slot()
{

    unsigned int i;

    for (i = 1; i < MULTI_TASK_SLOTS; i++)
    {

        if (tasks[i])
            continue;

        return i;

    }

    return 0;

}

static struct runtime_task *schedule(struct runtime_task *self)
{

    unsigned int i;

    for (i = MULTI_TASK_SLOTS - 1; i > 0; i--)
    {

        if (!tasks[i])
            continue;

        if (!tasks[i]->base.status.used)
        {

            tasks[i] = 0;

            continue;

        }

        if (tasks[i]->base.status.idle)
            continue;

        mmu_load_memory(&tasks[i]->directory);

        return &tasks[i]->base;

    }

    return self;

}

static struct runtime_task *notify_interrupt(struct runtime_task *self, unsigned int index)
{

    unsigned int i;

    for (i = 1; i < MULTI_TASK_SLOTS - 1; i++)
    {

        if (!tasks[i])
            continue;

        if (!tasks[i]->base.status.used)
            continue;

        if (!tasks[i]->base.status.idle)
            continue;

        if (!tasks[i]->base.events[index].callback)
            continue;

        runtime_init_registers(&tasks[i]->base.registers, tasks[i]->base.events[index].callback, RUNTIME_STACK_VADDRESS_BASE, RUNTIME_STACK_VADDRESS_BASE, tasks[i]->base.registers.status);

        tasks[i]->base.status.idle = 0;

    }

    return schedule(self);

}

static unsigned int spawn(struct runtime_task *task, void *stack)
{

    struct multi_spawn_args *args = stack;
    struct runtime_descriptor *descriptor = runtime_get_task_descriptor(task, args->index);
    unsigned int index = get_task_slot();
    struct multi_task *ntask = get_task(index);
    unsigned int entry;

    if (!descriptor || !descriptor->interface->read)
        return 0;

    if (!ntask)
        return 0;

    mmu_map_memory(&ntask->directory, &ntask->tables[0], ARCH_KERNEL_BASE, ARCH_KERNEL_BASE, ARCH_KERNEL_SIZE, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE);
    mmu_map_memory(&ntask->directory, &ntask->tables[1], RUNTIME_TASK_PADDRESS_BASE + index * RUNTIME_TASK_ADDRESS_SIZE, RUNTIME_TASK_VADDRESS_BASE, RUNTIME_TASK_ADDRESS_SIZE, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE | MMU_TABLE_FLAG_USERMODE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE | MMU_PAGE_FLAG_USERMODE);
    mmu_map_memory(&ntask->directory, &ntask->tables[2], RUNTIME_STACK_PADDRESS_BASE + index * RUNTIME_STACK_ADDRESS_SIZE, RUNTIME_STACK_VADDRESS_BASE - RUNTIME_STACK_ADDRESS_SIZE, RUNTIME_STACK_ADDRESS_SIZE, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE | MMU_TABLE_FLAG_USERMODE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE | MMU_PAGE_FLAG_USERMODE);
    mmu_load_memory(&ntask->directory);

    entry = binary_copy_program(descriptor->interface, descriptor->id);

    if (!entry)
        return 0;

    memory_copy(&ntask->base, task, sizeof (struct runtime_task));

    runtime_init_registers(&ntask->base.registers, entry, RUNTIME_STACK_VADDRESS_BASE, RUNTIME_STACK_VADDRESS_BASE, index);

    task->notify_interrupt = notify_interrupt;
    ntask->base.notify_interrupt = notify_interrupt;

    return index;

}

void init()
{

    memory_clear(tasks, sizeof (struct multi_task *) * MULTI_TASK_SLOTS);

    syscall_set_routine(SYSCALL_INDEX_SPAWN, spawn);

}

void destroy()
{

}

