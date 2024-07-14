/*
Why the Linking Script is Needed

Address Space Clashes:
Problem: Without a custom linking script, the loader and the loaded program might be mapped to the same default virtual memory addresses, typically starting at 0x08048000. This would lead to a collision between the loader and the loaded executable, causing undefined behavior or crashes.
Solution: The linking script specifies a different starting address (0x04048000) for the loader, ensuring there is no overlap between the loader and the executable it loads. This prevents memory space clashes by allocating the loader at a lower address.

How You Verified That It Worked

Explanation: Compiling the loader with the custom linking script to ensure it is mapped to the specified lower address. The -T linking_script option tells the linker to use our custom script.
Check the ELF Header: readelf -h loader
ELF Header:
  Entry point address:               0x4048920
  ...
The Entry point address should reflect the address specified in the linking script. In this case, 0x4048920 confirms that the loader is placed at the intended address range.

Check Program Headers:
readelf -l loader
Explanation: This command verifies the program headers and ensures that the segments are correctly mapped to the addresses specified in the linking script.

./loader hello
Explanation: Running the loader with a static ELF executable (hello) demonstrates that the loader can correctly map the executable segments into memory. The printed information confirms that the memory mapping follows the expected layout without any address clashes.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>

#define PAGE_SIZE 4096

extern int startup(int argc, char **argv, void (*start)());

// Function declarations from task 1
const char* segment_type(Elf32_Word type);
int get_protection_flags(Elf32_Word p_flags);
void print_phdr_info(Elf32_Phdr *phdr, int index);
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg);
int is_static_executable(void *map_start);

// Parameters:
// phdr: Pointer to the program header.
// fd: File descriptor of the executable file.
void load_phdr(Elf32_Phdr *phdr, int fd) {
    if (phdr->p_type != PT_LOAD) { // If the segment type is not PT_LOAD, return immediately.
        return;
    }

    // Align the virtual address and file offset to page boundaries
    // The ELF specification requires that loadable segments' virtual addresses and file offsets be aligned to page boundaries. This ensures efficient memory access and compatibility with the operating system's memory management.
    void *aligned_addr = (void *)(phdr->p_vaddr & ~(PAGE_SIZE - 1));
    off_t aligned_offset = phdr->p_offset & ~(PAGE_SIZE - 1);
    size_t offset_diff = phdr->p_vaddr & (PAGE_SIZE - 1);
    size_t map_size = phdr->p_memsz + offset_diff;

    int prot = get_protection_flags(phdr->p_flags);
    int flags = MAP_PRIVATE | MAP_FIXED;

    void *mapped_addr = mmap(aligned_addr, map_size, prot, flags, fd, aligned_offset);
    if (mapped_addr == MAP_FAILED) {
        perror("mmap");
        return;
    }

    print_phdr_info(phdr, 0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <executable>\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        perror("lseek");
        close(fd);
        exit(1);
    }

    void *map_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }

    if (!is_static_executable(map_start)) {
        fprintf(stderr, "Error: Only static executables are supported.\n");
        munmap(map_start, size);
        close(fd);
        exit(1);
    }

    printf("Type     Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align    Prot  Map\n");

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        load_phdr(&phdr[i], fd);
    }

    void (*entry_point)() = (void (*)())ehdr->e_entry;

    close(fd);

    startup(argc - 1, &argv[1], entry_point);

    if (munmap(map_start, size) == -1) {
        perror("munmap");
        close(fd);
        exit(1);
    }

    return 0;
}

// Implementation of function declarations from task 1

const char* segment_type(Elf32_Word type) {
    switch (type) {
        case PT_NULL: return "NULL";
        case PT_LOAD: return "LOAD";
        case PT_DYNAMIC: return "DYNAMIC";
        case PT_INTERP: return "INTERP";
        case PT_NOTE: return "NOTE";
        case PT_SHLIB: return "SHLIB";
        case PT_PHDR: return "PHDR";
        case PT_LOPROC: return "LOPROC";
        case PT_HIPROC: return "HIPROC";
        default: return "UNKNOWN";
    }
}

int get_protection_flags(Elf32_Word p_flags) {
    int prot = 0;
    if (p_flags & PF_R) prot |= PROT_READ;
    if (p_flags & PF_W) prot |= PROT_WRITE;
    if (p_flags & PF_X) prot |= PROT_EXEC;
    return prot;
}

void print_phdr_info(Elf32_Phdr *phdr, int index) {
    const char *type = segment_type(phdr->p_type);
    int prot_flags = get_protection_flags(phdr->p_flags);
    int map_flags = MAP_PRIVATE | MAP_FIXED;

    printf("%-8s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x ", type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz);

    printf("%c%c%c ", 
        (phdr->p_flags & PF_R) ? 'R' : ' ',
        (phdr->p_flags & PF_W) ? 'W' : ' ',
        (phdr->p_flags & PF_X) ? 'E' : ' ');

    printf("0x%x ", phdr->p_align);

    printf("Prot: %s%s%s ", 
        (prot_flags & PROT_READ) ? "R" : "", 
        (prot_flags & PROT_WRITE) ? "W" : "", 
        (prot_flags & PROT_EXEC) ? "E" : "");

    printf("Map: %s\n", (map_flags & MAP_FIXED) ? "FIXED PRIVATE" : "");
}

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], i);
    }
    return 0;
}

int is_static_executable(void *map_start) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC || phdr[i].p_type == PT_INTERP) {
            return 0;
        }
    }
    return 1;
}