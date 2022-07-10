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

val cpuid_leaf : int -> int -> (int * int * int * int)
(** [cpuid_leaf code leaf] is the CPUID instruction with input
    [code](eax) and leaf(ecx), return is the tuple
    (eax * ebx * ecx * edx) *)

val cpuid : int -> (int * int * int * int)
(** [cpuid code] is [cpuid_leaf code 0] *)

val register_to_bytes : int -> bytes
(** [register_to_bytes register] is the 4 byte representation of
    [register] in bytes *)

val decompose_apic : int -> (int * int * int)
(** [decompose_apic apicid] is the [smt * core * package] id of
    [apicid]. Can throw [invalid_argument] if cpu_vendor is unknown or
    [apicid] is invalid *)

val cpu_vendor : string
(** [cpu_vendor] is the cpu brand, Genuineintel, AuthenticAMD and so on *)

val cpu_model : string
(** [cpu_model] model like "Intel(R) Core(TM) i3-10100 CPU @ 3.60GHz" *)
