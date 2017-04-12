
#include <common.h>
#include <command.h>
#include <fdt_support.h>
#include <libfdt.h>

#define MEMORY_REGION_MAX 16

/**
 *	memparse - parse a string with mem suffixes into a number
 *	@ptr: Where parse begins
 *	@retptr: (output) Optional pointer to next char after parse completes
 *
 *	Parses a string into a number.  The number stored at @ptr is
 *	potentially suffixed with K, M, G.
 */

static unsigned long long memparse(const char *ptr, char **retptr)
{
	char *endptr;	/* local pointer to end of parsed string */

	unsigned long long ret = simple_strtoull(ptr, &endptr, 0);

	switch (*endptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

/*
 * fdt_pack_reg - pack address and size array into the "reg"-suitable stream
 */
static int fdt_pack_reg(const void *fdt, void *buf, u64 *address, u64 *size,
			int n)
{
	int i;
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	for (i = 0; i < n; i++) {
		if (address_cells == 2)
			*(fdt64_t *)p = cpu_to_fdt64(address[i]);
		else
			*(fdt32_t *)p = cpu_to_fdt32(address[i]);
		p += 4 * address_cells;

		if (size_cells == 2)
			*(fdt64_t *)p = cpu_to_fdt64(size[i]);
		else
			*(fdt32_t *)p = cpu_to_fdt32(size[i]);
		p += 4 * size_cells;
	}

	return p - (char *)buf;
}

static int fdt_fixup_memory_nodes(void *blob, u64 start[], u64 size[], int regions)
{
	int err, nodeoffset;
	int len;
	u8 tmp[MEMORY_REGION_MAX * 16]; /* Up to 64-bit address + 64-bit size */

	if (regions > MEMORY_REGION_MAX) {
		printf("%s: num regions %d exceeds hardcoded limit %d."
			" Recompile with higher MEMORY_REGION_MAX?\n",
			__FUNCTION__, regions, MEMORY_REGION_MAX);
		return -1;
	}

	err = fdt_check_header(blob);
	if (err < 0) {
		printf("%s: %s\n", __FUNCTION__, fdt_strerror(err));
		return err;
	}

	/* find or create "/memory" node. */
	nodeoffset = fdt_find_or_add_subnode(blob, 0, "memory");
	if (nodeoffset < 0)
			return nodeoffset;

	err = fdt_setprop(blob, nodeoffset, "device_type", "memory",
			sizeof("memory"));
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n", "device_type",
				fdt_strerror(err));
		return err;
	}

	if (!regions)
		return 0;

	len = fdt_pack_reg(blob, tmp, start, size, regions);

	err = fdt_setprop(blob, nodeoffset, "reg", tmp, len);
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n",
				"reg", fdt_strerror(err));
		return err;
	}
	return 0;
}

static void fixup_kernel_memory_layout(void *blob, bd_t *bd)
{
	int num = 0;
	u64 region_start[MEMORY_REGION_MAX];
	u64 region_size[MEMORY_REGION_MAX];
	char *final_bootargs;
	char *s;

	final_bootargs = strdup(getenv("bootargs"));
	if (final_bootargs == NULL)
		return;

	/*
	 * Pick out the memory size.  We look for mem=size@start,
	 * where start and size are "size[KkMm]@start[KkMm]"
	 */
	for ( num = 0, s = final_bootargs; s != NULL; ) {
		char *p;
		char *endp;

		if((p = strstr(s, " mem=")) == NULL) {
			/* no memory parameters found */
			break;
		}

		/*
		 * translate the kernel parameter "mem=" to uppercase, otherwise aarch64 kernel
		 * will be running with a limited range of memory as system RAM,
		 * see arch/arm64/mm/init.c, early_mem() for the detail info.
		 *
		 * no impact on aarch32 kernel that boot with dtb.
		 */
		strncpy(p, " MEM=", 5);

		/* skip over 'mem=' */
		p += 5;

		region_size[num] = memparse(p, &endp);
		if ( *endp == '@') {
			region_start[num] = memparse(endp + 1, &endp);
		} else {
			region_start[num] = bd->bi_dram[0].start;
		}

		/* search for next 'mem=' */
		s = endp;
		num++;
	}

	if (num > 0) {
		/* fixup memory node */
		fdt_fixup_memory_nodes(blob, region_start, region_size, num);

		/* fixup bootargs property in chosen node */
		setenv("bootargs", final_bootargs);
		fdt_chosen(blob);
	}

	return;
}

/*
 * Board code has addition modification that it wants to make
 * to the flat device tree before handing it off to the kernel
 */
int ft_board_setup(void *blob, bd_t *bd)
{
	fixup_kernel_memory_layout(blob, bd);

	return 0;
}
