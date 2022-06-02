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

#define _GNU_SOURCE

#include <sys/sysinfo.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#include "caml/memory.h"
#include "caml/fail.h"
#include "caml/unixsupport.h"
#include "caml/signals.h"
#include "caml/alloc.h"
#include "caml/custom.h"
#include "caml/bigarray.h"

#ifdef __linux__

#endif

CAMLprim value
caml_num_threads(value vunit)
{
	CAMLparam0();
	int n;

	caml_enter_blocking_section();
	n = get_nprocs_conf();
	caml_leave_blocking_section();

	CAMLreturn (Val_int(n));
}

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
	numcpus = get_nprocs_conf();
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

	caml_enter_blocking_section();
	numcpus = get_nprocs_conf();
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

	/* XXX-Check An empty list is really just an `uninitiliazed` value ? */
	CAMLreturn (cpulist);
}
