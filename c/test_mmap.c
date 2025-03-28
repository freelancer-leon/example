#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

static long pagesize;

/*
 * Convert a user mode virtual address belonging to the
 * current process to physical.
 * Does not handle huge pages.
 */
/*
 * get information about address from /proc/self/pagemap
 */
unsigned long long vtop(unsigned long long addr)
{
	static int pagesize;
	unsigned long long pinfo;
	long offset;
	int fd;

	if (pagesize == 0)
		pagesize = getpagesize();
	offset = addr / pagesize * (sizeof pinfo);
	fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd == -1) {
		perror("pagemap");
		exit(1);
	}
	if (pread(fd, &pinfo, sizeof pinfo, offset) != sizeof pinfo) {
		perror("pagemap");
		exit(1);
	}
	close(fd);
	if ((pinfo & (1ull << 63)) == 0) {
		printf("page not present\n");
		return ~0ull;
	}
	return ((pinfo & 0x007fffffffffffffull) * pagesize) + (addr & (pagesize - 1));
}

static void *data_alloc(void)
{
    char    *p = mmap(NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
    int i;

    if (p == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    srandom(getpid() * time(NULL));
    for (i = 0; i < pagesize; i++)
        p[i] = random();
    return p + pagesize / 4;
}

int main()
{
	void *vaddr = NULL;
	long long paddr = 0;

	pagesize = getpagesize();
	vaddr = data_alloc();
	paddr = vtop((long long)vaddr);

	printf("vaddr = %p paddr = %llx\n", vaddr, paddr);
	munmap(vaddr, pagesize);

	return 0;
}
