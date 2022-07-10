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

(* This code is only compiled on amd64, where int is 63bits *)
external cpuid_leaf : int -> int -> (int * int * int * int) = "caml_cpuid_leaf"
external decompose_apic : int -> (int * int * int) = "caml_decompose_apic"

let cpuid code = cpuid_leaf code 0

let register_to_bytes x =
  let bytes = Bytes.create 4 in
  Bytes.set bytes 0 (Int.shift_right (x land 0xff)       0  |> Char.chr);
  Bytes.set bytes 1 (Int.shift_right (x land 0xff00)     8  |> Char.chr);
  Bytes.set bytes 2 (Int.shift_right (x land 0xff0000)   16 |> Char.chr);
  Bytes.set bytes 3 (Int.shift_right (x land 0xff000000) 24 |> Char.chr);
  bytes

let cpu_vendor =
  let vendor_bytes = Bytes.create 12 in
  let _, ebx, ecx, edx = cpuid 0 in
  let ebx_bytes = register_to_bytes ebx in
  let edx_bytes = register_to_bytes edx in
  let ecx_bytes = register_to_bytes ecx in
  Bytes.blit ebx_bytes 0 vendor_bytes 0 4;
  Bytes.blit edx_bytes 0 vendor_bytes 4 4;
  Bytes.blit ecx_bytes 0 vendor_bytes 8 4;
  Bytes.to_string vendor_bytes

let cpu_model =
  let model_bytes = Bytes.create 48 in
  let b0,   b4,  b8, b12 = cpuid 0x80000002 in
  let b16, b20, b24, b28 = cpuid 0x80000003 in
  let b32, b36, b40, b44 = cpuid 0x80000004 in
  let blit b o = Bytes.blit (register_to_bytes b) 0 model_bytes o 4 in
  blit b0   0; blit b4   4; blit b8   8; blit b12 12;
  blit b16 16; blit b20 20; blit b24 24; blit b28 28;
  blit b32 32; blit b36 36; blit b40 40; blit b44 44;
  (* Find the terminating byte *)
  let len = Option.value (Bytes.index_opt model_bytes '\x00') ~default:48 in
  Bytes.sub_string model_bytes 0 len
