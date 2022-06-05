# CPU Topology & Affinity for ocaml

This library allows you to build the machine CPU topology and pin the
running process/Domain to a set of CPUs, it works with and without Domains.

The following snippet retrieves the current running affinity; then
sets it to only run on one thread of each core; then 

#### Retrieve current affinity
```
utop # let aff = Cpu.Affinity.get_lcpus ();;
val aff : Cpu.Lcpu.t list =
  [{Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 0; smt = 0; core = 0; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 1; smt = 0; core = 1; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 2; smt = 0; core = 2; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 3; smt = 0; core = 3; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 4; smt = 1; core = 0; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 5; smt = 1; core = 1; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 6; smt = 1; core = 2; socket = 0; cache_alignment = 64};
   {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 7; smt = 1; core = 3; socket = 0; cache_alignment = 64}]
```
#### Set affinity to be one thread of each core and check the result
```
utop # Cpu.Affinity.set_lcpus (Cpu.Lcpu.from_smt 1 aff);;
- : unit = ()
utop # Cpu.Affinity.get_lcpus ();;
- : Cpu.Lcpu.t list =
[{Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 4; smt = 1; core = 0; socket = 0; cache_alignment = 64};
 {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 5; smt = 1; core = 1; socket = 0; cache_alignment = 64};
 {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 6; smt = 1; core = 2; socket = 0; cache_alignment = 64};
 {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 7; smt = 1; core = 3; socket = 0; cache_alignment = 64}]
```
#### Set affinity to a single core (its two threads)
```
utop # Cpu.Affinity.set_lcpus (Cpu.Lcpu.from_core 2 aff);;
- : unit = ()
utop # Cpu.Affinity.get_lcpus ();;
- : Cpu.Lcpu.t list =
[{Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 2; smt = 0; core = 2; socket = 0; cache_alignment = 64};
 {Cpu.Lcpu.model = "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz"; id = 6; smt = 1; core = 2; socket = 0; cache_alignment = 64}]
```
