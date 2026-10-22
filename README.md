# virtu-lang

virtu-lang is a domain-specific language. It comes from lang-template with
the tcc-json host.

The type constructors are only cell monads and their duals (comonads), as
cubical type theory realizes them. The base types and the base operations
are only the four classes of the request: `Avatar`, `VirtuCard`,
`VirtualTerminal` and `VirtualItem`. `langc build` writes the JSON document
of the final state of each instance that the program makes.
`docs/PLAN.md` gives the reading of the request, the slices and the open
items. This seed (S0) is the kit with the sample domain. The kernel and the
domain come in the later slices.

- `SPEC.md`: the specification. Fill each section.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `docs/PLAN.md`: the plan.
- `formers/FORMERS.md`: the type formers. `formers/tcc-json.md` gives their
  status on the tcc-json host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.
