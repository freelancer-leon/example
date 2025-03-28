// gcc -Wall -g -ftest-coverage -fprofile-arcs -o bitset bitset.c

#include <stdio.h>

#define BITS_PER_LONG		(64)
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)

#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

void __bitmap_set(unsigned long *map, unsigned int start, int len)
{
    unsigned long *p = map + BIT_WORD(start);
    const unsigned int size = start + len;
    int bits_to_set = BITS_PER_LONG - (start % BITS_PER_LONG);
    unsigned long mask_to_set = BITMAP_FIRST_WORD_MASK(start);

    while (len - bits_to_set >= 0) {
        *p |= mask_to_set;
        len -= bits_to_set;
        bits_to_set = BITS_PER_LONG;
        mask_to_set = ~0UL;
        p++;
    }
    if (len) {
        mask_to_set &= BITMAP_LAST_WORD_MASK(size);
        *p |= mask_to_set;
    }
}

void __bitmap_clear(unsigned long *map, unsigned int start, int len)
{
    unsigned long *p = map + BIT_WORD(start);
    const unsigned int size = start + len;
    int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
    unsigned long mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

    while (len - bits_to_clear >= 0) {
        *p &= ~mask_to_clear;
        len -= bits_to_clear;
        bits_to_clear = BITS_PER_LONG;
        mask_to_clear = ~0UL;
        p++;
    }
    if (len) {
        mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
        *p &= ~mask_to_clear;
    }
}

int check_range(unsigned long map_size, unsigned int start, int len)
{
	if (start + len > map_size - 1) {
		printf("%d + %d > %lu, What do you want me do?\n", start, len, map_size);
		return 0;
	}
	return 1;
}

int bitmap_set(unsigned long *map, unsigned long map_size, unsigned int start, int len)
{
	int err = 0;

	if (check_range(map_size, start, len))
		__bitmap_set(map, start, len);
	else
		err = -1;

	return err;
}

int bitmap_clear(unsigned long *map, unsigned long map_size, unsigned int start, int len)
{
	int err = 0;

	if (check_range(map_size, start, len))
		__bitmap_clear(map, start, len);
	else
		err = -1;

	return err;
}

void print_map(unsigned long *map, int n)
{
	int i, j;
	unsigned char *c;

	for (i = 0; i < n; i++) {
		printf("map[%d]: 7: ", i);

		c = (unsigned char *)&map[i];
		//little endian
		for (j = sizeof(*map) - 1; j > 0; j--)
			printf("%02x ", c[j]);

		printf(":0\n");
	}
}

void print_bitmap(unsigned long *map, int n)
{
	printf("      Hi                Lo\n");
	for (int i = 0; i < n; i++)
		printf("map[%d]: %016lx\n", i, map[i]);
}

int main()
{
	unsigned long map[8] = {0};
	unsigned long map_size = sizeof(map) * 8; // Bit per char is 8

	if (!bitmap_set(map, map_size, 311, 68))
		print_bitmap(map, ARRAY_SIZE(map));

	if (!bitmap_set(map, map_size, 518, 88))
		print_bitmap(map, ARRAY_SIZE(map));

	if (!bitmap_set(map, map_size, 78, 168))
		print_bitmap(map, ARRAY_SIZE(map));

	if (!bitmap_clear(map, map_size, 121, 18))
		print_bitmap(map, ARRAY_SIZE(map));

	return 0;
}
