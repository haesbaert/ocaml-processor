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

external get_ids: unit -> int list = "caml_get_affinity"
external set_ids: int list -> unit = "caml_set_affinity"

let t =
  (* check where we can run *)
  let vendor_bytes = Bytes.create 12 in
  let _, ebx, ecx, edx = Amd64.cpuid 0 in
  let ebx_bytes = Amd64.bytes_of_register ebx in
  let ecx_bytes = Amd64.bytes_of_register ecx in
  let edx_bytes = Amd64.bytes_of_register edx in
  Bytes.blit ebx_bytes 0 vendor_bytes 0 4;
  Bytes.blit edx_bytes 0 vendor_bytes 4 4;
  Bytes.blit ecx_bytes 0 vendor_bytes 8 4;
  let vendor = Bytes.to_string vendor_bytes in
  let oldset = get_ids () in
  let topology = List.map
      (fun (id) ->
         set_ids [id];          (* pin ourselves to one cpu *)
         let _, ebx, _, _ = Amd64.cpuid 1 in
         let apicid = Int.shift_right ebx 24 in (* read this cpu apicid *)
         let smt, core, socket = Amd64.decompose_apic apicid in
         let _, _, _, edx = Amd64.cpuid 7 in
         let kind =
           if vendor = "GenuineIntel" then (* only intel has hybrids *)
             let hybrid = (edx land (Int.shift_left 1 15)) <> 0 in
             if not hybrid then
               Cpu.P_core
             else
               let eax, _, _, _ = Amd64.cpuid 0x1A in
               match (Int.shift_right_logical eax 24) with
               | 0x20 -> Cpu.E_core
               | 0x40 -> Cpu.P_core
               | _    -> Cpu.P_core (* best guess *)
           else                           (* AMD *)
             Cpu.P_core
         in
         Cpu.make ~id ~kind ~smt ~core ~socket)
      oldset
  in
  set_ids oldset;               (* restore old set so we can run anywhere *)
  topology
