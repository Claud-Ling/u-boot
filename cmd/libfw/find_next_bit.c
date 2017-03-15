/* find_next_bit.c: fallback find next bit implementation
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define BITS_PER_BYTE		(8)
#define BITS_PER_INT		(32)

#ifdef BITS_PER_LONG
#undef BITS_PER_LONG
#endif
#define BITS_PER_LONG		(sizeof(long) * BITS_PER_BYTE)

#define BIT(nr)			(1UL << (nr))
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)

#define BIT_INT_MASK(nr)	(1UL<< ((nr) % BITS_PER_INT))
#define BIT_INT(nr)		((nr)/ BITS_PER_INT)

#define BITOP_WORD(nr)		((nr) / BITS_PER_LONG)

#define __ffs(x) (__builtin_ffs(x) - 1)
#define ffz(x)  __ffs(~(x))

/*
 * Find the first set bit in a memory region.
 */
unsigned long find_first_bit(const unsigned long *addr, unsigned long size)
{
	const unsigned int *p = (const unsigned int *)addr;
	unsigned long result = 0;
	unsigned int tmp;

	while (size & ~(BITS_PER_INT-1)) {
		if ((tmp = *(p++)))
			goto found;
		result += BITS_PER_INT;
		size -= BITS_PER_INT;
	}
	if (!size)
		return result;

	tmp = (*p) & (~0UL >> (BITS_PER_INT - size));
	if (tmp == 0UL)		/* Are any bits set? */
		return result + size;	/* Nope. */
found:
	return result + __ffs(tmp);
}

void local_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned int mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);
	*p  |= mask;
}

void local_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned int mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);

	*p &= ~mask;
}

void local_change_bit(int nr, volatile unsigned long *addr)
{
	unsigned int mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);

	*p ^= mask;
}

int local_test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned int mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);
	unsigned int old;

	old = *p;
	*p = old | mask;

	return (old & mask) != 0;
}

int local_test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned int  mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);
	unsigned int old;

	old = *p;
	*p = old & ~mask;

	return (old & mask) != 0;
}

int local_test_and_change_bit(int nr, volatile unsigned long *addr)
{
	unsigned int mask = BIT_INT_MASK(nr);
	unsigned int *p = ((unsigned int *)addr) + BIT_INT(nr);
	unsigned int old;

	old = *p;
	*p = old ^ mask;

	return (old & mask) != 0;
}

int local_test_bit(int nr, const volatile unsigned long *addr)
{
	const volatile unsigned int *data = (const volatile unsigned int *)addr;
	return 1UL & (data[BIT_INT(nr)] >> (nr & (BITS_PER_INT-1)));
}

