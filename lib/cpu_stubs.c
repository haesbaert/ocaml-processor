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
