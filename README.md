# Processor Topology & Affinity for ocaml

This library allows you to query the processor topology as well as set
the processor affinity for the current process. This library does
*not* depend on ocaml-5 (Multicore), but it can be used within ocaml-5
Domains as expected.

The topology can identify individual threads (smt), cores, sockets as
well as P-cores (Performance) and E-cores (Energy energy efficient
cores) on both AMD64 and Apple's ARM64 (M1, M2 & friends).

## Modules

The library is split into 3 main modules:

### Query

Retrieves a count of threads, cores, sockets.
```
utop # Processor.Query.cpu_count;;
- : int = 8
utop # Processor.Query.core_count;;
- : int = 4
utop # Processor.Query.socket_count;;
- : int = 1
```

### Topology

Build's an actual topology of each CPU, each `Cpu.t` expresses a
logical cpu with a logical id `id`, a thread id `smt`, a core id
`core`, a socket id `socket` and a `kind` which can be `P-core` or
`E-core`, which is only relevant for Intel Alder Lake and Apple's
ARM64 machines.

The topology is built uppon Module load an it's static through the runtime.

```
utop # Processor.Topology.t;;
- : Processor.Cpu.t list =
[{Processor.Cpu.id = 0; kind = Processor.Cpu.P_core; smt = 0; core = 0; socket = 0};
 {Processor.Cpu.id = 1; kind = Processor.Cpu.P_core; smt = 0; core = 1; socket = 0};
 {Processor.Cpu.id = 2; kind = Processor.Cpu.P_core; smt = 0; core = 2; socket = 0};
 {Processor.Cpu.id = 3; kind = Processor.Cpu.P_core; smt = 0; core = 3; socket = 0};
 {Processor.Cpu.id = 4; kind = Processor.Cpu.P_core; smt = 1; core = 0; socket = 0};
 {Processor.Cpu.id = 5; kind = Processor.Cpu.P_core; smt = 1; core = 1; socket = 0};
 {Processor.Cpu.id = 6; kind = Processor.Cpu.P_core; smt = 1; core = 2; socket = 0};
 {Processor.Cpu.id = 7; kind = Processor.Cpu.P_core; smt = 1; core = 3; socket = 0}]
```

### Affinity

Sometimes it may be useful to see "what happens" when you restrict
your application to a set of CPUs, maybe you don't want them to cross
a socket, or maybe you want to see how it behaves without two threads
fighting for its core resources.

The affinity must be set on its own running context, so if you are
using Domains, it must be called individually within each domain.

Say you you want to restrict to running only on the threads of core 0:
```
utop # Processor.Affinity.set_cpus (Processor.Cpu.from_core 0 Processor.Topology.t);;
- : unit = ()
utop # Processor.Affinity.get_cpus ();;
- : Processor.Cpu.t list =
[{Processor.Cpu.id = 0; kind = Processor.Cpu.P_core; smt = 0; core = 0; socket = 0};
 {Processor.Cpu.id = 4; kind = Processor.Cpu.P_core; smt = 1; core = 0; socket = 0}]
```

## ocaml-processor-dump

A simple binary called `ocaml-processor-dump` is provided:
```
$ ocaml-processor-dump
cpu_count: 8
core_count: 4
socket_count: 1
cpus-per-core: 2
cpus-per-socket: 8
cores-per-socket: 4
cpu0: smt=0 core=0 socket=0 kind=P_core
cpu1: smt=0 core=1 socket=0 kind=P_core
cpu2: smt=0 core=2 socket=0 kind=P_core
cpu3: smt=0 core=3 socket=0 kind=P_core
cpu4: smt=1 core=0 socket=0 kind=P_core
cpu5: smt=1 core=1 socket=0 kind=P_core
cpu6: smt=1 core=2 socket=0 kind=P_core
cpu7: smt=1 core=3 socket=0 kind=P_core
```

## Implementation Details

Turns out all of this is harder than it should, there are basically no
portable APIs and even the consensus of what a CPU thread is, is
sketchy between different architectures.

### Linux, FreeBSD

On AMD64 we visit each CPU, by pinning our current context, and then
do the whole CPUID dance manually, the only thing we need from the
system is a working `pthread_setaffinity_np`. `Query` and `Topology`
will be accurate as long as the process doesn't start in an already
restricted affinity.

On anything other than AMD64 we will build a fake topology by using
`Query`, each CPU will be its own core and everyone will be on the
same socket.

Initially I've added support for parsing `/proc/cpuinfo` on Linux for
other architectures, but the format is not standarized, so it isn't
worth it.

### NetBSD, OpenBSD, DragonflyBSD

On these systems `Query` is accurate for `cpu_count`, but
`thread_count` and `socket_count` will be faked, topology will be
faked and affinity is a nop. NetBSD and DragonflyBSD could have
affinity support but I don't want to maintain it. OpenBSD has no
support for it.

### Apple/Darwin

Apple doesn't support affinity/pinning, so in order to retrieve the
actual `apicid` in AMD64 we have to go through the horrible `ioreg`
stuff from Apple, which we do. On Apple ARM64 we also go through
`ioreg` to retrieve the relationship between `E-cores` and `P-cores`.
On Apple, `Query` and `Topology` will always be accurate.

## Future Work

* Windows support, hopefully I work on this when I get a more
comfortable windows environment to develop.
* Cache topology would be welcome as well.
* CPU model/brand, there is some support but I want to make it right before
publishing.
* CI/CD setup.
* SPARC64 and RiscOS support would be welcome.

If you want to work on cache topology, I'll send you beers.
