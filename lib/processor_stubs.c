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

#ifdef __linux__
#define _GNU_SOURCE
#include <sys/sysinfo.h>
#endif

#include <sys/types.h>

#if defined(__APPLE__) || defined(__FreeBSD__) ||  \
    defined(__OpenBSD__) || defined(__NetBSD__) || \
    defined(__NetBSD__) || defined(__DragonFly__)
#include <sys/sysctl.h>
#endif

#ifdef __FreeBSD__
#include <sys/cpuset.h>
typedef cpuset_t cpu_set_t;
#endif

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <pthread_np.h>		/* Has CPU_ macros */
#endif

#include "caml/memory.h"
#include "caml/fail.h"
#include "caml/unixsupport.h"
#include "caml/signals.h"
#include "caml/alloc.h"
#include "caml/custom.h"
#include "caml/bigarray.h"

#if defined(__linux__) || defined(__FreeBSD__) /* Nice enough to have compat */
#define USE_AFFINITY_LINUX

#elif defined(__APPLE__)
#define USE_NOP_AFFINITY
#define USE_SYSCTLBYNAME_32
#define USE_NUM_APPLE

#else
#define USE_NOP_AFFINITY
#endif	/* USE_* */

#define num_cpu()		((int)sysconf(_SC_NPROCESSORS_CONF))
#define num_cpu_online()	((int)sysconf(_SC_NPROCESSORS_ONLN))

/*
 * Ocaml FFI
 */
CAMLprim value
caml_num_cpu(value vunit)
{
	CAMLparam0();

	CAMLreturn (Val_int(num_cpu()));
}

CAMLprim value
caml_num_cpu_online(value vunit)
{
	CAMLparam0();

	CAMLreturn (Val_int(num_cpu_online()));
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
	int error, cpuid;

	CPU_ZERO(&cpuset);

	for (cpu = cpulist; cpu != Val_emptylist; cpu = Field(cpu, 1)) {
		cpuid = Int_val(Field(cpu, 0));
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
	int error, cpuid;
	CAMLparam0();
	CAMLlocal2(cpulist, cpu);

	CPU_ZERO(&cpuset);

	error = pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
	if (error != 0) {
		errno = error;	/* Yes, errno is not set */
		uerror("pthread_getaffinity_np", Nothing);
	}

	cpulist = Val_emptylist;
	for (cpuid = num_cpu() - 1; cpuid >= 0; cpuid--) {
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
	int cpuid;

	/* Assume affinity is all, since we can't set or get */
	cpulist = Val_emptylist;
	for (cpuid = num_cpu() - 1; cpuid >= 0; cpuid--) {
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
