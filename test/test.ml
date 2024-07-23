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
  assert (Query.cpu_count > 0);
  assert (Query.core_count > 0);
  assert (Query.socket_count > 0);
  assert (List.length Topology.t = Query.cpu_count);
  assert (List.length (Affinity.get_ids ()) = List.length (Affinity.get_cpus ()));
  assert (List.length (Affinity.get_ids ()) = List.length Topology.t);
  assert (List.length (Affinity.get_ids ()) = Query.cpu_count);
  (* nop call, just to make sure we don't crash *)
  Affinity.(set_ids (get_ids ()));
  Affinity.(set_cpus (get_cpus ()));
  (* Make sure ids are monotonically increasing *)
  let _last_id : int =
    List.fold_left
      (fun last_id cpu ->
        assert (last_id = pred cpu.Cpu.id);
        succ last_id )
      (-1) Topology.t
  in
  let _last_id : int =
    List.fold_left
      (fun last_id id ->
        assert (last_id = pred id);
        succ last_id )
      (-1) (Affinity.get_ids ())
  in
  ()
