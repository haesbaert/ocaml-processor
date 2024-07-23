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

let _ =
  Printf.printf "cpu_count: %d\n" Query.cpu_count;
  Printf.printf "core_count: %d\n" Query.core_count;
  Printf.printf "socket_count: %d\n" Query.socket_count;
  Printf.printf "cpus-per-core: %d\n" (Query.cpu_count / Query.core_count);
  Printf.printf "cpus-per-socket: %d\n" (Query.cpu_count / Query.socket_count);
  Printf.printf "cores-per-socket: %d\n" (Query.core_count / Query.socket_count);
  List.iter Cpu.dump Topology.t
