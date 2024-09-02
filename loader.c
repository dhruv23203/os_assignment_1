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
    fd = open(exe[0], O_RDONLY);
    if (fd < 0) {
        printf("Failed to open file: %s\n", exe[0]);
        exit(EXIT_FAILURE);
    }

    // Load the ELF header
    ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        printf("Failed to read ELF header\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Validate the ELF magic number
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Invalid ELF file\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for program headers
    phdr = (Elf32_Phdr*)malloc(ehdr->e_phentsize * ehdr->e_phnum);
    lseek(fd, ehdr->e_phoff, SEEK_SET);
    if (read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum) != ehdr->e_phentsize * ehdr->e_phnum) {
        printf("Failed to read program headers\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Iterate over the program headers to load segments
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Map memory for the segment
            void* segment = mmap((void*)phdr[i].p_vaddr, phdr[i].p_memsz,
                                 PROT_READ | PROT_WRITE | PROT_EXEC,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (segment == MAP_FAILED) {
                printf("Failed to map segment at address %p\n", (void*)phdr[i].p_vaddr);
                exit(EXIT_FAILURE);
            }

            // Copy segment contents from file to memory
            lseek(fd, phdr[i].p_offset, SEEK_SET);
            if (read(fd, segment, phdr[i].p_filesz) != phdr[i].p_filesz) {
                printf("Failed to load segment from offset %d\n", (int)phdr[i].p_offset);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Navigate to the entry point address
    entry_point = (void*)ehdr->e_entry;
    int (*_start)() = entry_point;

    // Execute the _start function and print its return value
    int result = _start();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable> \n", argv[0]);
        exit(1);
    }

    // Load and execute the ELF file
    load_and_run_elf(&argv[1]);

    // Perform cleanup after execution
    loader_cleanup();

    return 0;
}

