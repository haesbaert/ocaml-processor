(lang dune 3.2)

(name processor)

(generate_opam_files true)

(source
 (github haesbaert/ocaml-processor))

(authors "Christiano Haesbaert")

(maintainers "Christiano Haesbaert")

(license ISC)

(documentation https://url/to/documentation)

(package
 (name processor)
 (synopsis "Processor Topology & Affinity for ocaml.")
 (description "Lets you pin current process to sets of CPUs as well as query the system CPU topology.")
 (depends ocaml dune)
 (tags
  (processor multicore cpu thread core package socket affinity scheduler domain)))
