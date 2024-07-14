#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>

// This function takes an ELF program header type (Elf32_Word type) as input and returns a string representing the type.
// ELF program headers have types represented by integer constants.
//  Using this function, we can convert these constants into human-readable strings. This makes the output more understandable.
// For functions that print or display program header information (like print_phdr_info), 
// converting numeric types to strings makes the output more aligned with what tools like readelf display.
const char* segment_type(Elf32_Word type) {
    switch (type) {
        case PT_NULL: return "NULL"; // Indicates an unused header.
        case PT_LOAD: return "LOAD"; // Describes a loadable segment in the file. This is crucial for mapping the executable into memory.
        case PT_DYNAMIC: return "DYNAMIC"; // Contains dynamic linking information. Our loader does not support dynamic executables, so recognizing this type helps in filtering unsupported files.
        case PT_INTERP: return "INTERP"; //  Specifies the path to the interpreter needed for dynamic linking. Again, important for filtering unsupported files.
        case PT_NOTE: return "NOTE"; // Contains auxiliary information.
        case PT_SHLIB: return "SHLIB"; // Reserved but has unspecified semantics.
        case PT_PHDR: return "PHDR"; // escribes the location and size of the program header table itself.
        case PT_LOPROC: return "LOPROC"; // Reserved for processor-specific semantics
        case PT_HIPROC: return "HIPROC"; // Reserved for processor-specific semantics
        default: return "UNKNOWN";
    }
}

// This function converts the ELF segment flags (p_flags) into protection flags used by the mmap system call.
// Input Parameter: Elf32_Word p_flags - The segment flags from an ELF program header, which indicate the permissions for that segment (read, write, execute).
// Return Type: The function returns an integer representing the combined protection flags suitable for use with mmap.
int get_protection_flags(Elf32_Word p_flags) {
    int prot = 0;
    if (p_flags & PF_R) prot |= PROT_READ; // Check if the ELF segment flag PF_R (read permission) is set. If it is, add the PROT_READ flag to prot.
    if (p_flags & PF_W) prot |= PROT_WRITE; // Check if the ELF segment flag PF_W (write permission) is set. If it is, add the PROT_WRITE flag to prot.
    if (p_flags & PF_X) prot |= PROT_EXEC; // Check if the ELF segment flag PF_X (execute permission) is set. If it is, add the PROT_EXEC flag to prot.
    return prot; // Return the combined protection flags.
}

// This function prints detailed information about an ELF program header and the corresponding memory mapping flags.
// Input Parameters: 
// Elf32_Phdr *phdr: A pointer to the program header structure.
// int index: The index of the program header, though it's not used in the function.
void print_phdr_info(Elf32_Phdr *phdr, int index) {
    const char *type = segment_type(phdr->p_type); // Converts the segment type (an integer) into a human-readable string.
    int prot_flags = get_protection_flags(phdr->p_flags); // Converts the ELF segment flags into mmap protection flags.
    int map_flags = MAP_PRIVATE | MAP_FIXED; // Sets the flags for the mmap system call.
    // MAP_PRIVATE: tell the mapping not to affect the file
    // MAP_FIXED: force the mapping to use the starting address you want

    // Print Program Header Information:
    printf("%-8s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x ", type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz);

    // Print Flags - Prints the segment permissions (read, write, execute).
    printf("%c%c%c ", 
        (phdr->p_flags & PF_R) ? 'R' : ' ',
        (phdr->p_flags & PF_W) ? 'W' : ' ',
        (phdr->p_flags & PF_X) ? 'E' : ' ');

    // Prints the alignment requirement for the segment.
    printf("0x%x ", phdr->p_align);

    // Print Protection Flags:
    printf("Prot: %s%s%s ", 
        (prot_flags & PROT_READ) ? "R" : "", 
        (prot_flags & PROT_WRITE) ? "W" : "", 
        (prot_flags & PROT_EXEC) ? "E" : "");

    // Prints the mapping flags used for the mmap call
    printf("Map: %s\n", (map_flags & MAP_FIXED) ? "FIXED PRIVATE" : "");
}

// This function iterates over each program header in an ELF file and applies a given function (func) to each header.
// Input Parameters:
// void *map_start: A pointer to the start of the mapped ELF file in memory. (map_start: The address in virtual memory the executable is mapped to.)
// A function pointer to the function that will be applied to each program header. This function takes two arguments: a pointer to the program header (Elf32_Phdr *) and an integer index (int). (func: the function which will be applied to each Phdr.)
// int arg: An additional argument that can be passed to func
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start; // Casts the map_start pointer to an Elf32_Ehdr pointer to access the ELF header.
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff); // The e_phoff field in the ELF header (ehdr) gives the offset to the program header table from the start of the file. By adding this offset to map_start, we get the address of the first program header.

    // Loops through each program header and applies the given function (func) to it.
    // ehdr->e_phnum gives the number of entries in the program header table.
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], i);
    }
    return 0;
}

// Function to check if the executable is static
int is_static_executable(void *map_start) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC || phdr[i].p_type == PT_INTERP) {
            return 0; // Not a static executable
        }
    }
    return 1; // Static executable
}

int main(int argc, char **argv) {
    if (argc != 2) { // Ensures that the program receives exactly one argument, which is the ELF executable filename.
        fprintf(stderr, "Usage: %s <executable>\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY); // Opens the file and retrieves its file descriptor.
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    off_t size = lseek(fd, 0, SEEK_END); // Determine File Size: 
    if (size == -1) {
        perror("lseek");
        close(fd);
        exit(1);
    }

    void *map_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0); // Maps the entire file into the process's memory using mmap.
    // mmap: A system call that maps files or devices into memory.
    // This statement maps the entire file into the process's memory, allowing the file to be accessed as if it were in memory.
    // On success, mmap returns a pointer to the mapped area.
    if (map_start == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }

    close(fd);

    if (!is_static_executable(map_start)) {
        fprintf(stderr, "Error: Only static executables are supported.\n");
        munmap(map_start, size);
        exit(1);
    }

    // Print the header for the output
    printf("Type     Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align    Prot  Map\n");

    // Iterate over program headers and print their information
    foreach_phdr(map_start, print_phdr_info, 0);

    if (munmap(map_start, size) == -1) {
        perror("munmap");
        exit(1);
    }

    return 0;
}

/*
Type: Type of the segment (e.g., LOAD).
Offset: Offset in the file where the segment begins.
VirtAddr: Virtual address in memory where the segment should be loaded.
PhysAddr: Physical address (same as virtual address in this context).
FileSiz: Size of the segment in the file.
MemSiz: Size of the segment in memory.
Flg: Flags indicating permissions (R = Read, W = Write, E = Execute).
Align: Alignment requirement for the segment.
Prot: Protection flags for mmap (R = Read, W = Write, E = Execute).
Map: Mapping flags for mmap (FIXED PRIVATE).
*/
