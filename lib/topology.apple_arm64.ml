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

external num_cpu : unit -> int = "caml_num_cpu"

let t =
  match Processor_apple_ioreg.fetch () with
  | [] ->
    (* ioreg may return nothing in VMs, fall back to fake topology *)
    let num_cpu = num_cpu () in
    let rec loop l i =
      if i = -1 then l
      else
        let cpu = Cpu.(make ~id:i ~kind:P_core ~smt:0 ~core:i ~socket:0) in
        loop (cpu :: l) (pred i)
    in
    loop [] (pred num_cpu)
  | entries ->
    let id = ref (-1) in
    List.map
      (fun (core, ecore) ->
        id := succ !id;
        let kind = if ecore = 1 then Cpu.E_core else Cpu.P_core in
        Cpu.make ~id:!id ~kind ~smt:0 ~core ~socket:0 )
      entries
