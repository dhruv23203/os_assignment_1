#include "loader.h"

// Global variables for ELF header, program header, and file descriptor
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void* entry_point = NULL;

/*
 * Release memory and other cleanups
 */
void loader_cleanup() {
    // Close the file descriptor
    if (fd >= 0) {
        close(fd);
    }

    // Free allocated memory
    if (ehdr) {
        free(ehdr);
    }
    if (phdr) {
        free(phdr);
    }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    // Open the ELF file
    fd = open(exe[0], O_RDONLY);
    assert(fd >= 0 && "Failed to open file");

    // Allocate memory for the ELF header
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    assert(ehdr != NULL && "Failed to allocate memory for ELF header");

    // Read the ELF header from the file
    int read_size = read(fd, ehdr, sizeof(Elf32_Ehdr));
    assert(read_size == sizeof(Elf32_Ehdr) && "Failed to read ELF header");

    // Validate the ELF magic number
    assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0 && "Invalid ELF file");

    // Allocate memory for the program headers
    phdr = (Elf32_Phdr*)malloc(ehdr->e_phentsize * ehdr->e_phnum);
    assert(phdr != NULL && "Failed to allocate memory for program headers");

    // Seek to the program header table in the ELF file
    int offset = lseek(fd, ehdr->e_phoff, SEEK_SET);
    assert(offset != -1 && "Failed to seek to program headers");

    // Read the program headers
    read_size = read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum);
    assert(read_size == ehdr->e_phentsize * ehdr->e_phnum && "Failed to read program headers");

    // Iterate over the program headers to load segments
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Map memory for the segment
            void* segment = mmap((void*)phdr[i].p_vaddr, phdr[i].p_memsz,
                                 PROT_READ | PROT_WRITE | PROT_EXEC,
                                 MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
            assert(segment != MAP_FAILED && "Failed to map segment");

            // Copy segment contents from file to memory
            offset = lseek(fd, phdr[i].p_offset, SEEK_SET);
            assert(offset != -1 && "Failed to seek to segment");

            read_size = read(fd, segment, phdr[i].p_filesz);
            assert(read_size == phdr[i].p_filesz && "Failed to load segment");
        }
    }

    // Navigate to the entry point address
    entry_point = (void*)ehdr->e_entry;
    assert(entry_point != NULL && "Invalid entry point");

    int (*_start)() = entry_point;

    // Execute the _start function and print its return value
    int result = _start();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char** argv) {
    assert(argc == 2 && "Usage: <program> <ELF Executable>");

    // Load and execute the ELF file
    load_and_run_elf(&argv[1]);

    // Perform cleanup after execution
    loader_cleanup();

    return 0;
}

