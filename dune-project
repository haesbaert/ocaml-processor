(lang dune 3.2)

(name processor)
(implicit_transitive_deps false)

(generate_opam_files true)

(source
 (github haesbaert/ocaml-processor))

(authors "Christiano Haesbaert")

(maintainers "Léo Andrès <contact@ndrs.fr>")

(license ISC)

(documentation https://haesbaert.github.io/ocaml-processor)

(package
 (name processor)
 (synopsis "Processor Topology & Affinity for ocaml")
 (description "Exposes processor, CPU Topology and lets you pin the runtime to a set of CPUs.")
 (depends (ocaml (>= 4.08)) dune)
 (tags
  (processor multicore cpu thread core package socket affinity scheduler domain)))
