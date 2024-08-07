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
(** Expresses a logical CPU/thread. *)

(** The [kind] of a {!Cpu.t} *)
type kind =
  | P_core  (** Performance cpu, faster, power hungry one *)
  | E_core  (** Energy efficient, slower, global warming aware one *)

(** A logical CPU *)
type t =
  { id : int  (** A monotonically increasing id *)
  ; kind : kind  (** [Performance] or [Energy Efficient]*)
  ; smt : int  (** The smt/thread id *)
  ; core : int  (** The core id, a core can have multiple smt/threads *)
  ; socket : int  (** The socked id, a socket can have multiple cores *)
  }

(** [id t] is [t.id] *)
val id : t -> int

(** [from_smt smt cpulist] are all {!Cpu.t} of [cpulist] of smt [smt] *)
val from_smt : int -> t list -> t list

(** [from_core core cpulist] are all {!Cpu.t} of [cpulist] of core [core] *)
val from_core : int -> t list -> t list

(** [from_socket cpulist] are all {!Cpu.t} of [cpulist] of socket [socket] *)
val from_socket : int -> t list -> t list

(** [dump cpu] Outputs the contents of {!Cpu.t} to stdout *)
val dump : t -> unit

(** [make kind id smt core socket] is a constructor for {!Cpu.t} *)
val make : kind:kind -> id:int -> smt:int -> core:int -> socket:int -> t
