/*
 * Stage 1.1: user-space page protection in a forked child process.
 *
 * The child revokes access to one anonymous page and intentionally reads it.
 * The parent remains unaffected and verifies the child's SIGSEGV via waitpid.
 */
#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_VALUE 0x5aU

int main(void)
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        return EXIT_FAILURE;
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        perror("sysconf(_SC_PAGESIZE)");
        return EXIT_FAILURE;
    }

    volatile unsigned char *page = mmap(NULL, (size_t)page_size,
                                        PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    page[0] = TEST_VALUE;
    printf("PARENT: pid=%ld, page=%p, bytes=%ld, initial=0x%02x\n",
           (long)getpid(), (void *)page, page_size, (unsigned)page[0]);

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        munmap((void *)page, (size_t)page_size);
        return EXIT_FAILURE;
    }

    if (child == 0) {
        struct rlimit no_core = { .rlim_cur = 0, .rlim_max = 0 };
        if (setrlimit(RLIMIT_CORE, &no_core) != 0) {
            perror("child: setrlimit");
            _exit(10);
        }

        printf("CHILD: pid=%ld, inherited=0x%02x\n",
               (long)getpid(), (unsigned)page[0]);

        if (mprotect((void *)page, (size_t)page_size, PROT_NONE) != 0) {
            perror("child: mprotect");
            _exit(11);
        }

        puts("CHILD: PROT_NONE gesetzt; absichtlicher Lesezugriff folgt");
        unsigned char unexpected = page[0]; /* volatile: load cannot be optimized away */
        printf("FAIL: Kinderprozess las unerwartet 0x%02x\n", (unsigned)unexpected);
        _exit(12);
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited != child) {
        perror("parent: waitpid");
        munmap((void *)page, (size_t)page_size);
        return EXIT_FAILURE;
    }

    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGSEGV) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "FAIL: Kind mit Exit-Code %d beendet (statt SIGSEGV)\n",
                    WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "FAIL: Kind bekam Signal %d (statt SIGSEGV)\n",
                    WTERMSIG(status));
        } else {
            fputs("FAIL: unerwarteter waitpid-Status\n", stderr);
        }
        munmap((void *)page, (size_t)page_size);
        return EXIT_FAILURE;
    }

    puts("PASS: Kind wurde durch SIGSEGV beendet (Speicherschutz aktiv)");

    if (page[0] != TEST_VALUE) {
        fputs("FAIL: Elternprozess konnte seine Seite nicht unveraendert lesen\n",
              stderr);
        munmap((void *)page, (size_t)page_size);
        return EXIT_FAILURE;
    }

    printf("PASS: Elternprozess liest weiterhin 0x%02x\n", (unsigned)page[0]);
    if (munmap((void *)page, (size_t)page_size) != 0) {
        perror("munmap");
        return EXIT_FAILURE;
    }

    puts("PASS: Gesamttest abgeschlossen");
    return EXIT_SUCCESS;
}
