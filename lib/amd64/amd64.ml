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

let bytes_of_register x =
  let bytes = Bytes.create 4 in
  Bytes.set bytes 0 (Int.shift_right (x land 0xff)       0  |> Char.chr);
  Bytes.set bytes 1 (Int.shift_right (x land 0xff00)     8  |> Char.chr);
  Bytes.set bytes 2 (Int.shift_right (x land 0xff0000)   16 |> Char.chr);
  Bytes.set bytes 3 (Int.shift_right (x land 0xff000000) 24 |> Char.chr);
  bytes

