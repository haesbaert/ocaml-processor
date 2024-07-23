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

open Processor

(* Testing is very limited since we can't make many assumptions of
   where we're running *)
let () =
  Format.eprintf "Testing CPU count@\n";
  assert (Query.cpu_count > 0);
  Format.eprintf "Testing core count@\n";
  assert (Query.core_count > 0);
  Format.eprintf "Testing socket count@\n";
  assert (Query.socket_count > 0);
  Format.eprintf "Testing CPU count equals topology@\n";
  assert (List.length Topology.t = Query.cpu_count);
  Format.eprintf "Testing CPUs cardinal equals Affinity.get_ids@\n";
  assert (List.length (Affinity.get_ids ()) = List.length (Affinity.get_cpus ()));
  Format.eprintf "Testing topology equals Affinity.get_ids@\n";
  assert (List.length (Affinity.get_ids ()) = List.length Topology.t);
  Format.eprintf "Testing CPU count equals Affinity.get_ids@\n";
  assert (List.length (Affinity.get_ids ()) = Query.cpu_count);
  (* nop call, just to make sure we don't crash *)
  Format.eprintf "Testing set_ids@\n";
  Affinity.(set_ids (get_ids ()));
  Format.eprintf "Testing set_cpus@\n";
  Affinity.(set_cpus (get_cpus ()));
  (* Make sure ids are monotonically increasing *)
  Format.eprintf "Testing monotonicity of topology@\n";
  let _last_id : int =
    List.fold_left
      (fun last_id cpu ->
        assert (last_id = pred cpu.Cpu.id);
        succ last_id )
      (-1) Topology.t
  in
  Format.eprintf "Testing monotonicity of get_ids@\n";
  let _last_id : int =
    List.fold_left
      (fun last_id id ->
        assert (last_id = pred id);
        succ last_id )
      (-1) (Affinity.get_ids ())
  in
  ()
