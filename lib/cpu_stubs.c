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
#define USE_LINUX_AFFINITY

#undef _GNU_SOURCE
#endif	/* __linux__ */

/*
 *    Darwin - __APPLE__
 */
#ifdef __APPLE__

#define USE_NUM_CPU_SYSCONF
#define USE_NOP_AFFINITY

#endif	/* endif __APPLE__ */

/*
 *    FreeBSD
 */
#ifdef __FreeBSD__

#include <pthread_np.h>		/* Has CPU_ macros */

#define USE_NUM_CPU_SYSCONF
#define USE_LINUX_AFFINITY	/* Nice enough to have compat */

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

#define USE_NUM_CPU_SYSCONF
#define USE_NOP_AFFINITY

#endif	/* endif __NetBSD__ */

/*
 *    DragonFly
 */
#ifdef __DrafonFly__

#define USE_NUM_CPU_SYSCONF
#define USE_NOP_AFFINITY

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

/*
 *    set_affinity() and get_affinity()
 */
#if defined(USE_LINUX_AFFINITY)

CAMLprim value
caml_set_affinity(value cpulist)
{
	CAMLparam1(cpulist);
	CAMLlocal1(cpu);
	cpu_set_t *cpuset;
	int numcpus, error;
	long cpuid;
	size_t sz;

	caml_enter_blocking_section();
	numcpus = num_cpu();
	cpuset = CPU_ALLOC(numcpus);
	caml_leave_blocking_section();
	if (cpuset == NULL)
		uerror("CPU_ALLOC", Nothing);

	sz = CPU_ALLOC_SIZE(numcpus);
	CPU_ZERO_S(sz, cpuset);

	for (cpu = cpulist; cpu != Val_emptylist; cpu = Field(cpu, 1)) {
		cpuid = Long_val(Field(cpu, 0));
		CPU_SET_S(cpuid, sz, cpuset);
	}

	error = pthread_setaffinity_np(pthread_self(), sz, cpuset);
	if (error != 0) {
		errno = error;	/* Yes, errno is not set */
		caml_enter_blocking_section();
		CPU_FREE(cpuset);
		caml_leave_blocking_section();
		uerror("pthread_setaffinity_np", Nothing);
	}

	caml_enter_blocking_section();
	CPU_FREE(cpuset);
	caml_leave_blocking_section();

	CAMLreturn (Val_unit);
}

CAMLprim value
caml_get_affinity(value unit)
{
	cpu_set_t *cpuset;
	int numcpus, error, cpuid;
	size_t sz;
	CAMLparam0();
	CAMLlocal2(cpulist, cpu);

	numcpus = num_cpu();
	caml_enter_blocking_section();
	cpuset = CPU_ALLOC(numcpus);
	caml_leave_blocking_section();
	if (cpuset == NULL)
		uerror("CPU_ALLOC", Nothing);

	sz = CPU_ALLOC_SIZE(numcpus);
	CPU_ZERO_S(sz, cpuset);

	error = pthread_getaffinity_np(pthread_self(), sz, cpuset);
	if (error != 0) {
		errno = error;	/* Yes, errno is not set */
		caml_enter_blocking_section();
		CPU_FREE(cpuset);
		caml_leave_blocking_section();
		uerror("pthread_setaffinity_np", Nothing);
	}

	for (cpuid = numcpus - 1; cpuid >= 0; cpuid--) {
		if (!CPU_ISSET_S(cpuid, sz, cpuset))
			continue;
		cpu = caml_alloc_2(0, Val_int(cpuid), cpulist);
		cpulist = cpu;
	}

	caml_enter_blocking_section();
	CPU_FREE(cpuset);
	caml_leave_blocking_section();

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

	numcpus = num_cpu();
	/* Assume affinity is all, since we can't set or get */
	for (cpuid = numcpus - 1; cpuid >= 0; cpuid--) {
		cpu = caml_alloc_2(0, Val_int(cpuid), cpulist);
		cpulist = cpu;
	}

	CAMLreturn (cpulist);
}

#else  /* USE_*_AFFINITY */
#error Dont know which set_affinity to use :(
#endif	/* USE_*_AFFINITY */
