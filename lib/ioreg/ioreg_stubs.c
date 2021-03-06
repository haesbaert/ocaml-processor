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

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Availability.h>
#if (defined __MAC_OS_X_VERSION_MIN_REQUIRED) && (__MAC_OS_X_VERSION_MIN_REQUIRED < 120000)
#define kIOMainPortDefault kIOMasterPortDefault
#endif
#define DT_PLANE "IODeviceTree"

/* #define DEBUG */
#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include "caml/memory.h"
#include "caml/fail.h"
#include "caml/unixsupport.h"
#include "caml/signals.h"
#include "caml/alloc.h"
#include "caml/custom.h"
#include "caml/bigarray.h"

void
debug(const char *fmt, ...)
{
#ifdef DEBUG
	va_list	 ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
#endif
}

#define IOREG_MAXCPU 128

struct index_map {
	uint32_t	index;
	uint32_t	apicid;
};

#if defined(__amd64__)
int
ioreg_to_index_map(struct index_map *map, int *nmap)
{
	io_registry_entry_t	cpus_root;
	io_iterator_t		cpus_iter;
	io_registry_entry_t	cpus_child;
	kern_return_t		kret;
	int			i = 0, nfound = 0;

	cpus_root = IORegistryEntryFromPath(kIOMainPortDefault,
	    DT_PLANE ":/cpus");
	if (cpus_root == 0) {
		debug("IORegistryEntryFromPath");
		return (-1);
	}

	kret = IORegistryEntryGetChildIterator(cpus_root, DT_PLANE, &cpus_iter);
	if (kret != KERN_SUCCESS) {
		IOObjectRelease(cpus_root);
		debug("IORegistryEntryGetChildIterator");
		return (-1);
	}

	while ((cpus_child = IOIteratorNext(cpus_iter)) != 0) {
		CFTypeRef ref;
		int apicid, index;

		if (nfound == *nmap)
			break;

		/* Fetch processor-lapic (apicid) */
		ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE,
		    CFSTR("processor-lapic"), kCFAllocatorDefault, kNilOptions);
		if (!ref) {
			debug("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"processor-lapic\") i=%d (this is ok!)", i);
			continue;
		}
		if (!CFNumberGetValue(ref, kCFNumberIntType, &apicid)) {
			debug("CFNumberGetValue");
			CFRelease(ref);
			continue;
		}
		CFRelease(ref);

		/* Fetch index, not sure they're sequential */
		ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE,
		    CFSTR("processor-index"), kCFAllocatorDefault, kNilOptions);
		if (!ref) {
			debug("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"processor-index\") i=%d (this is ok)", i);
			continue;
		}
		if (!CFNumberGetValue(ref, kCFNumberIntType, &index)) {
			debug("CFNumberGetValue");
			CFRelease(ref);
			continue;
		}
		CFRelease(ref);
		debug("%2d found entry: index=%2d apic=%2d\n", i++, index,
		    apicid);

		map->index = index;
		map->apicid = apicid;
		map++;
		nfound++;
	}
	IOObjectRelease(cpus_iter);
	IOObjectRelease(cpus_root);

	/* Tell how many we filled */
	*nmap = nfound;

	return (0);
}
#elif defined(__arm64__)
int
ioreg_to_index_map(struct index_map *map, int *nmap)
{
	io_registry_entry_t	cpus_root;
	io_iterator_t		cpus_iter;
	io_registry_entry_t	cpus_child;
	kern_return_t		kret;
	int			i = 0, nfound = 0;

	cpus_root = IORegistryEntryFromPath(kIOMainPortDefault,
	    DT_PLANE ":/cpus");
	if (cpus_root == 0) {
		debug("IORegistryEntryFromPath");
		return (-1);
	}

	kret = IORegistryEntryGetChildIterator(cpus_root, DT_PLANE, &cpus_iter);
	if (kret != KERN_SUCCESS) {
		IOObjectRelease(cpus_root);
		debug("IORegistryEntryGetChildIterator");
		return (-1);
	}

	while ((cpus_child = IOIteratorNext(cpus_iter)) != 0) {
		CFTypeRef ref;
		int cpuid, cputype;
		long long lld;
		uint8_t bytes[2];

		if (nfound == *nmap)
			break;

		/* Fetch logical-cpu-id, ok to fail */
		ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE,
		    CFSTR("logical-cpu-id"), kCFAllocatorDefault, kNilOptions);
		if (!ref) {
			debug("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"logical-cpu-id\") i=%d (this is ok!)", i);
			continue;
		}
		if (CFGetTypeID(ref) != CFNumberGetTypeID()) {
			debug("unexpected logical-cpu-id type");
			CFRelease(ref);
			continue;
		}
		if (!CFNumberGetValue(ref, kCFNumberLongLongType, &lld)) {
			debug("CFNumberGetValue");
			CFRelease(ref);
			continue;
		}
		cpuid = (int)lld;
		CFRelease(ref);

		/* Peek into cluster-type to figure if E-core or P-core */
		ref = IORegistryEntrySearchCFProperty(cpus_child,
		    DT_PLANE, CFSTR("cluster-type"), kCFAllocatorDefault,
		    kNilOptions);
		if (!ref) {
			debug("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"cluster-type\") i=%d (this is ok!)", i);
			continue;
		}
		if (CFGetTypeID(ref) != CFDataGetTypeID()) {
			debug("unexpected cluster-type type");
			CFRelease(ref);
			continue;
		}
		if (CFDataGetLength(ref) < 2) {
			debug("cluster-type too short");
			CFRelease(ref);
			continue;
		}
		/* NOTE no error */
		CFDataGetBytes(ref, CFRangeMake(0, 2), bytes);
		if (bytes[1] != 0) {
			debug("unterminated cluster-type");
			CFRelease(ref);
		}
		if (bytes[0] == 'P')
			cputype = 0;
		else if (bytes[0] == 'E')
			cputype = 1;
		else {
			debug("unknown cluster-type[0] %c", (char)bytes[0]);
			CFRelease(ref);
			continue;
		}
		CFRelease(ref);
		debug("%2d found entry: cpuid=%2d cputype=%2d\n", i++, cpuid,
		    cputype);

		map->index = cpuid;
		map->apicid = cputype;
		map++;
		nfound++;
	}
	IOObjectRelease(cpus_iter);
	IOObjectRelease(cpus_root);

	/* Tell how many we filled */
	*nmap = nfound;

	return (0);
}
#else
#error "no __amd64__ or __arm64__"
#endif

CAMLprim value
caml_ioreg_fetch(void)
{
	CAMLparam0();
	CAMLlocal3(list, cell, tuple);
	struct index_map *omap, *map;
	int nmap = IOREG_MAXCPU;

	/* Too big the stack */
	caml_enter_blocking_section();
	omap = calloc(nmap, sizeof(*omap));
	caml_leave_blocking_section();
	if (omap == NULL)
		CAMLreturn (Val_emptylist);

	if (ioreg_to_index_map(omap, &nmap) == -1) {
		free(omap);
		CAMLreturn (Val_emptylist);
	}

	list = Val_emptylist;
	for (map = (omap + nmap - 1); map >= omap; map--) {
		tuple = caml_alloc(2, Tag_cons);
		Store_field(tuple, 0, Val_int(map->index));
		Store_field(tuple, 1, Val_int(map->apicid));
		cell = caml_alloc(2, Tag_cons);
		Store_field(cell, 0, tuple);
		Store_field(cell, 1, list);
		list = cell;
	}
	free(omap);

	CAMLreturn(list);
}
