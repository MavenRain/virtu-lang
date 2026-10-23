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
theory", 2020). The non-idempotent cell monads are the free algebra monads
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
| `State S X` | 0-cells `get` and `put`, 1-cells for the laws of state, with `Trunc0` | the program monad (2.5) |

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
`duplicate` come from `unfold`. The comonad laws are paths.

| Comonad | Cocells | Dual of |
|---|---|---|
| `Env E X = E * X` | one 0-cocell `ask : D X -> E` | `Either E` |
| `Stream X` | one 0-cocell `next : D X -> D X` | `Nat` (the free monad on the identity) |
| `Cofree (A, B) X` | one 0-cocell `obs : D X -> (a : A) * (B a -> D X)` | `Free (A, B)` |
| `Store S X = S * (S -> X)` | the costate cocells `pos` and `peek` | `State S` |

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

- The program monad `Virt` is the cell monad whose 0-cells are the 27
  operations of section 5. Its 1-cells are the laws of section 5.3 (S6). A
  program is `def main : Virt Unit`.
- The world `W` is the state of all instances: for each class, the list of
  its instances in the order the program makes them. The world comonad is
  `Store W`, the dual of `State W` (2.3). An instance is a coalgebra of its
  class interface (objects as coalgebras, Jacobs).
- The runner is the eliminator of `Virt` into the `Store W` carrier
  `W -> W * X`: a monad and comonad interaction law (Katsumata, Rivas and
  Uustalu, 2020). Its case for a 0-cell runs the operation on the world. Its
  case for a 1-cell is a path that shows that the law holds in the world.
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
write) and row 19 (the overload of row 18). A read of a field is a 0-cell,
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

### 5.3 Laws (1-cells of `Virt`, S6)

Proposal (O23 (a)): these laws are 1-cells of `Virt`. The runner gives a
path for each one.

- L1 `setSecurity c l >> setSecurity c l' = setSecurity c l'`.
- L2 `setRarity i r >> setRarity i r' = setRarity i r'`.
- L3 `avatarSet a k v >> avatarSet a k v' = avatarSet a k v'`, and the
  same for `itemSet`.
- L4 `avatarSet a k v >> avatarGet a k = avatarSet a k v >> pure (some v)`,
  and the same for `itemSet`.
- L5 Two reads give the same value: `r >>= \x. r >>= \y. f x y =
  r >>= \x. f x x` for each read row.

`credit c m >> credit c n` is not `credit c (m + n)`: the history has two
entries. Thus no law joins two changes of a balance.

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
  order in which the keys were first set.
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
| S6 | The laws of 5.3 as 1-cells of `Virt`, with the runner paths. |
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
  `Virt`.
- O24 Output: (a) the instances of the run of `main`, references as `id`;
  (b) also the kit instances (top level definitions with a data type).
