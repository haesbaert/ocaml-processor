(lang dune 3.2)

(name cpu)

(generate_opam_files true)

(source
 (github haesbaert/ocaml-cpu))

(authors "Christiano Haesbaert")

(maintainers "Christiano Haesbaert")

(license ISC)

(documentation https://url/to/documentation)

(package
 (name cpu)
 (synopsis "CPU Topology & Affinity for ocaml-multicore.")
 (description "Lets you pin Domains to sets of cpus as well as query the system CPU topology.")
 (depends ocaml dune)
 (tags
  (multicore cpu thread core package socket affinity scheduler domain)))

; See the complete stanza docs at https://dune.readthedocs.io/en/stable/dune-files.html#dune-project
