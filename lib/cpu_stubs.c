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

#if !defined(__linux__) && !defined(__APPLE__) && \
    !defined(__FreeBSD__) && !defined(__OpenBSD__) && \
    !defined(__NetBSD__) && !defined(__DragonFly__)
#error Unsupported OS
#endif

/*
 * ""Portable"" num_cpu()
 */
int	num_cpu(void);
int	num_cpu_online(void);

/*
 *    Linux
 */
#ifdef __linux__

#define _GNU_SOURCE

#include <sys/sysinfo.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#define USE_NUM_CPU_SYSCONF
#define USE_AFFINITY_LINUX

#undef _GNU_SOURCE
#endif	/* __linux__ */

/*
 *    Darwin - __APPLE__
 */
#ifdef __APPLE__

#include <sys/types.h>
#include <sys/sysctl.h>

#define USE_NUM_CPU_SYSCONF
#define USE_NOP_AFFINITY
#define USE_SYSCTLBYNAME_32
#define USE_NUM_CORE_APPLE
#define USE_NUM_SOCKET_APPLE

#endif	/* endif __APPLE__ */

/*
 *    FreeBSD
 */
#ifdef __FreeBSD__

#include <errno.h>
#include <pthread.h>
#include <pthread_np.h>		/* Has CPU_ macros */

#include <sys/cpuset.h>

typedef cpuset_t cpu_set_t;

#define USE_NUM_CPU_SYSCONF
#define USE_AFFINITY_LINUX	/* Nice enough to have compat */

#endif	/* endif __FreeBSD__ */

/*
 *    OpenBSD
 */
#ifdef __OpenBSD__

#define USE_NUM_CPU_SYSCONF
#define USE_NOP_AFFINITY

#endif	/* endif __OpenBSD__ */

/*
 *    NetBSD
 */
#ifdef __NetBSD__

#include <errno.h>
#include <pthread.h>
#include <sched.h>

#define USE_NUM_CPU_SYSCONF
#define USE_AFFINITY_LINUX

/* Mini compat layer for linux affinity */
typedef cpu_set_t cpuset_t;
#define CPU_ALLOC(_n)		cpuset_create()
#define	CPU_FREE(cs)		cpuset_destroy(c)
#define	CPU_ALLOC_SIZE(c)	cpuset_size(c)
#define	CPU_ZERO_S(_s, c)	cpuset_zero(c)
#define	CPU_ISSET_S(i, _s, c)	cpuset_isset(i, c)
#define	CPU_SET_S(i, _s, c)	cpuset_set(i, c)
#define	CPU_CLR_S(i, _s, c)	cpuset_clear(i, c)

#endif	/* endif __NetBSD__ */

/*
 *    DragonFly
 */
#ifdef __DrafonFly__

#include <pthread.h>
#include <pthread_np.h>

#include <sys/cpumask.h>

#define USE_NUM_CPU_SYSCONF
#define USE_AFFINITY_LINUX

#define CPU_ALLOC(_n)		malloc(CPU_SETSIZE)
#define	CPU_FREE(cs)		free(cs)
#define	CPU_ALLOC_SIZE(_c)	CPU_SETSIZE
#define	CPU_ZERO_S(_s, c)	CPU_ZERO(c)
#define	CPU_ISSET_S(i, _s, c)	CPU_ISSET(i, c)
#define	CPU_SET_S(i, _s, c)	CPU_SET(i, c)
#define	CPU_CLR_S(i, _s, c)	CPU_CLR(i, c)

#endif	/* endif __DragonFly__ */

/*
 *    Ocaml includes
 */
#include "caml/memory.h"
#include "caml/fail.h"
#include "caml/unixsupport.h"
#include "caml/signals.h"
#include "caml/alloc.h"
#include "caml/custom.h"
#include "caml/bigarray.h"

/*
 *    num_cpu() and num_cpu_online()
 */
#if defined(USE_NUM_CPU_SYSCONF)

#include <unistd.h>

int
num_cpu(void)
{
	return (int)sysconf(_SC_NPROCESSORS_CONF);
}

int
num_cpu_online(void)
{
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

#elif defined(USE_NUM_CPU_NPROCS)

int
num_cpu(void)
{
	return (get_nprocs_conf());
}

int
num_cpu_online(void)
{
	return (get_nprocs());
}

#elif defined(USE_NUM_CPU_BSD)

int
num_cpu_bsd(int hw)
{
	int mib[] = { CTL_HW, hw };
	int numcpu;
	size_t size = sizeof(numcpu);

	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]),
	    &numcpu, &size, NULL, 0) == -1)
		return (-1);

	return (numcpu);
}

#else
#error Dont know which num_cpu to use :-(
#endif	/* USE_NUM_CPU_* */

#ifdef USE_NUM_CORE_APPLE

int
num_core(void)
{
	int32_t core;
	size_t len = sizeof(core);

	if (sysctlbyname("hw.physicalcpu", &core, &len, NULL, 0) == -1)
		return (-1);

	return ((int)core);
}

#endif

#ifdef USE_NUM_SOCKET_APPLE

int
num_socket(void)
{
	int32_t socket;
	size_t len = sizeof(socket);

	if (sysctlbyname("hw.packages", &socket, &len, NULL, 0) == -1)
		return (-1);

	return ((int)socket);
}

#endif

/*
 * Ocaml FFI
 */
CAMLprim value
caml_num_cpu(value vunit)
{
	CAMLparam0();
	int n;

	caml_enter_blocking_section();
	n = num_cpu();
	caml_leave_blocking_section();

	CAMLreturn (Val_int(n));
}

CAMLprim value
caml_num_cpu_online(value vunit)
{
	CAMLparam0();
	int n;

	caml_enter_blocking_section();
	n = num_cpu_online();
	caml_leave_blocking_section();

	CAMLreturn (Val_int(n));
}

#ifdef USE_SYSCTLBYNAME_32

CAMLprim value
caml_sysctlbyname32(value mib)
{
	CAMLparam1(mib);
	int32_t word;
	size_t len = sizeof(word);

	if (sysctlbyname(String_val(mib), &word, &len, NULL, 0) == -1)
		uerror("sysctlbyname", Nothing);

	CAMLreturn (caml_copy_int32(word));
}

#endif

#ifdef USE_NUM_CORE_APPLE

CAMLprim value /* NOTE remove if we don't use it from C */
caml_num_core(value vunit)
{
	CAMLparam0();
	int n;

	if ((n = num_core()) == -1)
		uerror("num_core", Nothing);

	CAMLreturn (Val_int(n));
}

#endif

#ifdef USE_NUM_SOCKET_APPLE

CAMLprim value /* NOTE remove if we don't use it from C */
caml_num_socket(value vunit)
{
	CAMLparam0();
	int n;

	if ((n = num_socket()) == -1)
		uerror("num_socket", Nothing);

	CAMLreturn (Val_int(n));
}

#endif

/*
 *    set_affinity() and get_affinity()
 */
#if defined(USE_AFFINITY_LINUX)

CAMLprim value
caml_set_affinity(value cpulist)
{
	CAMLparam1(cpulist);
	CAMLlocal1(cpu);
	cpu_set_t cpuset;
	int numcpus, error;
	long cpuid;

	caml_enter_blocking_section();
	numcpus = num_cpu();
	caml_leave_blocking_section();

	CPU_ZERO(&cpuset);

	for (cpu = cpulist; cpu != Val_emptylist; cpu = Field(cpu, 1)) {
		cpuid = Long_val(Field(cpu, 0));
		CPU_SET(cpuid, &cpuset);
	}

	error = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
	if (error != 0) {
		errno = error;	/* Yes, errno is not set */
		uerror("pthread_setaffinity_np", Nothing);
	}

	CAMLreturn (Val_unit);
}

CAMLprim value
caml_get_affinity(value unit)
{
	cpu_set_t cpuset;
	int numcpus, error, cpuid;
	CAMLparam0();
	CAMLlocal2(cpulist, cpu);

	caml_enter_blocking_section();
	numcpus = num_cpu();
	caml_leave_blocking_section();

	CPU_ZERO(&cpuset);

	error = pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
	if (error != 0) {
		errno = error;	/* Yes, errno is not set */
		uerror("pthread_getaffinity_np", Nothing);
	}

	cpulist = Val_emptylist;
	for (cpuid = numcpus - 1; cpuid >= 0; cpuid--) {
		if (!CPU_ISSET(cpuid, &cpuset))
			continue;
		cpu = caml_alloc(2, Tag_cons);
		Store_field(cpu, 0, Val_int(cpuid));
		Store_field(cpu, 1, cpulist);
		cpulist = cpu;
	}

	CAMLreturn (cpulist);
}

#elif defined(USE_NOP_AFFINITY)

CAMLprim value
caml_set_affinity(value cpulist)
{
	CAMLparam1(cpulist);

	CAMLreturn (Val_unit);
}

CAMLprim value
caml_get_affinity(value unit)
{
	CAMLparam0();
	CAMLlocal2(cpulist, cpu);
	int cpuid, numcpus;

	caml_enter_blocking_section();
	numcpus = num_cpu();
	caml_leave_blocking_section();
	/* Assume affinity is all, since we can't set or get */
	cpulist = Val_emptylist;
	for (cpuid = numcpus - 1; cpuid >= 0; cpuid--) {
		cpu = caml_alloc(2, Tag_cons);
		Store_field(cpu, 0, Val_int(cpuid));
		Store_field(cpu, 1, cpulist);

		cpulist = cpu;
	}

	CAMLreturn (cpulist);
}

#else  /* USE_*_AFFINITY */
#error Dont know which set_affinity to use :(
#endif	/* USE_*_AFFINITY */

#ifdef USE_AMD64_TOPOLOGY

#include <string.h>		/* for memcpy(3) */

#define	CPUID(code, eax, ebx, ecx, edx)                         \
	__asm volatile("cpuid"                                  \
	    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)    \
	    : "a" (code))
#define	CPUID_LEAF(code, leaf, eax, ebx, ecx, edx)		\
	__asm volatile("cpuid"                                  \
	    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)    \
	    : "a" (code), "c" (leaf))

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

#endif	/* USE_AMD64_TOPOLOGY */
