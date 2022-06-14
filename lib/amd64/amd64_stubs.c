/*
 * Copyright (c) 2022 Christiano F. Haesbaert <haesbaert@haesbaert.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __amd64__
#error Not amd64
#endif

#include <string.h>		/* for memcpy(3) */

#include "caml/memory.h"
#include "caml/fail.h"
#include "caml/alloc.h"

#define	CPUID(code, eax, ebx, ecx, edx)                         \
	__asm volatile("cpuid"                                  \
	    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)    \
	    : "a" (code))
#define	CPUID_LEAF(code, leaf, eax, ebx, ecx, edx)		\
	__asm volatile("cpuid"                                  \
	    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)    \
	    : "a" (code), "c" (leaf))

CAMLprim value
caml_cpuid_leaf(value code, value leaf)
{
	CAMLparam2(code, leaf);
	CAMLlocal1(ret);
	uint32_t eax, ebx, ecx, edx;

	CPUID_LEAF(Long_val(code), Long_val(leaf),
	    eax, ebx, ecx, edx);

	ret = caml_alloc(4, 0);
	Store_field(ret, 0, Val_long(eax));
	Store_field(ret, 1, Val_long(ebx));
	Store_field(ret, 2, Val_long(edx));
	Store_field(ret, 3, Val_long(ecx));

	CAMLreturn(ret);
}

/*
 * Base 2 logarithm of an int. returns 0 for 0 (yeye, I know).
 */
int
ilog2(unsigned int i)
{
	int ret = 0;

	while (i >>= 1)
		ret++;

	return (ret);
}

int
mask_width(unsigned int x)
{
	int bit;
	int mask;
	int powerof2;

	powerof2 = ((x - 1) & x) == 0;
	mask = (x << (1 - powerof2)) - 1;

	/* fls */
	if (mask == 0)
		return (0);
	for (bit = 1; mask != 1; bit++)
		mask = (unsigned int)mask >> 1;

	return (bit);
}

int
decompose_apicid(uint32_t apicid, int *smt, int *core, int *socket)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t cpuid_level, max_apicid = 0, max_coreid = 0;
	uint32_t smt_bits = 0, core_bits, pkg_bits = 0;
	uint32_t smt_mask = 0, core_mask, pkg_mask = 0;
	char cpu_vendor[13];

	/* We need at least apicid at CPUID 1 */
	CPUID(0, eax, ebx, ecx, edx);
	cpuid_level = eax;
	if (cpuid_level < 1)
		return (-1);

	memcpy(cpu_vendor, &ebx, 4);
	memcpy(cpu_vendor + 4, &edx, 4);
	memcpy(cpu_vendor + 8, &ecx, 4);
	cpu_vendor[sizeof(cpu_vendor) - 1] = 0;

	if (strcmp(cpu_vendor, "AuthenticAMD") == 0) {
		uint32_t nthreads = 1; /* per core */
		uint32_t thread_id; /* within a package */
		uint32_t pnfeatset;

		/* We need at least apicid at CPUID 0x80000008 */
		CPUID(0x80000000, eax, ebx, ecx, edx);
		pnfeatset = eax;
		if (pnfeatset < 0x80000008)
			return (-1);

		CPUID(0x80000008, eax, ebx, ecx, edx);
		core_bits = (ecx >> 12) & 0xf;

		if (pnfeatset >= 0x8000001e) {
			CPUID(0x8000001e, eax, ebx, ecx, edx);
			nthreads = ((ebx >> 8) & 0xf) + 1;
		}

		/* Shift the core_bits off to get at the pkg bits */
		*socket = apicid >> core_bits;

		/* Get rid of the package bits */
		core_mask = (1U << core_bits) - 1;
		thread_id = apicid & core_mask;

		/* Cut logical thread_id into core id, and smt id in a core */
		*core = thread_id / nthreads;
		*smt = thread_id % nthreads;
	} else if (strcmp(cpu_vendor, "GenuineIntel") == 0) {
		/* We only support leaf 1/4 detection */
		if (cpuid_level < 4)
			return (-1);
		/* Get max_apicid */
		CPUID(1, eax, ebx, ecx, edx);
		max_apicid = (ebx >> 16) & 0xff;
		/* Get max_coreid */
		CPUID_LEAF(4, 0, eax, ebx, ecx, edx);
		max_coreid = ((eax >> 26) & 0x3f) + 1;
		/* SMT */
		smt_bits = mask_width(max_apicid / max_coreid);
		smt_mask = (1U << smt_bits) - 1;
		/* Core */
		core_bits = ilog2(max_coreid);
		core_mask = (1U << (core_bits + smt_bits)) - 1;
		core_mask ^= smt_mask;
		/* Pkg */
		pkg_bits = core_bits + smt_bits;
		pkg_mask = ~0U << core_bits;

		*smt	= apicid & smt_mask;
		*core	= (apicid & core_mask) >> smt_bits;
		*socket = (apicid & pkg_mask) >> pkg_bits;
	} else
		return (-1);

#ifdef DEBUG
	printf("smt %u, core %u, pkg %u "
	    "(apicid 0x%x, max_apicid 0x%x, max_coreid 0x%x, smt_bits 0x%x, smt_mask 0x%x, "
	    "core_bits 0x%x, core_mask 0x%x, pkg_bits 0x%x, pkg_mask 0x%x)\n",
	    *smt, *core, *socket, apicid, max_apicid, max_coreid, smt_bits, smt_mask, core_bits,
	    core_mask, pkg_bits, pkg_mask);
#endif

	return (0);
}

CAMLprim value
caml_decompose_apic(value apicid)
{
	CAMLparam1(apicid);
	CAMLlocal1(tuple);
	int smt, core, package;

	if (decompose_apicid(Long_val(apicid), &smt, &core, &package) == -1)
		caml_invalid_argument("caml_decompose_apic: "
		    "bad apicid and/or cpu_vendor");

	tuple = caml_alloc(3, Tag_cons);
	Store_field(tuple, 0, Val_long(smt));
	Store_field(tuple, 1, Val_long(core));
	Store_field(tuple, 2, Val_long(package));

	CAMLreturn(tuple);
}
