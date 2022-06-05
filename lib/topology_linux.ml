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

let cpu_of_attrs attrs =
  let i a = List.assoc a attrs |> int_of_string in
  let s a = List.assoc a attrs in
  let smt = ((i "apicid") land 1) in
  Lcpu.make
    ~model:(s "model name")
    ~id:(i "processor")
    ~smt
    ~core:(i "core id")
    ~socket:(i "physical id")
    ~cache_alignment:(i "cache_alignment")

let parse_proc () =
  let proc = open_in "/proc/cpuinfo" in
  let rec loop cpus attrs =
    match input_line proc with
    | exception End_of_file -> cpus
    | line ->
      match Str.split_delim (Str.regexp "\t*: *") line with
      | [] -> loop ((cpu_of_attrs (List.rev attrs)) :: cpus) []
      | k :: [] -> let v = "" in loop cpus ((k, v) :: attrs)
      | k :: v :: [] -> loop cpus ((k, v) :: attrs)
      | tokens -> invalid_arg (Printf.sprintf "Unexpected number of tokens (%d) in\n%s%!"
                            (List.length tokens) line)
  in
  let cpus = loop [] [] in close_in_noerr proc; List.rev cpus

let make () = parse_proc ()

