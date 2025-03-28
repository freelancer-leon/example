/*
 * gcc -Wall -lpthread -g -o false-sharing false-sharing.c
 * perf stat -e cache-misses ./false-sharing
 *
 * NOTE: the UC write (option: -t 2) need to access /dev/mem, to archive it:
 *    1. Disable kernel config: "# CONFIG_STRICT_DEVMEM is not set"
 *    2. Disable PAT by kernel option: "nopat"
 */

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* x86 L1 cache line size is 64B */
#define L1_CACHE_SHIFT  (6)
#define L1_CACHE_BYTES  (1 << L1_CACHE_SHIFT)

#ifndef L1_CACHE_ALIGN
#define L1_CACHE_ALIGN(x) __ALIGN_KERNEL(x, L1_CACHE_BYTES)
#endif

#ifndef SMP_CACHE_BYTES
#define SMP_CACHE_BYTES L1_CACHE_BYTES
#endif

#ifndef ____cacheline_aligned
#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))
#endif

typedef unsigned int  u32;
typedef unsigned long u64;

#define FATAL do { fprintf(stderr, "Error %s:%d: (%d) [%s]\n", __FILE__, __LINE__, \
	errno, strerror(errno)); exit(EXIT_FAILURE); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define LOOP_MAX (1000000000)

static int g_infinite;

enum write_type {
	FULL_CACHELINE,	// full cacheline write by default
	NT_WRITE,
	UC_WRTIE,
	WRITE_MAX,
};

// No padding, might be sharing the same cacheline
typedef struct {
	volatile long x;
	volatile long y;
} ____cacheline_aligned false_sharing_t;

// Padding cacheline size, x and y shoud not in the same cacheline
typedef struct {
	volatile long x;
	char padding[SMP_CACHE_BYTES]; // padding 64 bytes which is cacheline size
	volatile long y;
} sharing_t;

void show_help(char *program)
{
	fprintf(stderr, "\nUsage: %s [-t write_type] [-s]\n"
		"\t-t write_type: the type to write memory:\n"
		"\t                 0 - full cacheline\n"
		"\t                 1 - non-temporal write\n"
		"\t                 2 - write uncachable memory\n"
		"\t-s           : sharing cacheline test (false-sharing by default)\n"
		"\t-i           : infinite looping and never exit\n"
		"\t-h           : print this help\n\n",
		program);
}

void get_xy_addresses(int is_sharing, void *data, volatile long **x,
					  volatile long **y)
{
	if (is_sharing) {
		*x = &((sharing_t *)data)->x;
		*y = &((sharing_t *)data)->y;
		assert(*y != (void *)*x + sizeof(*x));
	} else {
		*x = &((false_sharing_t *)data)->x;
		*y = &((false_sharing_t *)data)->y;
		// in false-sharing case, 'y' should next to 'x'
		assert(*y == (void *)*x + sizeof(*x));
	}
}

/*
 * Get information about address from /proc/self/pagemap
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
	    exit(EXIT_FAILURE);
	}

	if (pread(fd, &pinfo, sizeof pinfo, offset) != sizeof pinfo) {
	    perror("pagemap");
	    exit(EXIT_FAILURE);
	}

	close(fd);

	if ((pinfo & (1ull << 63)) == 0) {
	    printf("page not present\n");
	    exit(EXIT_FAILURE);
	    return ~0ull;  // should not reach here
	}

	return ((pinfo & 0x007fffffffffffffull) * pagesize)
			+ (addr & (pagesize - 1));
}

static __always_inline void clflush(volatile void *__p)
{
    asm volatile("clflush %0" : "+m" (*(volatile char *)__p));
}

static inline void clflushopt(volatile void *__p)
{
    asm volatile(".byte 0x3e; clflush %0" : "+m" (*(volatile char *)__p));
}

static __always_inline void nt_mov(void *dst, const void *src, size_t cnt)
{
	switch (cnt) {
	case 4:
		asm ("movntil %1, %0" : "=m"(*(u32 *)dst) : "r"(*(u32 *)src));
		return;
	case 8:
		asm ("movntiq %1, %0" : "=m"(*(u64 *)dst) : "r"(*(u64 *)src));
		return;
	case 16:
		asm ("movntiq %1, %0" : "=m"(*(u64 *)dst) : "r"(*(u64 *)src));
		asm ("movntiq %1, %0" : "=m"(*(u64 *)(dst + 8)) : "r"(*(u64 *)(src + 8)));
		return;
	}
}

void * full_write(void *arg)
{
	volatile long *var = (volatile long*)arg;

	for (int i = 0; g_infinite || i < LOOP_MAX; ++i) {
		(*var)++;
	}
	return NULL;
}

// NT write will invalidate the copy in cache first if there is
void * nt_write(void *arg)
{
	volatile long *var = (volatile long*)arg;

	for (int i = 0; g_infinite || i < LOOP_MAX; ++i) {
		nt_mov((void *)var, (const void *)&i, sizeof(*var));
	}
	return NULL;
}

// NOTE: UC write will by-pass cache, in practice we need to flush cache line
// first before write, otherwise, it will result in unexpected behavior.
// The functions clflush() and clflushopt() has been provided for these.
// But here for testing purpose we ignore is.
void * uc_write(void *arg)
{
	volatile long *var = (volatile long*)arg;

	for (int i = 0; g_infinite || i < LOOP_MAX; ++i) {
		(*var)++;
	}
	return NULL;
}

typedef void * (*write_func)(void *arg);

// Write functions table
static write_func write_func_table[] = {
	[FULL_CACHELINE] = full_write,
	[NT_WRITE]       = nt_write,
	[UC_WRTIE]       = uc_write,
	[WRITE_MAX]      = NULL,
};

int main(int argc, char *argv[])
{
	void *map_base = NULL, *new_virt_addr = NULL, *data = NULL;
	write_func worker = NULL; // full cacheline write by default
	int is_sharing = 0; // false sharing by default
	size_t data_size = 0;
	enum write_type wr_type = FULL_CACHELINE;
	long long paddr = 0;
	volatile long *x, *y;
	pthread_t t1, t2;
	int opt, fd;

	while ((opt = getopt(argc, argv, "hist:")) != -1) {
		switch (opt) {
		case 's':
			is_sharing = 1;
			break;
		case 'i':
			g_infinite = 1;
			break;
		case 't':
			wr_type = atoi(optarg);
			if (wr_type < FULL_CACHELINE || wr_type >= WRITE_MAX) {
				fprintf(stderr, "Error: unrecognized write type.\n");
				show_help(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
		default: /* '?' */
			show_help(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	printf("Size of false sharing data: %zu vs sharing data: %zu\n",
		sizeof(false_sharing_t), sizeof(sharing_t));

	// Allocate memory space
	data_size = is_sharing ? sizeof(sharing_t) : sizeof(false_sharing_t);
	new_virt_addr = data = mmap(NULL, data_size,
							PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
	if (data == MAP_FAILED) FATAL;
	memset(data, 0, data_size);

	if (wr_type == UC_WRTIE) {
		// Get the physical address of 'data'
		paddr = vtop((unsigned long long)data);
		printf("data=%p PA=0x%llx\n", data, paddr);

		// Re-map the page where the 'data' is located with UC memory type,
		if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
		map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
						fd, paddr & ~MAP_MASK);
		if (map_base == MAP_FAILED) FATAL;

		new_virt_addr = map_base + (paddr & MAP_MASK);
	}

	get_xy_addresses(is_sharing, new_virt_addr, &x, &y);

	clflushopt(x);
	if (is_sharing)
		clflushopt(y); // 'y' is not in the same cache line is this case

	worker = write_func_table[wr_type];
	printf("x=%p y=%p %ssharing write_type=%d\n",
			x, y, is_sharing ? "" : "false-", wr_type);

	// Start testing, record the time
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	// Create and start 2 threads and testing
	pthread_create(&t1, NULL, worker, (void*)x);
	pthread_create(&t2, NULL, worker, (void*)y);

	// Wait for the threads end
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	// End testing
	clock_gettime(CLOCK_MONOTONIC, &end);
	long duration_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
	double duration_ms = duration_ns / 1e6;
	printf("Time: %.2f ms\n", duration_ms);

	// Release resources
	if (wr_type == UC_WRTIE) {
		munmap(map_base, MAP_SIZE);
		close(fd);
	}
	if (data && (munmap(data, data_size) == -1)) FATAL;

	return 0;
}
