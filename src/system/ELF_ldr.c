#include "ELF_ldr.h"
#include "File.h"
#include "Stdout.h"

#define ELF_NIDENT	16

typedef u16 Elf32_Half;	 // Unsigned half int
typedef u32 Elf32_Off;	 // Unsigned offset
typedef u32 Elf32_Addr;	 // Unsigned address
typedef u32 Elf32_Word;	 // Unsigned int
typedef s32 Elf32_Sword; // Signed int

typedef struct
{
    u8          e_ident[ELF_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
    Elf32_Word  p_type;
    Elf32_Off   p_offset;
    Elf32_Addr  p_vaddr;
    Elf32_Addr  p_paddr;
    Elf32_Word  p_filesz;
    Elf32_Word  p_memsz;
    Elf32_Word  p_flags;
    Elf32_Word  p_align;
} Elf32_Phdr;

enum Elf_Ident
{
    EI_MAG0         = 0,    // 0x7F
    EI_MAG1         = 1,    // 'E'
    EI_MAG2         = 2,    // 'L'
    EI_MAG3         = 3,    // 'F'
    EI_CLASS        = 4,    // Architecture (32/64)
    EI_DATA         = 5,    // Byte Order
    EI_VERSION      = 6,    // ELF Version
    EI_OSABI        = 7,    // OS Specific
    EI_ABIVERSION   = 8,    // OS Specific
    EI_PAD          = 9     // Padding
};

#define ELFMAG0 0x7F    // e_ident[EI_MAG0]
#define ELFMAG1 'E'     // e_ident[EI_MAG1]
#define ELFMAG2 'L'     // e_ident[EI_MAG2]
#define ELFMAG3 'F'     // e_ident[EI_MAG3]

#define ELFDATA2MSB (2) // Little Endian
#define ELFCLASS32  (1) // 32-bit Architecture

enum Elf_Type
{
    ET_NONE = 0,    // Unkown Type
    ET_REL  = 1,    // Relocatable File
    ET_EXEC = 2     // Executable File
};

#define EM_68K      (4) // x86 Machine Type
#define EV_CURRENT  (1) // ELF Current Version


bool ELF_CheckFile(Elf32_Ehdr *hdr)
{
    if (!hdr) return FALSE;

    if (hdr->e_ident[EI_MAG0] != ELFMAG0)
    {
        Stdout_Push("ELF Header EI_MAG0 incorrect.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_MAG1] != ELFMAG1)
    {
        Stdout_Push("ELF Header EI_MAG1 incorrect.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_MAG2] != ELFMAG2)
    {
        Stdout_Push("ELF Header EI_MAG2 incorrect.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        Stdout_Push("ELF Header EI_MAG3 incorrect.\n");
        return FALSE;
    }

    return TRUE;
}

bool ELF_CheckSupported(Elf32_Ehdr *hdr)
{
    if (!ELF_CheckFile(hdr))
    {
        Stdout_Push("Invalid ELF File.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
    {
        Stdout_Push("Unsupported ELF File Class.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_DATA] != ELFDATA2MSB)
    {
        Stdout_Push("Unsupported ELF File byte order.\n");
        return FALSE;
    }
    if (hdr->e_machine != EM_68K)
    {
        Stdout_Push("Unsupported ELF File target.\n");
        return FALSE;
    }
    if (hdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        Stdout_Push("Unsupported ELF File version.\n");
        return FALSE;
    }
    if (hdr->e_type != ET_REL && hdr->e_type != ET_EXEC)
    {
        Stdout_Push("Unsupported ELF File type.\n");
        return FALSE;
    }

    return TRUE;
}

// Temp
void *proc_space = NULL;
void *ram_space = NULL;

void *ELF_LoadProc(const char *fn)
{
    SM_File *file = F_Open(fn, FM_RDONLY);

    if (file == NULL)
    {
        stdout_printf("Failed to load \"%s\"\n", fn);
        return NULL;
    }

    Elf32_Ehdr hdr;

    F_Read(&hdr, sizeof(Elf32_Ehdr), 1, file);

    if (ELF_CheckSupported(&hdr) == FALSE)
    {
        F_Close(file);
        return NULL;
    }

    //stdout_printf("phoff: $%lX - phentsize: $%X - phnum: $%X\n", hdr.e_phoff, hdr.e_phentsize, hdr.e_phnum);

    Elf32_Phdr phdr[hdr.e_phnum];

    if (hdr.e_phoff == 0)
    {
        stdout_printf("ELF has no program header table!\n");
        F_Close(file);
        return NULL;
    }

    F_Seek(file, hdr.e_phoff, SEEK_SET);
    F_Read(&phdr, hdr.e_phentsize, hdr.e_phnum, file);

    // this might not always be TRUE...
    //u32 top = hdr.e_entry;

    for (u8 p = 0; p < 1; p++)//hdr.e_phnum; p++)
    {
        /*stdout_printf("type: $%lX\n", phdr[p].p_type);
        stdout_printf("offset: $%lX\n", phdr[p].p_offset);
        stdout_printf("vaddr: $%lX\n", phdr[p].p_vaddr);
        stdout_printf("paddr: $%lX\n", phdr[p].p_paddr);
        stdout_printf("filesz: $%lX\n", phdr[p].p_filesz);
        stdout_printf("memsz: $%lX\n", phdr[p].p_memsz);
        stdout_printf("flags: $%lX\n", phdr[p].p_flags);
        stdout_printf("align: $%lX\n", phdr[p].p_align);

        kprintf("type: $%lX", phdr[p].p_type);
        kprintf("offset: $%lX", phdr[p].p_offset);
        kprintf("vaddr: $%lX", phdr[p].p_vaddr);
        kprintf("paddr: $%lX", phdr[p].p_paddr);
        kprintf("filesz: $%lX", phdr[p].p_filesz);
        kprintf("memsz: $%lX", phdr[p].p_memsz);
        kprintf("flags: $%lX", phdr[p].p_flags);
        kprintf("align: $%lX", phdr[p].p_align);*/

        if (phdr[p].p_type != 1) continue;  // Not a loadable segment

        if (p > 0) stdout_printf("Warning: more than one loadable segment. TODO: investigate\n");

        proc_space = MEM_allocAt(hdr.e_entry, phdr[p].p_memsz);
        //ram_space  = MEM_allocAt(0xE0FFB000, 0x2000);

        if (proc_space == NULL)
        {
            kprintf("Failed to allocate proc_space");
        }

        memset((void*)hdr.e_entry, 0, phdr[p].p_memsz);

        F_Seek(file, phdr[p].p_vaddr, SEEK_SET);
        F_Read(proc_space, phdr[p].p_memsz, 1, file);
    }

    //kprintf("e_entry: $%lX - p_vaddr: $%lX - p_memsz: $%lX", hdr.e_entry, phdr[0].p_vaddr, phdr[0].p_memsz);

    F_Close(file);

    return (void*)hdr.e_entry;
}
