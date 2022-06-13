#include <stdio.h>
#include <err.h>

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Availability.h>

#if (defined __MAC_OS_X_VERSION_MIN_REQUIRED) && (__MAC_OS_X_VERSION_MIN_REQUIRED < 120000)
#define kIOMainPortDefault kIOMasterPortDefault
#endif

#define DT_PLANE "IODeviceTree"

int
main(void)
{
	io_registry_entry_t cpus_root;
	io_iterator_t cpus_iter;
	io_registry_entry_t cpus_child;
	kern_return_t kret;
	int i = 0;

	cpus_root = IORegistryEntryFromPath(kIOMainPortDefault,
	    DT_PLANE ":/cpus");
	if (cpus_root == 0)
		errx(1, "IORegistryEntryFromPath");

	kret = IORegistryEntryGetChildIterator(cpus_root, DT_PLANE, &cpus_iter);
	if (kret != KERN_SUCCESS) {
		IOObjectRelease(cpus_root);
		errx(1, "IORegistryEntryGetChildIterator");
	}

	while ((cpus_child = IOIteratorNext(cpus_iter)) != 0) {
		CFTypeRef ref;
		int apicid, index;

		/* ARM64! */
		/* ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE, */
		/*     CFSTR("logical-cpu-id"), kCFAllocatorDefault, kNilOptions); */

		/* Fetch processor-lapic (apicid) */
		ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE,
		    CFSTR("processor-lapic"), kCFAllocatorDefault, kNilOptions);
		if (!ref) {
			warnx("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"processor-lapic\") i=%d", i);
			continue;
		}
		if (!CFNumberGetValue(ref, kCFNumberIntType, &apicid)) {
			warnx("CFNumberGetValue");
			continue;
		}
		CFRelease(ref);

		/* Fetch index, not sure they're sequential */
		ref = IORegistryEntrySearchCFProperty(cpus_child, DT_PLANE,
		    CFSTR("processor-index"), kCFAllocatorDefault, kNilOptions);
		if (!ref) {
			warnx("IORegistryEntrySearchCFProperty"
			    "CFSTR(\"processor-index\") i=%d", i);
			continue;
		}
		if (!CFNumberGetValue(ref, kCFNumberIntType, &index)) {
			warnx("CFNumberGetValue");
			continue;
		}
		CFRelease(ref);

		printf("%2d found entry: index=%2d apic=%2d\n", i++, index, apicid);
	}

	  
}
