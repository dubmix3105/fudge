#include <lib/elf.h>
#include <lib/file.h>
#include <lib/memory.h>
#include <lib/string.h>
#include <kernel/vfs.h>
#include <kernel/modules.h>
#include <kernel/arch/x86/arch.h>
#include <kernel/arch/x86/isr.h>
#include <kernel/arch/x86/mmu.h>
#include <kernel/arch/x86/syscall.h>
#include <modules/elf/elf.h>

static void *syscallRoutines[SYSCALL_ROUTINES_SIZE];

void syscall_register_handler(unsigned char index, void (*handler)(struct syscall_registers *registers))
{

    syscallRoutines[index] = handler;

}

void syscall_unregister_handler(unsigned char index)
{

    syscallRoutines[index] = 0;

}

void syscall_handler(struct syscall_registers *registers)
{

    void (*handler)(struct syscall_registers *registers) = syscallRoutines[registers->eax];

    if (handler)
        handler(registers);

}

static void syscall_open(struct syscall_registers *registers)
{

    char *path = (char *)registers->esi;

    registers->eax = vfs_open(path);

}

static void syscall_close(struct syscall_registers *registers)
{

    unsigned int fd = registers->ebx;

    vfs_close(fd);

}

static void syscall_read(struct syscall_registers *registers)
{

    unsigned int fd = registers->ebx;
    char *buffer = (char *)registers->esi;
    unsigned int count = registers->ecx;

    struct vfs_node *node = vfs_get_descriptor(fd)->node;

    if (!(node && node->operations.read))
    {

        registers->eax = 0;

        return;

    }

    registers->eax = node->operations.read(node, count, buffer);

}

static void syscall_write(struct syscall_registers *registers)
{

    unsigned int fd = registers->ebx;
    char *buffer = (char *)registers->esi;
    unsigned int count = registers->ecx;

    struct vfs_node *node = vfs_get_descriptor(fd)->node;

    if (!(node && node->operations.write))
    {

        registers->eax = 0;

        return;

    }

    registers->eax = node->operations.write(node, count, buffer);

}

static void syscall_info(struct syscall_registers *registers)
{

    unsigned int fd = registers->ebx;
    struct file_info *info = (struct file_info *)registers->edi;
    
    struct vfs_node *node = vfs_get_descriptor(fd)->node;

    if (!node)
    {

        registers->eax = 0;

        return;

    }

    info->id = node->id;
    info->length = 0;

    registers->eax = 1;

}

unsigned int oldEip;
struct mmu_header *oldHeader;

static void syscall_exit()
{

    file_write_format(2, "EXIT\n");

    for(;;);

}

static void syscall_map(struct syscall_registers *registers)
{

    unsigned int fd = registers->ebx;

    struct vfs_node *node = vfs_get_descriptor(fd)->node;

    if (!(node && node->operations.read))
    {

        registers->eax = 0;

        return;

    }

    void *address = mmu_get_slot();

    node->operations.read(node, 0x10000, address);

    if (!elf_check(address))
    {

        registers->eax = 0;

        return;

    }

    struct elf_header *header = (struct elf_header *)address;
    struct elf_program_header *programHeader = (struct elf_program_header *)(address + header->programHeaderOffset);

    struct mmu_header *pHeader = mmu_get_program_header(address);

    mmu_map_header(pHeader, programHeader->virtualAddress, address, 0x10000, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE | MMU_TABLE_FLAG_USERMODE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE | MMU_PAGE_FLAG_USERMODE);
    mmu_set_directory(&pHeader->directory);

    // This is for testing purposes
    if (registers->eip > 0x00800000)
    {

        memory_set(header->entry + 0x7FFC, 0, 4); // second argument
        memory_set(header->entry + 0x7FF8, 0, 4); // first argument argc
        memory_set(header->entry + 0x7FF8, 1, 1); // first argument argc
        memory_set(header->entry + 0x7FF7, ((unsigned int)syscall_exit & 0xFF000000) >> 24, 1); // eip
        memory_set(header->entry + 0x7FF6, ((unsigned int)syscall_exit & 0x00FF0000) >> 16, 1); // eip
        memory_set(header->entry + 0x7FF5, ((unsigned int)syscall_exit & 0x0000FF00) >> 8, 1); // eip
        memory_set(header->entry + 0x7FF4, ((unsigned int)syscall_exit & 0x000000FF) >> 0, 1); // eip

        registers->eip = (unsigned int)header->entry;
        registers->useresp = (unsigned int)(header->entry + 0x7FF4); // set esp to eip
        registers->eax = 0;

        return;

    }

    registers->eax = (unsigned int)header->entry;

}

static void syscall_execute(struct syscall_registers *registers)
{

    registers->eax = 0;

}

static void syscall_reboot(struct syscall_registers *registers)
{

    arch_reboot();

}

void syscall_init()
{

    syscall_register_handler(SYSCALL_ROUTINE_OPEN, syscall_open);
    syscall_register_handler(SYSCALL_ROUTINE_CLOSE, syscall_close);
    syscall_register_handler(SYSCALL_ROUTINE_READ, syscall_read);
    syscall_register_handler(SYSCALL_ROUTINE_WRITE, syscall_write);
    syscall_register_handler(SYSCALL_ROUTINE_INFO, syscall_info);
    syscall_register_handler(SYSCALL_ROUTINE_MAP, syscall_map);
    syscall_register_handler(SYSCALL_ROUTINE_EXECUTE, syscall_execute);
    syscall_register_handler(SYSCALL_ROUTINE_REBOOT, syscall_reboot);

}

