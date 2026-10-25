# virtu-lang plan

## 1. Rulings

The USER gave these rulings in the request (`design/DESIGN.md`):

- The type constructors and the higher kinded types are only the cell
  monads and their duals in a locally presentable, locally cartesian
  closed (LCC) (infinity, 1)-category, as cubical type theory (CTT)
  realizes it computationally (section 2).
- The base types and the base operations are only the four classes of the
  two images: `Avatar`, `VirtuCard`, `VirtualTerminal` and `VirtualItem`,
  with their 27 rows (sections 4 and 5).
- Host: TinyCC. Output: the JSON document of the final state of each
  instance that the program makes (section 6).

These are the builder's calls. The USER did not rule them. Section 8 lists
them as open items:

- Name: virtu-lang (O1). All four classes are virtual objects, and one is
  the `VirtuCard`. The compiler binary is `build/langc`.
- Host kit: `hosts/tcc-json` of lang-template 5265a2a, without change. The
  cubical kernel goes into this repository (O2). The kit files are the same
  as at b76b859, the agent-lang seed (`git diff b76b859 5265a2a --
  hosts/tcc-json` is empty).
- Tests: unit and integration tests are in the guest language. A host
  runner in C that loads and runs guest test files is permitted. Do not add
  a node, Python or shell harness. The kit JS regressions stay.

### USER ruling (S0)

On 2026-10-22 the USER accepted every proposal: O1 to O24 are all (a)
(section 8). These are now rulings, not open items:

- O1 the name virtu-lang; O2 the kernel in this repository.
- O3 to O8 the type constructors: the ambient `Sigma`, `Pi`, `Path` and
  `U` with no user type declaration (O3); a cell monad is the free algebra
  monad of a cell signature, as a HIT (O4); 0-cocells, and a higher cocell
  only as `P * -` (O5); laws as paths, no proof irrelevance (O6); `Virt`,
  the world `Store W` and the runner (O7); `data` and `family` refused in
  a program and kept in the kernel (O8).
- O9 to O22 the base span: each carrier only as far as a row needs it
  (O9); `Text` with literals and `textEq` only (O10); `Credits` as 64-bit
  natural numbers with a refusal at overflow (O11); `Role := Text` (O12);
  a new card is `low` (O13); rarity multipliers 1, 2, 4, 8, 16 (O14);
  `Value := Text` (O15); one history entry for each balance change (O16);
  a listing from row 19 keeps the price at the time of listing (O17);
  row 8 does not attach, `attachCard` needs the owner (O18);
  `tap_denominator` adds bonus credits (O19); not enough credits gives
  `none` and no change (O20); a purchase takes the first listing with the
  name and removes it (O21); camel case source names, `kind` for the item
  type (O22).
- O23 the laws L1 to L5 as 1-cells of `Virt` in S6; O24 the output is the
  instances of the run of `main`, with references as `id`.

The claims that PLAN 2.2 and 2.3 mark "to check" stay claims. The S1
kickoff checks them.

### USER ruling (S1 kickoff)

On 2026-10-22 the USER accepted every S1 kickoff proposal: K1 to K12 are
all (a). The kernel donor is agent-lang 06854bf. These are now rulings:

- K1 the big `Nat` stays (the port is a copy), so `Nat` has no bound. S5
  gives `Credits` its 64-bit refusal at overflow (O11). The agent-lang
  `trap.lang` (EVAL_FUEL) and `nat-range.lang` (FIN_RANGE) tests replace
  the seed tests.
- K2 when a file is in the two repositories and the two copies differ,
  the agent-lang copy wins. S1 then makes only the removals and the verb
  change on it.
- K3 test/path-regressions.c keeps the kernel helpers and rows. The IO,
  filesystem and Ollama helpers and rows go, and the rows that use a
  removed type go or change.
- K4 `Fin` stays a kernel builtin in S1 (`Bool := Fin 2`). S2 adds
  `Fin k` as a library cell monad and a path that shows that the two
  forms are equivalent.
- K5 S1 uses the agent-lang domain.lang without Json, Log and IO. The
  seed types (Color, Flag, Item, Stack, Lot) go. The agent-lang families
  are kernel fixtures, not base types. S5 moves the fixtures that are not
  cell monads, duals or carriers to a domain file that only the gate
  loads.
- K6 the S1 examples are 27 kernel examples: 24 without change, and nat,
  inventory and laws without the parts that use a removed type.
- K7 each removed name (`Text`, `Int64`, `Json`, `Log`, `IO`) has a guest
  check refusal test, a string literal has a parse refusal test, and the
  gate has a row for the removed `json` verb.
- K8 docs/STATUS.md `## S1` records each removed feature with its
  agent-lang source and the command that restores it.
- K9 S1 has 10 mutants: the kickoff mutants f1 to f9, and M10, which
  puts the `Text` builtin row back.
- K10 gen/embed.c loses the `-r` runtime embed mode.
- K11 json.c loses the `Text` and `Int64` value cases and the Json
  family code. S5 adds back only the `Text` output for the carriers
  (O10).
- K12 the edits of sections 2.2 and 2.3 from the claim checks C1 to C3
  are accepted. All three claims are confirmed. The `State S X` row now
  has the hypothesis "for sets `S` and `X`".

### USER ruling (S2 kickoff)

On 2026-10-22 the USER said: "Take all recommendations for K1-K12." Thus
the S2 kickoff proposals K1 to K12 are all (a). These are now rulings:

- K1 the 0-cell syntax is `cell NAME PARAMS (X : Type 0) := op : TYPE |
  ...`. The last parameter is the carrier. Each op type is a Pi over
  parameters that do not name X, then the arguments `X` or `B -> X`, then
  the result `X`. `NAME PARAMS V` is the free algebra on V. `pure` is
  implicit.
- K2 a program can declare a cell. A program `family` or `cofamily` keeps
  REFUSE_DATA (O8), and the message names `cell`.
- K3 a cell becomes a family with an is_cell flag. It has one constructor
  for each 0-cell and `pure (v : V)` last. A constructor can have a
  continuation field `B -> T`. Its induction result has the type `B -> Y`.
- K4 the Carrier enum gets a cell carrier that holds the family. `pure` is
  the pure constructor, `bind` is the fold with the pure case `f`, and
  `map f = bind (pure . f)`. `filter` stays on Option and List.
- K5 a new section of domain/domain.lang declares `Either`, `Free`,
  `OptionC`, `ListC`, `NatC` and `FinC`. The builtins keep their names.
  The library gives a path from each cell to its builtin by `ua` and
  `isoToEquiv`.
- K6 `cell FinC (k : Nat) (X : Type 0) := pick : Fin k -> X`. Thus
  `FinC k Empty = Fin k` for each k. Enumerations at a literal k are S5
  cells.
- K7 hcomp_data loses the guard that makes hcomp at a family stuck. A HIT
  still goes to hcomp_formal.
- K8 examples/cell-laws.lang gives the monad laws of each library cell as
  paths: left unit by `refl`, right unit and associativity by `elim`.
- K9 the 27 `Virt` operations are in S5. S2 gives only the cell form and
  the library.
- K10 S2 adds 5 examples (cells, free, state-free, cell-laws, fin-cell), 8
  check refusal files and 1 parse refusal file. No new host harness.
- K11 S2 has 10 mutants, N1 to N10.
- K12 the predicted S2 gate is: examples 32, parse 14, check 86, evals
  about 123, json 32 builds, build refusals 4, regressions 6, printer 2,
  path regressions 148.

### USER ruling (S3 kickoff)

On 2026-10-22 the USER said: "K1 to K13 all (a)". Thus the S3 kickoff
proposals K1 to K13 are all (a). These are now rulings:

- K1 an n-cell in a `cell` has the type form `op : (a : A) -> ... ->
  (i : I) [-> (j : I)] -> X [ faces ]`. The parser reads the system after
  the op type.
- K2 a path op is a path constructor of the cell family. `pure` stays
  last and cannot be in a boundary. `bind` and `map` keep the path cell at
  the images. The fold has a point case for a 0-cell, a path case for a
  1-cell and a square case (isSet of the result) for a 2-cell.
- K3 domain/domain.lang gets `Trunc1C`, `Trunc0C` and `QuotC A R X`, with
  the maps to and from `Trunc1`, `Trunc0` and `Quot` in
  examples/ncell.lang. If the kernel refuses a `Path X x y` op field,
  cell_field_type gets a Path over the carrier.
- K4 there is no new set-target rule. The square case refuses a fold into
  a type that is not a set.
- K5 domain/domain.lang gets `StateC S X` with the 0-cells `getC` and
  `putC`, the 4 laws GP, GG, PP and PG as 1-cells (GG over `Prod S S`) and
  the `Trunc0` square. examples/state.lang gives the fold into
  `S -> Prod S X`, the inverse and the two round trips.
- K6 examples/loc.lang gives `Null2`, the nullification at `Fin 2`
  (Rijke, Shulman and Spitters 2020, Definition 2.14), and its fold into
  `Trunc1`.
- K7 the fold_image fix of dbc8e1a stays.
- K8 F1 to F3 go to a later kernel slice.
- K9 a missing fold case message names the cell or HIT case. `NAME.pure`
  stays. The build tries `bind (pure a) f` and records the result.
- K10 S3 adds 3 examples, 1 parse refusal file, 7 check refusal files,
  about 12 evals and 6 path regressions. No new host harness.
- K11 S3 has 10 mutants, P1 to P10.
- K12 the predicted S3 gate is: examples 35, parse 15, check 93, evals
  about 140, json 35 builds or fewer, build refusals 5, regressions 6,
  printer 2, path regressions 164.
- K13 PLAN 2.2 cites Rijke, Shulman and Spitters 2020 for the
  localization HIT. The State row says "for sets S and X".

### USER ruling (S4 kickoff)

On 2026-10-23 the USER said: "Rule defaults for K1-K12". Thus the S4
kickoff proposals K1 to K12 are all (a). These are now rulings:

- K1 a cocell has the form `cocell D (ps) (X : Type 0) := obs1 : X -> T1
  | obs2 : X -> T2`. Each op type starts with `X ->`. An X in T_i is the
  self position. `extract` is implicit and its name is reserved. `cocell`
  is a keyword. A cocell with no op is a parse refusal.
- K2 a cocell is `nu Y. X * F Y`. The kernel makes the layer family and
  the cofamily. A Sigma observation splits into 2 fields. The builtins
  `extract`, `extend` and `duplicate` make a canonical node: one
  projection of `out s` for each field. A program can declare a cocell.
  USER ruling (2026-10-23): the node of a cocell D is the surface
  constructor `nodeD`, a core name under rule R4. `D.layer`,
  `D.extract` and `D.Y` stay hidden (STATUS S4 D1).
- K3 the type of the step is the guard. There is no new rule.
- K4 domain/domain.lang gets `EnvK`, `StreamK` and `CofreeK`. Store is a
  library comonad with a direct layer, not a cocell comonad.
  examples/cocell.lang gives Store by hand and the maps between
  `StreamK X` and `Stream X`.
- K5 conversion up to bisimulation. A neutral converts with an unfold by
  eta. Two unfolds convert when a hypothesis (g1, g2, h) passes one check
  at a fresh variable, and each check spends fuel. L3 and
  coassociativity in the form `extend duplicate w = duplicate (duplicate
  w)` are refl. The map form of coassociativity and codata-bisim.lang
  stay refused.
- K6 `P * -` is a cocell with a parameter pair `(P : Type 0)
  (pp : isProp P)`. The idempotence is an open point.
- K7 the kit `unfold` into `List` goes. examples/algebra.lang `down` uses
  `take`.
- K8 the S3 leftovers: the D1 message names the cell, path_case_type
  gives a diagnostic, the c1 and c6 killers, fixtures for the refusal
  paths with no fixture, and tries for the P5, P10 and P10b killers.
- K9 the tests are in the guest language and in the C runner. There is no
  new host harness.
- K10 S4 has 9 mutants, R1 to R9.
- K11 the predicted S4 gate is: examples 36, parse 16, check 101, evals
  about 155, json 36, build refusals 5, regressions 6, printer 2, path
  regressions 178.
- K12 PLAN 2.3 changes: the Stream dual is `Nat * X`, the Store row names
  `pos`, `seek` and the lens laws, and the comonad laws cite the sources
  and the K5 route.


S4 staged review correction: the proposed variable graph relation failed transitivity and substitution checks and is removed from judgmental conversion. The implementation retains layer eta, typed eta at closed or neutral seeds, and uniform comparisons with convertible seeds of the same type. A cofamily without recursive Y fields compares by its finite observations. L1, L2 and L3 remain `refl`; Store laws and EnvK coassociativity also check. Full StreamK/CofreeK coassociativity and stream round trips require a stronger path rule and are currently explicit `refl` refusals. The kickoff ruling above records the original proposal; it does not establish these unsupported conversion cases.

### USER ruling (S5 kickoff)

On 2026-10-24 the USER said: "Rule defaults for K1-K14 (S5-kickoff.md
`## STEP 3 K rows`). Follow ~/Documents/virtu-lang-work/S5-build-PROMPT.md."
Thus the S5 kickoff proposals K1 to K14 are all (a). These are now rulings:

- K1 `Text` comes back as a kernel builtin with string literals (double
  quotes) and `textEq : Text -> Text -> Option Unit` only. `Role := Text`
  and `Value := Text`. `Credits := Nat` with the library `creditsAdd`,
  `creditsSub` (to `Option`), `creditsMul` and `creditsLe`. `creditsAdd` and
  `creditsMul` call one kernel builtin that refuses a value above 2^64 - 1
  at run time. `Entry` and `Listing` are `Prod` chains. The kernel fixtures
  that are not cell monads, duals or carriers go to a domain file that only
  the gate loads.
- K2 each class is a reference type with its own type: `cell AvatarC
  (X : Type 0) := avatarRef : Nat -> X` and `Avatar := AvatarC Empty`, and
  the same for `VirtuCard`, `VirtualTerminal` and `VirtualItem`. The state
  is in the world. An id with no instance is a run-time refusal.
- K3 one `cell Virt (X : Type 0)` has 28 0-cells (27 rows; row 4 gives
  `avatarCard` and `attachCard`). An op with the result R has a field
  `R -> X`. An op with the result `Unit` has a field `X`. The op fields have
  no names.
- K4 `W := List Instance`, in run order. `Instance` is a cell at `Empty`
  with `avatarI`, `cardI`, `terminalI` and `itemI`, each with the state
  record of section 4. The id of an instance is its position.
- K5 `run : (X : Type 0) -> Virt X -> W -> Prod W X` is a library fold with
  one case for each 0-cell. `langc build` finds `def main : Virt Unit`, runs
  it from the empty world and writes the final world.
- K6 json.c writes the world: `{"virtu-lang":1,"instances":[...]}` with the
  keys `id`, `class` and `state`. Text is a JSON string, Credits a JSON
  number, an enumeration its name, a reference its id, an `Option` of a
  reference the id or `null`, and `attributes` an object in the order of
  the first set. A program with no `main` keeps the S1 to S4 form.
- K7 `SecurityLevelC` (4), `RarityC` (5) and the entry kinds (7) are cells
  with nullary 0-cells at `Empty`, each with a path to `Fin k` by `ua` and
  `isoToEquiv`. `multiplier : Rarity -> Credits` is a fold.
- K8 the S4 leftovers stay open: S4 open points 8, 9 and 10, the review
  leftovers and the historical S4 K10 table.
- K9 the tests are in the guest language and in the C runner:
  examples/virt.lang, examples/carriers.lang, test/json/virt.json, check,
  parse and build refusals, and path regressions. There is no new host
  harness.
- K10 S5 runs U1 to U10 again at the S5 tree and adds the sites of the new
  code (V1 to Vn). U2 gets a killer try or an equivalent-mutant note.
- K11 the predicted S5 gate is: examples 39, parse 16, check about 124,
  evals about 185, json 38, build refusals 7, regressions 6, printer 2,
  path regressions about 203.
- K12 PLAN 2.5 cites Plotkin and Pretnar for the runner, qualifies the
  Katsumata, Rivas and Uustalu cite and gives the Jacobs details. PLAN 2.5
  and 5.1 say "28 0-cells (27 rows)". PLAN 6 gets the RFC 8259 notes. The
  S4 kickoff ruling and the "S4 staged review correction" paragraph both
  stay.
- K13 S5 accepts that conversion is incomplete at full StreamK and CofreeK
  coassociativity. A stronger path rule needs a source for the cubical
  model first.
- K14 Q2 is dropped. The all-pairs idea is future work that needs a
  soundness argument first.

### USER ruling (S6 kickoff)

On 2026-10-24 the USER said: "Follow
~/Documents/virtu-lang-work/S6-build-PROMPT.md. After, if necessary, give
next session prompt." To the question on the S6 K rows (K1 to K11 and Q1
to Q4 of S6-kickoff.md `## STEP 3 K rows`), the USER selected "All (a)
(Recommended)". Thus the S6 kickoff proposals are all (a). These are now
rulings:

- K1 the laws are L1 to L4 of 5.3, with L4 in the continuation form
  `avatarSet a k v (avatarGet a k t) = avatarSet a k v (t (some v))` for
  `t : Option Value -> X` (and the same for `itemSet`), and L5 in the Prod
  form `r a (fun x => r a (fun y => k (pair x y))) = r a (fun x => k (pair
  x x))` for `k : Prod R R -> X`, one law for each of the 11 read rows. L1
  to L3 are schema 3, L4 is schema 4 and L5 is schema 2 of the 7 global
  state schemas of Plotkin and Power.
- K2 a 1-cell declaration needs no kernel change. Each 1-cell takes X
  explicitly. Each field name is new at the global level.
- K3 in S6 only L5 is a 1-cell of `Virt`: 11 1-cells after `itemGet` and
  before `pure`. `Virt` has 40 constructors, and the fold of `run` has 40
  cases and 41 arguments. L1 to L4 are laws at known references, with
  closed-world path regressions.
- K4 each L5 path case of `run` is `refl`. L1 to L4 get no `run` path in
  S6.
- K5 a new slice does the routes R1 to R4, with its own kickoff and a row
  in section 7. S6 has no kernel change.
- K6 STATUS and 5.3 give the S6 answer to the S5 open points 1, 2, 5, 6
  and 8. Open point 9 and the sites U1 to U10 stay.
- K7 the tests are a new example with its JSON golden, 5 check refusals,
  1 build refusal and 20 path regressions (11 L5 law uses, 6 closed-world
  rows for L1 to L4, 3 two-dimensional rows).
- K8 the S6 close runs W1 to W7 again at the S6 tree. W3 and W6 get
  equivalent-mutant notes.
- K9 the predicted S6 gate is: examples 40, parse 16, check 130, evals
  210 + n, json 40, build refusals 8, regressions 6, printer 2, path
  regressions 255.
- K10 5.3 gets the K1 forms, the condition "at a known reference", and
  the C22 and C23 cites. Section 7 gets a row for the R1 to R4 slice.
- K11 Q1 (a): a refusal stays a refusal; the new slice adds `refUnknown`
  propagation for the world lemma. Q2 (a): S6 has no L1 to L4 1-cells; O23
  (a) is complete after the R1 to R4 slice. Q3 (a): schema 1 and the
  schemas 5 to 7 are not in S6, and 5.3 says that it has 3 of the 7
  schemas. Q4 (a): the S6 build probes the 1-cell cycle on a scratch copy
  only, and records the result as an open point.

### USER ruling (S6R kickoff)

On 2026-10-25 the USER said: "Follow
~/Documents/virtu-lang-work/S6R-build-PROMPT.md. Rulings: K1-K11 and
Q1-Q3 all (a). After, if necessary, give next session prompt." Thus the
S6R kickoff proposals (S6R-kickoff.md `## STEP 3 K rows`) are all (a).
These are now rulings:

- K1 the world is a list cell `WorldC` (`W := WorldC Unit`, nil is
  `pure unit`), and `Attributes` is a list cell `AttrC`. The list
  functions are folds over the cells. json.c reads the cells, and the JSON
  does not change. The kernel gets no `List` eliminator.
- K2 the library has a Bool cell, and the compares `natEqB` and
  `textEqB` give it. The world lemmas split by `elim` on the Bool cell.
  The kernel gets no rule for `natEq n n`.
- K3 a refusal stays a refusal. The kernel propagates `refUnknown` at a
  neutral scrutinee of the List fold, the family fold, the Option elim and
  `finCase`, and in apply_value. The result type comes from the motive.
  Conv compares two `refUnknown` values by their ids.
- K4 `textEq t u` gives `some unit` when `t` and `u` are convertible, at
  the conv level of the machine. A confluence note goes with the rule.
- K5 the order of work: K2, K1, K4, K3 with Q1, the world lemmas, the 6
  1-cells.
- K6 `Virt` gets 6 1-cells (L1, L2, L3 for `avatarSet` and `itemSet`, L4
  for `avatarSet` and `itemSet`) after the 11 L5 1-cells and before
  `pure`. `Virt` then has 46 constructors. Each run path case applies a
  world lemma.
- K7 the tests: path regressions (6 law uses, 6 world lemmas, R3 and R4
  rows), check refusals (the false list lemma, `elim` on `List`, Q1), and
  one example with its JSON golden.
- K8 the mutants X1 to X6, each with a killer.
- K9 the predicted gate: examples 41, parse 16, check refusals 133, evals
  212 + n, json 41, build refusals 8, regressions 6, printer 2, path
  regressions about 273.
- K10 the PLAN edits: 5.3, a soundness note for each kernel rule,
  section 7, and this section.
- K11 STATUS: open points 7 and 10 stay. F20 is a new open point unless
  Q1 closes it.
- Q1 (a): `check` refuses `refUnknown` in a program. Only the domain
  library names it. The build probes that no closed term of a false
  `Path` type checks through the library.
- Q2 (a): `W` and `Attributes` change to cells in one slice.
- Q3 (a): the R3 route is K3 (a).

Two rulings came during the S6R build. For each one, the USER chose an
option. The work files keep the option and the date, not more words.

- Q-S6R-s9-1 (i), USER 2026-10-25 (S6R build s10): one fold for the
  attribute write. `attrSet` is restated with the one fold `attrSetF`
  over the `AttrC` cell. The other options were (ii) a kernel `Option`
  elim and (iii) a restriction of the lemmas.
- Q-S6R-s11-1 (a), USER 2026-10-25 (S6R build s12, through the question
  tool): raise the arena of `accepts()` in test/path-regressions.c from
  128 MB to 512 MB, in the test only. After chunk B, the row `law L5
  2-dim: run of rrBal at an inner point (S6)` needs more than 128 MB. The
  other options were (b) a new form of the law cases and (c) a change to
  the conv of the engine. The CLI arena (1 GB, src/main.c) does not
  change: `langc check` of that row as a program gives rc 0 (S6R build
  s13).

## 2. The type constructors

### 2.1 The ambient category

The request names the category, and the category gives structure that is
not a type constructor of a program:

- LCC: dependent sums `Sigma`, dependent products `Pi` and identity types.
  CTT realizes the identity type as `Path`, with the interval, `PathP`,
  `transp`, `hcomp`, Glue and `ua` (CCHM). The universe is univalent.
- Locally presentable: the category has all small colimits, and an
  accessible endofunctor has an initial algebra and a final coalgebra. CTT
  realizes the colimits as higher inductive types (HITs), the initial
  algebras as inductive types and the final coalgebras as coinductive
  types.

Proposal (O3 (a)): `Sigma`, `Pi`, `Path` (with the CCHM operations) and
the universe `U` are the ambient structure. A program uses them, but it
declares no type with them. Every other type comes from a cell monad or
from its dual (2.2 and 2.3).

### 2.2 Cell monads

The request does not define "cell monad". This plan reads it as follows
(O4 (a)), because each part has a direct CTT realization:

- A cell signature `C` is a list of cells. A 0-cell is an operation with an
  arity: `op : (a : A) -> (B a -> X) -> X` (a container `(A, B)`). An
  n-cell, n >= 1, is a path constructor of dimension n. Its boundary is a
  term in the earlier cells. A 1-cell is an equation between operations. A
  higher cell is a coherence.
- The cell monad `T_C` sends a type `X` to the HIT that `pure : X -> T_C X`
  and the cells generate: the free `C`-algebra on `X`.
- `bind : T_C X -> (X -> T_C Y) -> T_C Y` is the eliminator. On a 0-cell
  it keeps the cell. On an n-cell it gives the same n-cell, applied to the
  images of the boundary. `map f = bind (pure . f)`. `fold` into any
  `C`-algebra is the general eliminator: the algebra gives a case for each
  0-cell and a path (or a higher path) for each n-cell.
- The monad laws are paths (2.4).

In a locally presentable (infinity, 1)-category, a reflective localization
is accessible exactly when it is the localization at a small set of maps
(Lurie, Higher Topos Theory, Propositions 5.5.4.2 and 5.5.4.15; S1
kickoff C3). Thus the idempotent cell monads
are the accessible localizations, and CTT gives each one as a HIT
(Christensen, Opie, Rijke and Scoccola, "Localization in homotopy type
theory", 2020). The HIT constructors are in Rijke, Shulman and Spitters,
"Modalities in homotopy type theory", 2020, Definition 2.14 and Theorem
2.18 (S3 kickoff C10). The non-idempotent cell monads are the free algebra monads
of the signatures.

| Cell monad | Cells | Use in this plan |
|---|---|---|
| `Option X` | one 0-cell `none` of arity 0 | a lookup that can fail |
| `Either E X` | one 0-cell `throw : E -> T X` | a failure with a reason |
| `Free (A, B) X` | one 0-cell `op : (a : A) -> (B a -> T X) -> T X` | trees |
| `List A := Free (A, \_. Unit) Unit` | one 0-cell `cons : A -> T -> T`; `nil := pure tt` | inventories, histories |
| `Nat := Free (Unit, \_. Unit) Unit` | one 0-cell `succ : T -> T` | credits (O9) |
| finite enumerations `Fin k` | k 0-cells of arity 0, at `Empty` | security levels, rarities |
| `Trunc1 X` | one 1-cell `squash : (x y : T X) -> Path (T X) x y` | the propositional truncation |
| `Trunc0 X` | one 2-cell | the set truncation |
| `Quot X R` | one 1-cell for each `R x y`, with `Trunc0` | quotients |
| `State S X` | 0-cells `get` and `put`, 1-cells for the laws of state, with `Trunc0`, for sets `S` and `X` | the program monad (2.5) |

The S1 kickoff checked the first row below (C1). The second row is a note:

- `State S X`: the free model of the theory of state on `X` is
  `S -> S * X` (Plotkin and Power, "Notions of Computation Determine
  Monads", 2002, section 3, Theorem 1). For one location the laws are the
  four interaction laws: GP `get (\s. put s t) = t`; GG
  `get (\s. get (\s'. k s s')) = get (\s. k s s)`; PP
  `put s (put s' t) = put s' t`; PG `put s (get k) = put s (k s)`. In CTT
  the fold of the HIT into `S -> S * X` sends each law 1-cell to `refl`.
  The fold is an equivalence when `S` and `X` are both sets: the `Trunc0`
  cell needs a set as the target. No source gives this CTT form. The S3
  probe must show: the declaration checks (`get` has the arity `S`); the
  fold checks with `refl` for each law; the inverse and the two round-trip
  paths check, with no choice; a fold into a target that is not a set is
  refused; and evals of the fold.
- `List A` and `Nat` as cell monads at `Unit`: the types are the
  inductive types of the kit. Only the declaration form changes.

`Empty` is the cell monad with no cells at the empty type. `Unit` is
`pure tt`. A finite coproduct `A + B` is `Either A` at `B`.

### 2.3 The duals

The dual of a cell monad is a comonad that cocells generate. A 0-cocell is
an observation with a co-arity: `obs : D X -> (a : A) * (B a -> D X)`. The
comonad `D_C` sends `X` to the final coalgebra that `extract : D_C X -> X`
and the cocells generate: the cofree `C`-coalgebra on `X`. `extend` and
`duplicate` come from `unfold`. The comonad laws are paths. A
bisimulation gives a path in an M-type (Ahrens, Capriotti and Spadotti,
"Non-wellfounded trees in homotopy type theory", TLCA 2015, Definition 4
and Theorem 18). Cubical Agda gives paths in a coinductive type by
copatterns over an interval variable (Vezzosi, Moertberg and Abel, JFP
2021, section 2.3). The kernel route (S4 K5) is limited after staged
review to seed-preserving bisimulation, typed eta, and finite observation
equality. The observation and layer eta rules require a single-constructor
layer. The proposed variable graph rule is deferred as described above.

| Comonad | Cocells | Dual of |
|---|---|---|
| `Env E X = E * X` | one 0-cocell `ask : D X -> E` | `Either E` |
| `Stream X` | one 0-cocell `next : D X -> D X` | `Nat * X` (the free monad on the identity, `mu Y. X + Y`) |
| `Cofree (A, B) X` | one 0-cocell `obs : D X -> (a : A) * (B a -> D X)` | `Free (A, B)` |
| `Store S X = S * (S -> X)` | the costate co-operations `pos` and `seek` with the lens laws as coequations (Gibbons and Johnson 2012, section 3.2; Garner 2021, Example 16). Store is a library comonad, not a cocell comonad: the bare cocells give `nu Y. X * S * (S -> Y)` | `State S` |

The dual of an n-cell (n >= 1) is a colocalization. An internal
coreflection in HoTT is `X |-> P * X` for a proposition `P` (Shulman,
"Brouwer's fixed-point theorem in real-cohesive homotopy type theory",
2018, Theorem 4.1; S1 kickoff C2). The hypotheses: a predicate on the
universe that is invariant under equivalence, an operation on the
universe into that predicate, a counit, and the universal property, all
stated in type theory. Then `P` is the coreflection of `Unit`. Thus a
higher cocell gives only that case. Ruling (O5 (a)): admit 0-cocells, and
admit a higher cocell only in the form `P * -`.

CTT realizes the final coalgebras with coinductive families, a guarded
`unfold` and a lazy `out` (agent-lang S4). The kit `unfold` into `List`
with a step limit is not a final coalgebra, and it goes (S4).

### 2.4 Laws and proofs

A proof is data, and a path is data. The kernel and the JSON writer keep
them. The request does not name proof irrelevance. CTT has none, and this
plan adds none: no `Prop` or `SProp` universe, no UIP, no axiom K, and no
conversion that makes two paths equal because they have the same type. The
monad laws, the comonad laws and the laws of each cell are paths (O6).

### 2.5 Objects: the program monad and the world comonad

The four classes of the images have state that the operations change. This
plan gives that state with one cell monad and its dual (O7 (a)):

- The program monad `Virt` is the cell monad whose 28 0-cells (27 rows)
  are the operations of section 5. Its 1-cells are the laws of section 5.3
  (S6). A program is `def main : Virt Unit`.
- The world `W` is the state of all instances: for each class, the list of
  its instances in the order the program makes them. The world comonad is
  `Store W`, the dual of `State W` (2.3). An instance is a coalgebra of its
  class interface (objects as coalgebras: B. Jacobs, "Objects and classes,
  co-algebraically", in Object-Orientation with Parallelism and
  Persistence, Kluwer, 1996, pp. 83 to 103; CWI report CS-R9536, 1995). S5
  gives an instance as an id and a state in the world, the identifier and
  local state form of the source; S5 does not build the creation by the
  terminal coalgebra (C19).
- The runner is the eliminator of `Virt` into the `Store W` carrier
  `W -> W * X`: the homomorphism out of the free model, the handler form of
  G. Plotkin and M. Pretnar, "Handling algebraic effects", LMCS 9(4:23),
  2013 (C20). Its case for a 0-cell runs the operation on the world. Its
  case for a 1-cell is a path that shows that the law holds in the world.
  The beta equations give this homomorphism; its uniqueness needs an eta
  equation (C20).
- The carrier `W -> W * X` has the form of a stateful runner (S. Katsumata,
  E. Rivas and T. Uustalu, "Interaction laws of monads and comonads", LICS
  2020, the abstract). S5 does not check the State/Store interaction law of
  that paper (C18).
- `langc build` runs `main` from the empty world and writes the final world
  (section 6).

Alternative (O7 (b)): an instance is a value, and each operation returns
the new value. There is no world and no `Virt`. Then "each instance created
in the program" has no clear meaning when a program copies a value.

### 2.6 The request on the kit basis

| Concept | Request text | Encoding (proposal) | Slice |
|---|---|---|---|
| Ambient LCC structure | locally cartesian closed | `Sigma`, `Pi`, `Path`, `U` of the agent-lang kernel. No user type declaration with them (O3). | S1 |
| Paths, univalence | Cubical Type Theory | The interval, `PathP`, `transp`, `hcomp`, Glue and `ua` of agent-lang 06854bf. | S1 |
| Cell monads, 0-cells | cell monads | A `cell` declaration: a signature of 0-cells. `pure`, `bind`, `map`, `fold`. The kit `data` and `family` forms are refused (O8). | S2 |
| Cell monads, n-cells | cell monads; locally presentable | Path and square cells in a `cell` declaration: `Trunc1`, `Trunc0`, `Quot`, localizations. `bind` and `fold` through each cell. | S3 |
| Duals | their duals | A `cocell` declaration: a signature of 0-cocells. `extract`, `extend`, `duplicate`, `unfold` (guarded) and `out` (lazy). | S4 |
| Higher kinded types | higher-kinded types | A type operator is a term of `U -> U`. A cell monad and its dual are type operators. There is no other former. | S2 |
| Objects | instances of types created in the program | `Virt`, `Store W` and the runner (2.5). | S5 |

## 3. The kernel route

The kernel is a port. It is not a new design.

- S1 ports the agent-lang kernel at 06854bf (agent-lang S8, closed):
  `src/front/*` and `src/json.c`, with the CCHM kernel of coin-lang, the
  HITs (`S1`, `Trunc1`, `Trunc0`, `Quot`), the indexed families, the mutual
  blocks and the dependent eliminator of orbit-lang S6 and S7, and the
  coinductive families of agent-lang S4. agent-lang and this repository come
  from the same kit files (section 1), so the port is a copy.
- S1 removes the agent-lang parts that the request does not give: the base
  types `Text`, `Int64`, `Json` and `Log` (S5 adds back only the carriers of
  section 4), `IO`, the thirteen operations, the Ollama and filesystem
  drivers, libtcc in `langc` and the console program. The agent-lang
  `langc json` verb becomes `langc build` again.
- S2 to S4 put the declaration forms of the request in front of the kernel:
  `cell` and `cocell`. The kernel families stay as the target of these
  forms, but a program cannot declare a family directly (O8).
- S6R adds three kernel rules (S6R kickoff K3, K4 and Q1). K1 and K2 add
  no kernel rule: the world, `Attributes` and Bool are library cells, and
  json.c reads the cells. Each rule has a soundness note.
  - R3 (K3 (a)), eval.c. At a stuck `refUnknown T id` as the scrutinee,
    the `Option` elim and `finCase` give `refUnknown T' id`, where `T'`
    is the result type from the motive (the checker writes it into the
    core). The List fold gives it when the end of the list is a
    `refUnknown`. The fold over a domain family gives it at a
    `refUnknown` scrutinee, with the result type of the fold, and `elim`
    over a domain family gives it with the motive at the scrutinee
    (eval.c `fold_family` and `elim_family`). `apply_value` of `refUnknown (Pi A B) id` at `x` gives
    `refUnknown (B x) id`. Soundness note (C28): the rule adds no
    inhabitant of a type. Before the rule, `refUnknown` inhabits each
    type (F20, the ExTT inconsistency of C28). The rule moves a stuck
    term to the result type of its eliminator and keeps its id. The
    runner refuses `refUnknown` at a closed world (EVAL_REF_UNKNOWN), as
    in S5.
  - R4 (K4 (a)), eval.c `text_eq`. `textEq t u` gives `some unit` when
    `t` and `u` are convertible, also when they are neutral. The conv
    test uses the levels above the fresh levels, as `line_constant` does
    (open point 1). Else the S5 rule compares the bytes of two literals.
    Confluence note (C27): the rule is not left-linear. Its critical pair
    with the literal rule closes: two equal literals give `some unit` by
    each rule. The reduction terminates, thus the Klop counterexample
    does not apply. C27 stays PARTIAL until the paper section is read.
  - R2 (Q1 (a)), check.c. `check` refuses `refUnknown` in a program
    (REFUSE_REF_UNKNOWN). The checker keeps the origin of the
    declaration that it checks (D1). Only domain/domain.lang can name
    `refUnknown`. Soundness note (F20): R2 closes the direct route of
    F20, because a program cannot write `refUnknown`
    (test/check/ref-unknown-program.lang). The library can name it, thus
    F20 stays an open point for the library terms (S6R kickoff K11).

## 4. The base types

The request admits only the four classes. Their fields need carriers that
the images use but do not name: texts (names, roles, keys, descriptions,
categories, item types), amounts (balances, prices, base values), the
security levels, the rarities, and lists (inventories, histories,
listings). Proposal (O9 (a)): admit each carrier only as far as a row of
section 5 needs it. Each carrier is a cell monad of section 2.2 or a
primitive with no operation beyond the table.

| Carrier | Values | Operations (only these) | Open |
|---|---|---|---|
| `Text` | A UTF-8 byte string. | Literals; `textEq : Text -> Text -> Option Unit` (for the lookups by key and by name). No append, no slice, no length. | O10 |
| `Credits` | A natural number of the smallest unit. | Literals; add; subtract with `Option` (`none` when the result is below zero); multiply by a rarity multiplier; compare. A sum above 2^64 - 1 is a refusal at run time. | O11 |
| `Role` | `explorer`, `merchant`, and other texts (the image row ends with "…"). | Proposal (a): `Role := Text`, with the library constants `explorer` and `merchant`. | O12 |
| `SecurityLevel` | `low`, `medium`, `high`, `maximum`. | A finite enumeration (2.2). The level of a new card is `low`. | O13 |
| `Rarity` | `common`, `uncommon`, `rare`, `epic`, `legendary`. | A finite enumeration. The rarity of a new item is `common`. `multiplier : Rarity -> Credits`. | O14 |
| `Value` | The value of an attribute. | Proposal (a): `Value := Text`. | O15 |
| `List A` | `Free (A, \_. Unit) Unit` (2.2). | `nil`, `cons`, `fold`. | |
| `Entry` | One line of a card history. | A record: kind (`create`, `bonus`, `debit`, `credit`, `transfer-out`, `transfer-in`, `purchase`), amount, balance after. | O16 |
| `Listing` | One listing of a terminal. | A record: name, price, description, and the item when the listing comes from a `VirtualItem`. | O17 |

The four classes are reference types: `Avatar`, `VirtuCard`,
`VirtualTerminal` and `VirtualItem` are the identities of instances in the
world. The state of an instance is in the world, not in the reference.

| Class | State (JSON keys) |
|---|---|
| `Avatar` | `name`, `role`, `virtucard` (a card reference or `null`), `attributes` (key to value), `inventory` (item references) |
| `VirtuCard` | `owner` (an avatar reference), `balance`, `security_level`, `history` (entries) |
| `VirtualTerminal` | `name`, `category`, `listings` |
| `VirtualItem` | `name`, `type`, `base_value`, `rarity`, `attributes` |

## 5. The base operations

### 5.1 The operations

Each row of the images is one 0-cell of `Virt`, except row 4 (a read and a
write) and row 19 (the overload of row 18). Thus `Virt` has 28 0-cells
(27 rows): row 4 gives `avatarCard` and `attachCard`. A read of a field is a 0-cell,
because the field is in the world. A failure is a value: `Option` or
`Either Text`.

| # | Image row | Operation (proposal) | Effect on the world | Open |
|---|---|---|---|---|
| 1 | `Avatar(name, role)` | `avatar : Text -> Role -> Virt Avatar` | Makes an avatar with no card, no attributes and no items. | |
| 2 | `avatar.name` | `avatarName : Avatar -> Virt Text` | Read. | |
| 3 | `avatar.role` | `avatarRole : Avatar -> Virt Role` | Read. | |
| 4 | `avatar.virtucard` | `avatarCard : Avatar -> Virt (Option VirtuCard)`; `attachCard : Avatar -> VirtuCard -> Virt Unit` | Read; "set explicitly" is the write. | O18 |
| 5 | `avatar.set_attribute(key, value)` | `avatarSet : Avatar -> Text -> Value -> Virt Unit` | Sets or replaces the key. | |
| 6 | `avatar.get_attribute(key)` | `avatarGet : Avatar -> Text -> Virt (Option Value)` | Read. | |
| 7 | `avatar.get_inventory()` | `avatarInventory : Avatar -> Virt (List VirtualItem)` | Read, in the order of purchase. | |
| 8 | `create_virtucard(avatar, amount)` | `createVirtucard : Avatar -> Credits -> Virt VirtuCard` | Makes a card with the owner, the balance `amount`, `low` and one `create` entry. The card is not attached. | O18 |
| 9 | `card.balance` | `cardBalance : VirtuCard -> Virt Credits` | Read. | |
| 10 | `card.security_level` | `cardSecurity : VirtuCard -> Virt SecurityLevel` | Read. | |
| 11 | `card.tap_denominator(amount)` | `tapDenominator : VirtuCard -> Credits -> Virt Unit` | Adds `amount` and a `bonus` entry. | O19 |
| 12 | `card.debit(amount)` | `debit : VirtuCard -> Credits -> Virt (Option Unit)` | When the balance is enough: subtracts and adds a `debit` entry. Else `none` and no change. | O20 |
| 13 | `card.credit(amount)` | `credit : VirtuCard -> Credits -> Virt Unit` | Adds `amount` and a `credit` entry. | |
| 14 | `card.set_security(level)` | `setSecurity : VirtuCard -> SecurityLevel -> Virt Unit` | Sets the level. No entry. | |
| 15 | `card.get_history()` | `cardHistory : VirtuCard -> Virt (List Entry)` | Read, oldest first. | |
| 16 | `transfer(from_card, to_card, amount)` | `transfer : VirtuCard -> VirtuCard -> Credits -> Virt (Option Unit)` | As row 12 on the source and row 13 on the target, or no change. Same card: `none`. | O20 |
| 17 | `VirtualTerminal(name, category)` | `terminal : Text -> Text -> Virt VirtualTerminal` | Makes a terminal with no listings. | |
| 18 | `terminal.list_item(name, price, desc)` | `listItem : VirtualTerminal -> Text -> Credits -> Text -> Virt Unit` | Adds a listing with no item. | O17 |
| 19 | `terminal.list_item(virtual_item)` | `listVirtualItem : VirtualTerminal -> VirtualItem -> Virt Unit` | Adds a listing with the item, its name and the price `calculate_value()` at the time of listing. | O17 |
| 20 | `terminal.get_inventory()` | `terminalInventory : VirtualTerminal -> Virt (List Listing)` | Read, in the order of listing. | |
| 21 | `terminal.purchase_item(buyer, name, card)` | `purchaseItem : VirtualTerminal -> Avatar -> Text -> VirtuCard -> Virt (Option VirtualItem)` | The first listing with the name: debit the price (a `purchase` entry), remove the listing, add the item to the buyer inventory. A listing with no item makes a new `VirtualItem` (name, the terminal category as the type, the price as the base value). Else `none` and no change. | O21 |
| 22 | `terminal.purchase(name, card)` | `purchase : VirtualTerminal -> Text -> VirtuCard -> Virt (Option VirtualItem)` | Row 21 with the card owner as the buyer. | |
| 23 | `VirtualItem(name, type, base_value)` | `item : Text -> Text -> Credits -> Virt VirtualItem` | Makes an item, `common`, no attributes. | |
| 24 | `item.set_rarity(level)` | `setRarity : VirtualItem -> Rarity -> Virt Unit` | Sets the rarity. | |
| 25 | `item.calculate_value()` | `itemValue : VirtualItem -> Virt Credits` | Read: `base_value * multiplier rarity`. | O14 |
| 26 | `item.set_attribute(key, value)` | `itemSet : VirtualItem -> Text -> Value -> Virt Unit` | Sets or replaces the key. | |
| 27 | `item.get_attribute(key)` | `itemGet : VirtualItem -> Text -> Virt (Option Value)` | Read. | |

The source syntax has no methods and no overloads. `type` is a field name
in the JSON document, and `kind` is its name in the source (O22).

### 5.2 What the request does not give

No delete of an instance, no change of a name, a role, a category or a base
value, no refund, no stock count, and no other base type. A card gets
credits only from rows 8, 11, 13 and 16. The credits of a purchase leave
the world (a terminal has no card).

### 5.3 Laws (1-cells of `Virt`, S6 and S6R)

Proposal (O23 (a)): these laws are 1-cells of `Virt`. The runner gives a
path for each one. S6 adds only L5 as 1-cells (S6 kickoff K3 (a)). L1 to
L4 are laws at known references: S6 has closed-world rows for them, and
the slice S6R gives their proofs at a variable world (section 7).

- L1 `setSecurity c l >> setSecurity c l' = setSecurity c l'`.
- L2 `setRarity i r >> setRarity i r' = setRarity i r'`.
- L3 `avatarSet a k v >> avatarSet a k v' = avatarSet a k v'`, and the
  same for `itemSet`.
- L4 `avatarSet a k v >> avatarGet a k = avatarSet a k v >> pure (some v)`,
  and the same for `itemSet`. As a 1-cell, L4 has the continuation form
  `avatarSet a k v (avatarGet a k t) = avatarSet a k v (t (some v))` for
  `t : Option Value -> X`. A boundary cannot contain `pure`: a boundary
  slot has the carrier type `X`, thus `pure` has no declared type there,
  and `check` gives TYPE_INFER (test/check/law-pure-boundary.lang).
- L5 Two reads give the same value: `r >>= \x. r >>= \y. f x y =
  r >>= \x. f x x` for each read row. As a 1-cell, L5 has the Prod form
  `r a (fun x => r a (fun y => k (pair x y))) = r a (fun x => k (pair x x))`
  for `k : Prod R R -> X`. A curried `k : R -> R -> X` gives CELL_POSITIVE
  (test/check/law-curried.lang).

The laws hold at a known reference. At an unknown reference, the two
sides refuse at a closed world (EVAL_REF_UNKNOWN). In S6, at a variable
world, they were two different `refUnknown` neutrals. In S6R, R3
(section 3) moves a `refUnknown` to the result type of its eliminator
and keeps its id. Thus at the empty world and a variable card, the two
sides of L1 for `setSecurity` are convertible (the path regression
`reference: setSecurity twice at a card of the empty world (S6R K3)`).

In S6, `Virt` has 11 1-cells: one L5 law for each read row of 5.1
(`rrName` to `rrIGet`, in the row order, after the 28 0-cells and before
`pure`). A read does not change the world, thus each path case of `run`
is `refl`. L1 to L4 are not 1-cells in S6. They have path regressions at
closed worlds. A proof at a variable world needs a world lemma, and the
lemma needs the routes R1 to R4 of the slice S6R: R1 a `List` eliminator
or `W` as a list cell; R2 a Bool compare in the library or a rule for
`natEq n n`; R3 the propagation of `refUnknown` in the kernel; R4 a rule
for `textEq t t`.

Sources (C22, C23). L1 to L5 are 3 of the 7 axiom schemas of global state
in G. Plotkin and J. Power, "Notions of computation determine monads"
(FoSSaCS 2002, section 3). A location of `Virt` is a reference and a
field, or a reference and a key. L1 to L3 are schema 3 (two updates at
one location: the second update wins). L4 is schema 4 (an update and then
a lookup at the same location give the new value). L5 is schema 2 (two
lookups at one location are one lookup). 5.3 does not have schema 1 (a
lookup and then an update with the value found do not change the store)
or the schemas 5 to 7 (operations at two different locations commute).
The completeness result of the source is for the monad of a store `V^L`.
`Virt` is not that monad: an unknown reference refuses, and `credit`
writes a history entry. Thus 5.3 does not say that L1 to L5 are all the
laws of `Virt`. G. Plotkin and M. Pretnar, "Handling Algebraic Effects"
(LMCS 9(4:23), 2013, section 1.3): a handler is correct only when its
interpretation satisfies the equations of the theory, and the source
leaves this check to the programmer. In virtu-lang, the kernel does this
check: a fold over a cell with a 1-cell needs a path case, and `check`
compares the two ends of the path case with the images of the two
boundary ends. Thus the path case of `run` is the proof that the runner
satisfies the law.

`credit c m >> credit c n` is not `credit c (m + n)`: the history has two
entries. Thus no law joins two changes of a balance.

In S6R, `Virt` gets 6 more 1-cells, after the 11 L5 1-cells and before
`pure` (S6R kickoff K6 (a)): `wwSec` (L1), `wwRar` (L2), `wwASet` and
`wwISet` (L3), `wrASet` and `wrISet` (L4). Thus `Virt` has 46
constructors: 28 0-cells, 17 1-cells and `pure`. The routes of S6R are
R1 as K1 (a) (the world `W := WorldC Unit` and `Attributes` are list
cells), R2 as K2 (a) (a Bool cell in the library), R3 as K3 (a) and R4 as
K4 (a) (section 3). The path case of `run` for each new 1-cell applies a
world lemma at a variable world: `secLaw` (L1), `rarLaw` (L2), `asetLaw`
and `isetLaw` (L3), `agetLaw` and `igetLaw` (L4). The world lemmas split by `elim` on
the Bool cell. `attrSet` is restated with the one fold `attrSetF` over
the `AttrC` cell (Q-S6R-s9-1 (i)). Thus O23 (a) is complete: L1 to L5 are
1-cells of `Virt`, and the path case of `run` for each one is a proof at
a variable world.

## 6. The JSON output

`langc build PROG [-o OUT]` checks `PROG`, finds `def main : Virt Unit`,
runs it from the empty world (2.5) and writes one JSON document:

```json
{"virtu-lang":1,"instances":[
 {"id":0,"class":"Avatar","state":{"name":"Ada","role":"explorer","virtucard":1,"attributes":{"guild":"north"},"inventory":[3]}},
 {"id":1,"class":"VirtuCard","state":{"owner":0,"balance":80,"security_level":"high","history":[{"kind":"create","amount":100,"balance":100},{"kind":"purchase","amount":20,"balance":80}]}},
 {"id":2,"class":"VirtualTerminal","state":{"name":"Bazaar","category":"tools","listings":[]}},
 {"id":3,"class":"VirtualItem","state":{"name":"Lamp","type":"tools","base_value":20,"rarity":"common","attributes":{}}}
]}
```

- `instances` holds one object for each instance that the run made, in the
  order of the run. `id` is that position. A reference is an `id`
  (O24 (a)).
- `state` is the final state (section 4). An `attributes` object keeps the
  order in which the keys were first set. This order is in the text only:
  RFC 8259 (section 4) notes that some JSON parsers do not show the member
  order to the caller (C21).
- A `Credits` value is a JSON number with all its digits, up to 2^64 - 1.
  RFC 8259 (section 6) notes that a number above 2^53 - 1 is not
  interoperable: a parser that uses IEEE 754 binary64 numbers can change
  it (C21).
- A run time refusal (a `Credits` overflow, the fuel limit, the arena
  limit) writes nothing and exits 1.
- The kit rule "an instance is a top level definition with a data type" is
  replaced (O24).

There is no IO, no network and no console program.

## 7. Slices and gates

| Slice | Contents |
|---|---|
| S0 | This seed: the kit, the design, this plan. |
| S1 | The agent-lang kernel port (section 3), with the removals. The claim checks of 2.2 and 2.3. |
| S2 | `cell` declarations with 0-cells: `pure`, `bind`, `map`, `fold`; `Option`, `Either`, `Free`, `List`, `Nat` and `Fin k` as library cell monads; the monad laws as paths; refusal of `data` and `family`. |
| S3 | n-cells: path and square cells, `Trunc1`, `Trunc0`, `Quot`, one localization example, `State S`; `bind` and `fold` through each cell. |
| S4 | `cocell` declarations: `extract`, `extend`, `duplicate`, guarded `unfold`, lazy `out`; `Env`, `Stream`, `Cofree`, `Store`; the comonad laws as paths; O5. |
| S5 | The base span: the carriers of section 4, the four classes, `Virt` with the 27 operations as 0-cells, the world, the runner and the JSON output of section 6. |
| S6 | L5 of 5.3 as 11 1-cells of `Virt`, with `refl` runner paths; L1 to L4 as closed-world rows. |
| S6R | The routes R1 to R4 of 5.3, each with a probe before code: the world lemmas, the proofs of L1 to L4 at a variable world, and L1 to L4 as 1-cells of `Virt` with their runner paths. Then O23 (a) is complete. S7 can come after S6R or in parallel with it. Built: R1 to R4 as K1 to K4 (a), 6 1-cells (46 constructors), the mutants X1 to X6 killed, O23 (a) complete. |
| S7 | Examples (a marketplace with two avatars, two terminals and a transfer), mutants, docs. Close. |

Each slice ends with `make check` GREEN, guest test files for each new
rule and refusal, and a mutant table (each mutant killed or recorded).
Each slice after S0 is a USER commit.

## 8. Open items

Each item gives the proposal first.

- O1 Name: virtu-lang. Rename before S1 if the USER prefers another name.
- O2 Kernel home: this repository. lang-template stays without a cubical
  kernel.
- O3 Ambient structure: (a) `Sigma`, `Pi`, `Path` and `U` are usable, but
  no type is declared with them; (b) also refuse a user `Sigma` and `Pi`
  type, and give records only through cocells.
- O4 Cell monad: (a) the free algebra monad of a cell signature (0-cells
  and n-cells), as a HIT (2.2); (b) only the idempotent case
  (localizations); (c) only signatures with 0-cells (free monads on
  containers).
- O5 Higher cocells: (a) 0-cocells, and a higher cocell only as `P * -`;
  (b) 0-cocells only.
- O6 Laws: (a) paths, with no proof irrelevance; (b) conversions.
- O7 Objects: (a) `Virt`, the world `Store W` and the runner (2.5);
  (b) instances as values with no world.
- O8 The kit `data` and `family` forms: (a) refuse them in a program and
  keep them in the kernel; (b) keep them.
- O9 Carriers: (a) each carrier only as far as a row needs it (section 4);
  (b) no carrier operation beyond literals and equality.
- O10 `Text`: (a) UTF-8 literals and `textEq` only; (b) also append.
- O11 `Credits`: (a) a natural number of the smallest unit, 64 bits, a
  refusal at overflow; (b) a fixed point number with two decimals.
- O12 `Role`: (a) `Text` with the constants `explorer` and `merchant`;
  (b) a closed enumeration of the two.
- O13 The level of a new card: (a) `low`; (b) an argument of row 8.
- O14 Rarity multipliers (the images give none): (a) 1, 2, 4, 8, 16;
  (b) 1, 2, 3, 4, 5; (c) a USER table.
- O15 `Value`: (a) `Text`; (b) `Either Text Credits`.
- O16 History: (a) every change of a balance adds one entry, with kind,
  amount and balance after; (b) only `debit`, `credit` and `transfer`.
- O17 Listings: (a) a listing from row 19 keeps the item and the price at
  the time of listing; (b) the price at the time of purchase.
- O18 Card attach: (a) row 8 does not attach; `attachCard` is the only
  write of `avatar.virtucard`, and it needs the card owner to be the
  avatar; (b) row 8 also attaches when the avatar has no card.
- O19 `tap_denominator`: (a) add `amount` as bonus credits with a `bonus`
  entry; (b) the USER gives another meaning (the name suggests a divisor).
- O20 Not enough credits: (a) `none` and no change; (b) a run time refusal
  of the whole build.
- O21 Purchase: (a) the first listing with the name; remove it; a listing
  with no item makes a new `VirtualItem`; (b) the listing stays (no stock
  limit).
- O22 Source names: (a) the camel case names of 5.1, `kind` for the item
  type; (b) the image names with underscores.
- O23 Laws of `Virt`: (a) L1 to L5 as 1-cells in S6; (b) no 1-cells in
  `Virt`. S6 kickoff Q2 (a): S6 has the L5 1-cells only, and S6R adds L1
  to L4 (done: O23 (a) is complete, 5.3).
- O24 Output: (a) the instances of the run of `main`, references as `id`;
  (b) also the kit instances (top level definitions with a data type).
