#include <memory.h>
#include <error.h>
#include <runtime.h>
#include <arch/x86/cpu.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/isr.h>

static void (*routines[ISR_TABLE_SLOTS])(struct isr_registers *registers);

static unsigned short load_kstate(struct isr_registers *registers)
{

    registers->interrupt.cs = gdt_get_selector(GDT_INDEX_KCODE);
    registers->interrupt.eip = (unsigned int)cpu_halt;
    registers->interrupt.esp = 0x00400000;
    registers->general.ebp = 0;
    registers->general.eax = 0;

    return gdt_get_selector(GDT_INDEX_KDATA);

}

static unsigned short load_ustate(struct runtime_task *task, struct isr_registers *registers)
{

    registers->interrupt.cs = gdt_get_selector(GDT_INDEX_UCODE);
    registers->interrupt.eip = task->registers.ip;
    registers->interrupt.esp = task->registers.sp;
    registers->general.ebp = task->registers.sb;
    registers->general.eax = task->registers.status;

    return gdt_get_selector(GDT_INDEX_UDATA);

}

unsigned short isr_raise(struct isr_registers *registers)
{

    struct runtime_task *task = runtime_get_task();

    runtime_init_registers(&task->registers, registers->interrupt.eip, registers->interrupt.esp, registers->general.ebp, registers->general.eax);

    task->notify_interrupt(task, registers->index);

    routines[registers->index](registers);

    task->notify_complete(task);

    task = runtime_get_task();

    if (task->status.used && !task->status.idle)
        return load_ustate(task, registers);

    return load_kstate(registers);

}

void isr_set_routine(unsigned int index, void (*routine)(struct isr_registers *registers))
{

    if (index >= ISR_TABLE_SLOTS)
        return;

    routines[index] = routine;

}

void isr_unset_routine(unsigned int index)
{

    if (index >= ISR_TABLE_SLOTS)
        return;

    routines[index] = 0;

}

void isr_setup(unsigned short selector)
{

    unsigned int i;

    for (i = 0; i < ISR_TABLE_SLOTS; i++)
        idt_set_entry(i, isr_undefined, selector, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);

}

