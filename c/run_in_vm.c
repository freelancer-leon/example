#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define DEFAULT_SLEEP (60)
#define DEFAULT_SIZE (4096)

#define _mm_clflushopt(addr) \
  asm volatile(".byte 0x66; clflush %0" : \
      "+m" (*(volatile char *)(addr)));

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

void malloc_free(unsigned int sec, size_t size)
{
  void *ptr = malloc(size);

  assert(ptr != NULL);
  printf("malloc(%ld): GVA: %p GPA: %p\n", size, ptr, (void *)vtop((unsigned long long)ptr));

  // Step1
  printf("sleep %d sec for error injection...\n", sec);
  sleep(sec);

  // Step2
  memset(ptr, 0xab, size);
  _mm_clflushopt(ptr);

  // Step3
  useconds_t usec = 10000;
  printf("sleep %d usec before reading malloced buffer...\n", usec);
  usleep(usec);

  // Step4 - consume poison
  sec = *(unsigned int *)ptr;

  printf("!!! should never reach here, expected to be killed by recovery code :%p=%x\n", ptr, sec);
  free(ptr);
}

int main(int argc, char **argv)
{
  unsigned int sec = DEFAULT_SLEEP;
  size_t size = DEFAULT_SIZE;

  if (argc > 1)
    sec = atol(argv[1]);
  if (argc > 2)
    size = atol(argv[2]);

  malloc_free(sec, size);

  return 0;
}
