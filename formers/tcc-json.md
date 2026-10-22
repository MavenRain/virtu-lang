# TinyCC JSON realization

Paths below are relative to this host kit, or to the root of a generated
language. The template's `formers/FORMERS.md` gives the host-neutral laws.
The front end in `src/front/` is a fork of the tcc-wasm front end (`README.md`,
section Origin). Each status covers `langc check`, `langc eval` and
`langc build`. `test/gate.sh` checks every example, every eval line in
`test/eval/expect.txt` and every JSON golden in `test/json/`. `make check`
also runs `test/regressions.js` for type safety, complete type metadata,
binder scope and domain declarations.

| ID | Former | Status and evidence |
|---|---|---|
| F1 | Product | DONE: `pair`, `first`, `second`; `examples/formers.lang`, `examples/functions.lang` |
| F2 | Coproduct | DONE: `inl`, `inr`; `examples/formers.lang`, `examples/laws.lang` |
| F3 | Option | DONE: `none`, `some`; `examples/formers.lang`, `examples/monad.lang` |
| F4 | List | DONE: `nil`, `cons`; `examples/formers.lang`, `examples/inventory.lang` |
| F5 | Monad pure/map/bind | DONE over Option, Sum E and List: `examples/monad.lang`, `examples/laws.lang` |
| F6 | Algebra fold | DONE over Nat, List and inductive families, including HIT boundary cases: `examples/algebra.lang`, `examples/circle.lang`, `examples/mutual.lang`; dependent `elim`: `examples/elim.lang` |
| F7 | Algebra unfold | DONE into List with a Nat step limit, and into cofamilies with `out`: `examples/algebra.lang`, `examples/stream.lang`, `examples/colist.lang`, `examples/mealy.lang` |
| F8 | Filter | DONE over Option and List: `examples/monad.lang`, `domain/domain.lang` |
| F9 | Non-dependent Pi | DONE: `examples/functions.lang` |
| F10 | Dependent Pi | DONE: `examples/functions.lang`, `examples/universes.lang` |
| F11 | Sigma | DONE: `pack`, `witness`, `payload`; `examples/sigma.lang` |
| F12 | Cubical paths | DONE: `Path`, `PathP`, `refl`, `sym`, `cong`, `funExt` and interval application; `examples/equality.lang`, `examples/paths.lang` |
| F13 | Transport and composition | DONE: `transp`, `hcomp`, Glue and `ua` at Type 0; `examples/kan.lang`, `examples/glue.lang`, `examples/univ.lang` |
| F14 | Universes | DONE: `Type 0` and `Type 1`, without cumulativity; `examples/universes.lang`, `test/check/cumulative.lang`, `test/check/type2.lang` |
| F15 | Generic indexed family | DONE in `domain/` only (rule R1): `Vec` in `domain/domain.lang`, `examples/indexed.lang`, `examples/mutual.lang` |

JSON output of each former: `docs/host/README.md`, section The JSON document.
Functions, types, families and top-level coinductive values are not instances.
Paths are instances, including paths of functions and types. A constant
path emits `{"refl": value}`; a Sigma payload of path type uses the path
JSON rules. Nested coinductive values are refused.
