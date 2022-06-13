(*
 * Copyright (c) 2022 Christiano Haesbaert <haesbaert@haesbaert.org>
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
 *)

open Cpu

let _ =
  Printf.printf "We haz %d thread(s) in %d core(s) within %d socket(s)\n%!"
    (Query.num_lcpu ()) (Query.num_core ()) (Query.num_socket ());
  Printf.printf "Pinning us to 0-1\n%!";
  Affinity.set_ids [0; 1];
  List.iter (fun cpuid -> Printf.printf "Seen cpu %d\n%!" cpuid) (Affinity.get_ids ());
  let topo = Topology.get () in
  Printf.printf "topology:\n%!";
  List.iter Lcpu.dump topo;
  Printf.printf "Pinning only to one thread of each core (smt=0):\n%!";
  Affinity.set_lcpus (Lcpu.from_smt 0 topo);
  List.iter Lcpu.dump (Affinity.get_lcpus ())
