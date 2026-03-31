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

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601	/* Windows 7 */
#endif
#include <windows.h>

#else /* !_WIN32 */

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

#endif /* !_WIN32 */

#include "caml/memory.h"
#include "caml/fail.h"
#ifndef _WIN32
#include "caml/unixsupport.h"
#endif
#include "caml/signals.h"
#include "caml/alloc.h"
#include "caml/custom.h"
#include "caml/bigarray.h"

#ifdef _WIN32
#define USE_WIN32_AFFINITY
#define num_cpu()		((int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS))
#define num_cpu_online()	((int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS))

#elif defined(__linux__) || defined(__FreeBSD__) /* Nice enough to have compat */
#define USE_AFFINITY_LINUX
#define num_cpu()		((int)sysconf(_SC_NPROCESSORS_CONF))
#define num_cpu_online()	((int)sysconf(_SC_NPROCESSORS_ONLN))

#elif defined(__APPLE__)
#define USE_NOP_AFFINITY
#define USE_SYSCTLBYNAME_32
#define USE_NUM_APPLE
#define num_cpu()		((int)sysconf(_SC_NPROCESSORS_CONF))
#define num_cpu_online()	((int)sysconf(_SC_NPROCESSORS_ONLN))

#else
#define USE_NOP_AFFINITY
#define num_cpu()		((int)sysconf(_SC_NPROCESSORS_CONF))
#define num_cpu_online()	((int)sysconf(_SC_NPROCESSORS_ONLN))

#endif	/* USE_* */

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

#elif defined(USE_WIN32_AFFINITY)

CAMLprim value
caml_set_affinity(value cpulist)
{
	CAMLparam1(cpulist);
	CAMLlocal1(cpu);
	DWORD_PTR mask = 0;

	for (cpu = cpulist; cpu != Val_emptylist; cpu = Field(cpu, 1)) {
		int cpuid = Int_val(Field(cpu, 0));
		mask |= ((DWORD_PTR)1 << cpuid);
	}

	if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0)
		caml_failwith("SetThreadAffinityMask");

	CAMLreturn (Val_unit);
}

CAMLprim value
caml_get_affinity(value unit)
{
	DWORD_PTR proc_mask, sys_mask;
	int cpuid;
	CAMLparam0();
	CAMLlocal2(cpulist, cpu);

	if (!GetProcessAffinityMask(GetCurrentProcess(),
	    &proc_mask, &sys_mask))
		caml_failwith("GetProcessAffinityMask");

	cpulist = Val_emptylist;
	for (cpuid = num_cpu() - 1; cpuid >= 0; cpuid--) {
		if (!(proc_mask & ((DWORD_PTR)1 << cpuid)))
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

/*
 *    Windows topology via GetLogicalProcessorInformationEx
 */
#ifdef _WIN32

struct win_cpu_entry {
	int id;
	int socket;
	int core;
	int smt;
	int efficiency;
	int valid;
};

static int
win_group_base(WORD group)
{
	int base = 0;
	WORD g;

	for (g = 0; g < group; g++)
		base += (int)GetActiveProcessorCount(g);

	return (base);
}

CAMLprim value
caml_windows_topology(value vunit)
{
	CAMLparam0();
	CAMLlocal3(result, tuple, cons);
	DWORD len = 0;
	BYTE *buf;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info;
	DWORD offset;
	int total_cpus, i;
	struct win_cpu_entry *cpus;
	int core_id, socket_id, max_eff;

	total_cpus = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
	if (total_cpus <= 0)
		caml_failwith("GetActiveProcessorCount");

	cpus = (struct win_cpu_entry *)calloc(total_cpus, sizeof(*cpus));
	if (cpus == NULL)
		caml_raise_out_of_memory();

	/* Get required buffer size */
	GetLogicalProcessorInformationEx(RelationAll, NULL, &len);
	buf = (BYTE *)malloc(len);
	if (buf == NULL) {
		free(cpus);
		caml_raise_out_of_memory();
	}

	if (!GetLogicalProcessorInformationEx(RelationAll,
	    (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf, &len)) {
		free(buf);
		free(cpus);
		caml_failwith("GetLogicalProcessorInformationEx");
	}

	/* Pass 1: assign socket IDs from package records */
	socket_id = 0;
	for (offset = 0; offset < len; ) {
		WORD g;
		info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf + offset);
		if (info->Relationship == RelationProcessorPackage) {
			for (g = 0; g < info->Processor.GroupCount; g++) {
				KAFFINITY mask = info->Processor.GroupMask[g].Mask;
				int base = win_group_base(info->Processor.GroupMask[g].Group);
				int bit = 0;
				while (mask) {
					if (mask & 1) {
						int gid = base + bit;
						if (gid < total_cpus)
							cpus[gid].socket = socket_id;
					}
					mask >>= 1;
					bit++;
				}
			}
			socket_id++;
		}
		offset += info->Size;
	}

	/* Pass 2: assign core IDs, SMT index, and efficiency class */
	core_id = 0;
	max_eff = 0;
	for (offset = 0; offset < len; ) {
		WORD g;
		info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf + offset);
		if (info->Relationship == RelationProcessorCore) {
			int smt_idx = 0;
			int eff = info->Processor.EfficiencyClass;
			if (eff > max_eff)
				max_eff = eff;
			for (g = 0; g < info->Processor.GroupCount; g++) {
				KAFFINITY mask = info->Processor.GroupMask[g].Mask;
				int base = win_group_base(info->Processor.GroupMask[g].Group);
				int bit = 0;
				while (mask) {
					if (mask & 1) {
						int gid = base + bit;
						if (gid < total_cpus) {
							cpus[gid].id = gid;
							cpus[gid].core = core_id;
							cpus[gid].smt = smt_idx;
							cpus[gid].efficiency = eff;
							cpus[gid].valid = 1;
						}
						smt_idx++;
					}
					mask >>= 1;
					bit++;
				}
			}
			core_id++;
		}
		offset += info->Size;
	}

	free(buf);

	/* Build OCaml list in ascending ID order (cons from the back) */
	result = Val_emptylist;
	for (i = total_cpus - 1; i >= 0; i--) {
		int is_ecore;
		if (!cpus[i].valid)
			continue;
		is_ecore = (max_eff > 0 && cpus[i].efficiency < max_eff) ? 1 : 0;

		tuple = caml_alloc(5, 0);
		Store_field(tuple, 0, Val_int(cpus[i].id));
		Store_field(tuple, 1, Val_int(is_ecore));
		Store_field(tuple, 2, Val_int(cpus[i].smt));
		Store_field(tuple, 3, Val_int(cpus[i].core));
		Store_field(tuple, 4, Val_int(cpus[i].socket));

		cons = caml_alloc(2, Tag_cons);
		Store_field(cons, 0, tuple);
		Store_field(cons, 1, result);
		result = cons;
	}

	free(cpus);
	CAMLreturn(result);
}

#endif /* _WIN32 */
