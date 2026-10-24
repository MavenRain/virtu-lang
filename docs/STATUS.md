# virtu-lang status

> Write here: one paragraph. Say which milestone the build implements, and
> which milestone it does not complete yet.

## Implemented

> Write here: one item for each feature that works. Name the former (F1 to
> F15), the core type or the core operation.

- <feature>.

## Remaining M0 work

> Write here: one numbered item for each missing feature. Say what each item
> needs first.

1. <work item>.

## Known limits

> Write here: one item for each limit of the current build, with its cause
> (a fuel budget, a host limit, a stack size).

- <limit>.

## Internal boundaries

> Write here: one paragraph for each module group. Say which module holds
> which job, which tests check which tables, and which types a program
> cannot use.

## S1

S1 ports the kernel of agent-lang 06854bf (A in this section) into virtu-lang. Then it removes the features that the request does not give (PLAN 3; kickoff rulings K1 to K12, all (a)). The port is a copy of the A files, then the removals below, then the verb change `langc json` to `langc build`. Big Nat stays (K1). The `Fin` builtin stays (K4).

Kernel fixtures (K5): the families in domain/domain.lang that come from A (S1, Vec, Charter, Ratified, Mealy and the other A families) are kernel fixtures. They are not base types. The base types are the four image classes (PLAN 2). S5 moves the fixtures that are not cell monads, duals or carriers to a test-only domain file.

### Gate

`make -B -j 2 check` exits 0.

| gate line | A 06854bf | S1 |
|---|---|---|
| examples | 36 | 27 |
| parse refusals | 14 | 13 (14 - 2 + 1) |
| check refusals | 88 | 78 (88 - 15 + 5) |
| evals | 200 | 99 (200 - 101) |
| json builds (A), build (S1) | 36 | 27 |
| json refusals (A), build refusals (S1) | 4 | 4 |
| regressions | 6 | 6 |
| printer regressions | 2 | 2 |
| path regressions | 301 | 148 |
| terminal environment | 9 | line removed |
| terminal | 31 | line removed |

### Removed features

The command `git -C ~/Documents/agent-lang show 06854bf:PATH` prints the A file PATH. Take the lines in the source column. All line numbers are A 06854bf lines.

| feature | A 06854bf source (file: lines) | restore |
|---|---|---|
| `Text` type and its 6 operations: textAppend, textEq, textSlice, textLength, textUtf8, natShow | src/front/core.h: 22, 80-85, 113, 165, 360; src/front/check.c: 102, 119, 178-183, 371-378 (text_type, int64_type), 2374-2392 (rule_text), 3156-3164 (core_text); src/front/eval.c: 1634-1635, 1663-1744 (text_copy to nat_show) | `git -C ~/Documents/agent-lang show 06854bf:src/front/core.h`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/check.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/eval.c` |
| `Int64` type and its 10 operations: int64Add, int64Sub, int64Mul, int64Neg, int64Div, int64Rem, int64Eq, int64Le, int64ToNat, natToInt64 | src/front/core.h: 23, 86-95, 166, 248, 361; src/front/check.c: 103, 120, 184-193, 2393-2404 (rule_int64), 3178-3181 and 3202-3212 (the Int64 literal hint in elab_nat); src/front/eval.c: 1637, 1745-1798 (int64_arith to int64_convert) | `git -C ~/Documents/agent-lang show 06854bf:src/front/core.h`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/check.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/eval.c` |
| Text and Int64 values (VAL_TEXT, VAL_INT64) in the evaluator, the checker output and the JSON output | src/front/eval.c: 14, 35, 41, 57, 124-139, 584-585, 702-703, 1632-1662 (the argument kinds, prim_args), 2287-2288, 2372-2389, 2448-2449, 2646, 2662, 2780-2781, 3032-3033, 3351-3352, 3434-3461, 3531-3533, 3654-3657, 3889-3906, 4084-4085, 4221-4294, 4342-4343; src/front/check.c: 2633-2638 (the rule dispatch), 3957-3964, 4009-4010; src/json.c: 47-48, 70-85, 109-110, 248-258, 318-335, 401, 725-726, 768-769 | `git -C ~/Documents/agent-lang show 06854bf:src/front/eval.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/check.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/json.c` |
| `Json` families (Json, JsonList, JsonFields) and the operations jsonParse, jsonRender | domain/domain.lang: 46-51; src/front/jsontext.c (648 lines) and src/front/jsontext.h (26 lines): whole files; src/front/core.h: 96-97; src/front/check.c: 8, 104, 194-195, 2405-2428 (json_type, rule_json); src/front/eval.c: 5, 1799-1811 (json_op); src/json.c: 7 | `git -C ~/Documents/agent-lang show 06854bf:domain/domain.lang`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/jsontext.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/jsontext.h` |
| `Log` family and logLength | domain/domain.lang: 52-57 | `git -C ~/Documents/agent-lang show 06854bf:domain/domain.lang` |
| `IO` cofamily (IOF, IO), ioPure, ioBind, ioMap, ioLoop and the 13 operations: readLine, write, args, listDir, find, readFile, search, diskUsage, makeDir, move, copy, trash, ollama | domain/domain.lang: 67-141; src/front/check.c: 569-665 (io_of, io_type, io_small_type, io_def), 666-672 and 719 (carried, carried_arg), 931-936, 956-961, 982-991 (the IO branches of pure, map, bind); src/front/core.h: 400-403; src/front/eval.c: 3337-3344 (eval_out, eval_apply) | `git -C ~/Documents/agent-lang show 06854bf:domain/domain.lang`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/check.c` |
| Filesystem driver (listDir, find, readFile, search, diskUsage, makeDir, move, copy, trash) | src/runtime/driver.c: 260-1093 (fs_ok to trash_path) | `git -C ~/Documents/agent-lang show 06854bf:src/runtime/driver.c` |
| Ollama driver and its mock | src/runtime/driver.c: 1094-1458 (ollama_endpoint to ollama_call); test/mock-ollama.c (347 lines): whole file | `git -C ~/Documents/agent-lang show 06854bf:src/runtime/driver.c`<br>`git -C ~/Documents/agent-lang show 06854bf:test/mock-ollama.c` |
| Console program: its main and the IO driver loop | src/runtime/rt_main.c (67 lines) and src/runtime/driver.h (54 lines): whole files; src/runtime/driver.c: 1-259 (readLine, write, args, the Json makers), 1459-1561 (io_run_init, io_step, io_run); gen/embed.c: the `-r` runtime embed mode (72 lines; K10, V keeps the seed file); Makefile: 15-16, 26-34 (build/runtime.c, build/runtime.o) | `git -C ~/Documents/agent-lang show 06854bf:src/runtime/rt_main.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/runtime/driver.c`<br>`git -C ~/Documents/agent-lang show 06854bf:gen/embed.c` |
| libtcc in langc (the console build verb) | src/build.c (65 lines) and src/build.h (13 lines): whole files; src/main.c: 7, 17, 36, 43, 66-73, 92-93, 136-137; Makefile: 11-14, 36-39; test/regressions.js: 39, 52-67 (S1: the V `tcc()` with big.c) | `git -C ~/Documents/agent-lang show 06854bf:src/build.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/main.c`<br>`git -C ~/Documents/agent-lang show 06854bf:Makefile` |
| `json` verb (S1: `build` with the V seed wording; `langc json` exits 2 with the usage) | src/main.c: 16, 26, 35, 42, 57-64, 90-91, 138 (V keeps the 454f9e0 file); test/gate.sh: 97-133 (S1 keeps the A message at line 44); test/regressions.js: 29; test/printer-regressions.js: 25 | `git -C ~/Documents/agent-lang show 06854bf:src/main.c`<br>`git -C ~/Documents/agent-lang show 06854bf:test/gate.sh` |
| String literals (double-quoted text and its escapes) | src/front/lexer.c: 39, 101-102, 160-238 (lex_str), 300; src/front/lexer.h: 11, 44-45; src/front/ast.h: 11 (TERM_TEXT); src/front/parser.c: 135, 350-351, 373, 597; src/front/check.c: 3231-3233 | `git -C ~/Documents/agent-lang show 06854bf:src/front/lexer.c`<br>`git -C ~/Documents/agent-lang show 06854bf:src/front/parser.c` |
| The 9 examples: chat, fs, int64, io, json, log, ollama, organize, text | examples/NAME.lang and test/json/NAME.json: whole files; examples/nat.lang: 3-4 (the natShow sentence), 12 (bigShown); examples/inventory.lang: 1-6 (S1: one new header line); examples/laws.lang: 16 (logLaw); domain/README.md: 10-15, 24, 31 | `git -C ~/Documents/agent-lang show 06854bf:examples/NAME.lang`<br>`git -C ~/Documents/agent-lang show 06854bf:test/json/NAME.json` |
| Removed tests, by count | check refusals 15: test/check/ io-bind-carrier, io-find-arity, io-listdir-nat, io-makedir-bind, io-move-nat, io-ollama-bind, io-ollama-nat-path, io-pure-type, io-readfile-bind, io-write-arg, int64-literal-range, json-entry-type, log-entry-type, text-literal-type, text-slice-arity (expect.txt 68-72, 79-88); parse refusals 2: test/parse/ text-escape, text-unterminated (expect.txt 12-13); eval rows 101: test/eval/expect.txt (text 17, int64 14, json 10, log 10, io 22, fs 7, ollama 4, organize 12, chat 4, `nat bigShown` 1); json goldens 9: test/json/; path regression cases 153 (301 to 148): test/path-regressions.c 16, 18, 121-1040, 1335-1362, 1838, 1872-1891, 1993-2070 and the Text, Int64 and JSON number rows; terminal 31 and terminal environment 9: test/terminal.c (460 lines), test/terminal-env.c (129 lines), test/term/ (99 files); Makefile: 44-48, 50-51, 58-59; test/check/domain-name.lang: 1 (S1: `boolIf`, not `logLength`) | `git -C ~/Documents/agent-lang show 06854bf:test/check/NAME.lang`<br>`git -C ~/Documents/agent-lang show 06854bf:test/eval/expect.txt`<br>`git -C ~/Documents/agent-lang show 06854bf:test/path-regressions.c`<br>`git -C ~/Documents/agent-lang show 06854bf:test/terminal.c` |

### Added tests (K7)

- test/check: text-removed.lang, int64-removed.lang, json-removed.lang, log-removed.lang, io-removed.lang. Each uses a removed name and gives NAME_UNKNOWN.
- test/parse/text-literal.lang: a double-quoted literal gives LEX_CHAR.
- test/gate.sh: `langc json PROG` exits 2 with the usage.

### Deviation

The first copy of src/json.c from A set LANG_NAME to "agent-lang". The V seed sets "virtu-lang". The S1 build found this and restored "virtu-lang" in src/json.c, in 2 comments (src/front/check.c, src/front/eval.c) and in the 27 goldens in test/json/.

### Mutants (K9)

The runner ~/Documents/virtu-lang-work/S1-mutants.zsh (outside the repo) applies each mutant, runs `make -B -j 2` on the target, runs the killers and restores the file from the index. Before the mutants and after the restore, each killer passes. Lines are V lines. Result: 10 of 10 KILLED.

| id | kickoff | site | mutation | killer | result |
|---|---|---|---|---|---|
| M1 | f1 | src/front/check.c:2310 rule_fin_case | the arm count test `!=` to `<` | test/check/fin-case-arity.lang (FIN_CASE) | KILLED |
| M2 | f2 | src/front/check.c:1887 hcomp_cap | skip the cap test (`0 &&`) | test/check/hcomp-cap.lang, hcomp-universe-cap.lang (HCOMP_CAP) | KILLED |
| M3 | f3 | src/front/check.c:1862 hcomp_overlap | skip the overlap test (`0 &&`) | test/check/system-overlap.lang (SYSTEM_OVERLAP) | KILLED |
| M4 | f4 | src/front/check.c:2112 glue_boundary | accept a mismatch (`0 &&`) | test/check/glue-boundary.lang (GLUE_BOUNDARY) | KILLED |
| M5 | f5 | src/front/check.c:2795 check_plam | skip the path end check (`0 &&`) | test/check/elim-path-case.lang, fold-path-boundary.lang (PATH_END) | KILLED |
| M6 | f6 | src/front/check.c:3521 codata_positive | accept a negative occurrence (`0 &&`) | test/path-regressions.c row "cofamily: X in the domain of a Pi field" (CODATA_POSITIVE) | KILLED |
| M7 | f7 | src/front/check.c:1554 rule_counfold | infer the step, not check it at `S -> F ps S` (the guard) | test/check/codata-step.lang (TYPE_MISMATCH) | KILLED |
| M8 | f8 | src/front/eval.c:1386 add_mod | modulus k to k + 1 | eval `fin mulAt => fun _ => 10` | KILLED |
| M9 | f9 | src/front/eval.c:1736 out_field | give the seed, not the unfold of the seed | eval `stream thirdNat => 2` | KILLED |
| M10 | K9 | src/front/check.c:115 BUILTINS | put the `Text` row back after the Nat row (OP_NAT, because S1 removes OP_TEXT) | test/check/text-removed.lang (NAME_UNKNOWN) | KILLED |

Changes from the kickoff list (a read-only survey found them, with a build of each form):

- M1: `>` survives (fin-case-arity gives 2 arms to `finCase 3`, and 3 > 2 still refuses). `<` is the kept form.
- M5: check_boundary checks only the path constructors of domain/domain.lang, and a program that declares a family gets REFUSE_DATA. The PATH_END of both killers comes from path_ends, which check_plam calls.
- M6: no guest test reaches codata_positive, because a program that declares a cofamily gets REFUSE_DATA. The killer is a C runner row.
- M7: rule_unfold gives the cofamily form to rule_counfold, which holds the guard.
- Named killers that do not kill: fin-all-case (M1; its FIN_CASE comes from a PATH_END in one case), hcomp-higher-universe (M2; rule_hcomp gives HCOMP_UNIVERSE), codata-program (M7; a declaration refusal).
- K9 (a) does not include f10 (path_apply, hcomp_true).

## S2

S2 adds the `cell` declaration to the kernel and a library of cell monads in domain/domain.lang. The kickoff rulings K1 to K12 are all (a) (docs/PLAN.md, USER ruling 2026-10-22). A cell declares its operations over the carrier X. The checker makes a family from a cell, with the implicit `pure` as the last constructor (K3). The rules `pure`, `bind`, `map` and `fold` accept a cell (K4). hcomp at a uniform family is structural (K7).

Ruling Q-S2-s5-1 (a): only FinC has a path to its builtin type (`finPath`, by ua). The library has the maps in the two directions for each cell, and the round trips that pass. The ua paths for OptionC, Either, ListC and NatC are an open point for a later kernel slice. Findings F1 to F3 give the cause.

### Gate

`make -B -j 2 check` exits 0 (~/Documents/virtu-lang-work/s8-gate2.log).

| gate line | S1 | K12 | S2 | added in S2 |
|---|---|---|---|---|
| examples | 27 | 32 | 32 | examples/cells.lang, cell-laws.lang, fin-cell.lang, free.lang, state-free.lang |
| parse refusals | 13 | 14 | 14 | test/parse/cell-no-ops.lang |
| check refusals | 78 | 86 | 86 | 8 files in test/check (D8) |
| evals | 99 | about 123 | 128 | 29 rows in test/eval/expect.txt (D5, D10) |
| build | 27 | 32 | 32 | test/json/cells.json, cell-laws.json, fin-cell.json, free.json, state-free.json |
| build refusals | 4 | 4 | 5 | test/emit/cell-function.lang (JSON_VALUE, D7) |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 148 | 148 | 158 | 4 cell source round trips and 6 continuation boundary checks |

### Kernel changes

- src/front/lexer.c, lexer.h: the `cell` keyword (TOK_KW_CELL).
- src/front/parser.c, ast.h: parse_cell and DECL_CELL.
- src/front/core.h: CARRIER_CELL, CLOS_FOLD_CONT, the `continuation` flag of a field and the `cell` flag of a family.
- src/front/check.c: check_decl admits a program cell (K2). check_cell makes the family, with `pure` last (K3). cell_fields, cell_field_type and is_carrier check the operations. The checker gives `pure`, `bind` and `map` at a cell (K4). check_field, own_family, ih_type and case_fields accept a continuation field.
- src/front/eval.c: fold_family and elim_family accept a continuation field. hcomp_data does not make hcomp at a family stuck (K7).
- Staged review: fold_image maps continuation fields and their pointwise induction hypotheses in HIT boundaries. The value printer emits cell pure values using `pure` under a typed identity, retaining the cell type in inference contexts.
- domain/domain.lang: the library section (K5, K6): the cells Either (throw), Free (op), OptionC (noneC), ListC (consC), NatC (succC) and FinC (pickC); the maps toOption, ofOption, toSum, ofSum, toList, ofList, cellToNat, cellOfNat, toFin, ofFin; the round trips retOption, retSum, finSec, finRet; finPath.

### Added tests (K10)

- examples: cells.lang, cell-laws.lang, fin-cell.lang, free.lang, state-free.lang.
- test/check: cell-codomain.lang (CELL_CODOMAIN), cell-positive.lang (CELL_POSITIVE), cell-carrier.lang (CELL_CARRIER), cell-pure.lang (CELL_PURE), cell-duplicate.lang (CELL_DUPLICATE), cell-fold-missing.lang (TYPE_MISMATCH), cell-bind-mismatch.lang (TYPE_MISMATCH), family-and-program.lang (REFUSE_DATA).
- test/parse/cell-no-ops.lang (PARSE_EXPECT).
- test/emit/cell-function.lang (JSON_VALUE).
- test/json: 5 goldens. Each golden was read by hand.
- test/eval/expect.txt: 29 rows.
- test/path-regressions.c: 4 cell source round trips and 6 HIT continuation boundary checks, including refusal of unequal boundary ends.

### Deviations

- D1, a kernel bug, fixed. check_program gave m->ctors the size of the parsed ctor_count of each declaration. But check_cell adds the implicit `pure` constructor. The 6 domain cells wrote past the array into the next arena values. The domain def Bool became "a term of type " (empty), and entries, formers, inventory and monad gave TYPE_MISMATCH. The fix at src/front/check.c:3873 adds 1 for each DECL_CELL. The program probes did not show the bug (1 to 5 program cells).
- D2: `toNat` is a def in examples/dec.lang. Thus the NatC maps are cellToNat and cellOfNat.
- D3: examples/cell-laws.lang uses unitC helpers (`unitOption V a` is `pure a`) in the left unit law, because `bind` needs the type of its first argument. The form `bind (pure a) f` was not tried.
- D4, print forms: the staged review fixed the non-source form `OptionC.pure Nat 3`. Pure values now print as `pure 3` under a typed identity for their cell type. `throw Bool Nat true` prints as `throw (Fin 2) Nat 1`, because Bool unfolds. In the JSON types, Bool prints as `Fin 2`.
- D5: evals 127 at the end of chunk B (99 + 28), against about 123 in K12.
- D6: `langc build` refused free.lang and state-free.lang with JSON_VALUE (a function in the instance). A Pi-type def has no JSON form, so test/json/cell-laws.json has no instance.
- D7, the D6 fix: in free.lang and state-free.lang each Free or State value is a function of a seed (`tree d`, `grown d`, `tripled d`, `shaped w`, `prog d`, `after d`). The folds take the seed 1 (shapeOf: the shape 3). The 9 eval values did not change. free.json has total, totalMap, leaves and shapeOf. state-free.json has pairAt4. test/emit/cell-function.lang (a top-level Free value) gives JSON_VALUE. Thus build refusals is 5, not 4 (K12).
- D8: the refusal codes come from real runs. cell-fold-missing gives TYPE_MISMATCH, and its message names "Nat, List A or a domain family", not a cell (a finding). cell-bind-mismatch gives TYPE_MISMATCH, because bind needs a function into the cell `OptionC Nat`. family-and-program gives REFUSE_DATA at Ping, the first family. cell-no-ops gives PARSE_EXPECT.
- D9: the counts agree with K12, except evals (D5, D10) and build refusals (D7).
- D10: the mutant runner is outside the repo. It restores each file from a copy, not from the index, because the S2 changes were not in the index. A builder subagent for the mutant loop stopped at its first step on an API classifier, with no file change. The loop ran by hand. N10 was first killed only by test/json/monad.json, because cells ce uses the cell Either map (check.c cell_fold) and not the builtin Sum map. S2 build session 8 added `def sumMap` (map at the builtin Sum) to examples/cells.lang and the eval row `cells sumMap => inr 5` (evals 128). The N10 rerun fails that row.
- K6 gives the FinC operation the name `pick`. `pick` is a def in examples/entries.lang, so the operation is `pickC` (F5).

### Findings

- `NAME.pure` does not lex (LEX_CHAR 0x2e). Only the `pure` rule gives a pure value; the printer uses that rule under a typed identity.
- Library binder names clash across cells: `REFUSE_NAME: a: a is a domain field (rule R4)`.
- Staged review fixed fold_image (the boundary images of a HIT): a continuation is mapped pointwise, and an elim case receives the raw continuation followed by its induction hypothesis. Valid folds previously gave PATH_END, and valid elims could give INTERNAL.
- src/json.c tests only `recursive`. A value with a function field has no JSON form (D6).
- CARRIER_CELL is in the enum, but no core holds it. check.c gives `pure`, `bind` and `map` at check time (OP_CTOR of the last constructor; OP_FOLD_FAMILY with cell_rebuild cases and the pure case), because reduce_bind cannot rebuild a cell value. The meaning of K4 holds.
- The removal of the hcomp_data guard alone did not give the K7 structural hcomp. family_field_type adds it, for a uniform family only. A dependent field and an indexed family stay stuck.
- A bind at a neutral x stays a fold.
- F1: `option`, `either` and the Nat and List `fold` are non-dependent ("needs a function with a non-dependent result type"). `elim` takes only a domain family. Thus the builtin-side homotopy of isoToEquiv for Option, Sum, List and Nat has no proof. `sec : Path (Option V) (toOpt V (ofOpt V o)) o := refl` gives PATH_END.
- F2: Unit has no eta rule. `unitEta : (u : Unit) -> Path Unit u unit := refl` gives PATH_END. Thus the cell-side round trip of `ListC A Unit` and `NatC Unit` fails at the pure case.
- F3: the builtin Nat fold at `succ x` with a neutral x stays stuck. The Nat fold reduces only at a literal.
- F4, pass: the cell-side round trips of OptionC and Either by elim (retOption, retSum); Free with the dependent field `(opShape : A) -> (B opShape -> X) -> X` (`op` takes A, B, X, the shape and the continuation: 5 arguments); `FinC k Empty = Fin k`.
- F5, names: `pick` is a def in examples/entries.lang, so the FinC operation is `pickC`. The other operation names have no clash: `noneC`, `throw`, `consC`, `succC`, `op` with the field `opShape`. `get` and `put` are only in examples/state-free.lang.
- F6: the builtin fold takes the step first, then the base. A cell fold takes the operation cases, then the pure case.

### Mutants (K11)

The runner ~/Documents/virtu-lang-work/s7-probe/mutants/run.zsh (outside the repo) applies each mutant and runs `make -B -j 2 check`. Then it restores the file from a copy, compares the file with the copy and runs the gate again. The N10 rerun is ~/Documents/virtu-lang-work/s8-probe/n10.zsh. Each restore is equal to the copy, and each restore gate exits 0. Lines are V lines. Result: 10 of 10 KILLED.

| id | site | mutation | killer | result |
|---|---|---|---|---|
| N1 | src/front/check.c:3859 check_decl | a program cell gives REFUSE_DATA | examples/state-free.lang, the 5 CELL_* check refusals, 4 state-free eval rows | KILLED |
| N2 (M1) | src/front/check.c:3844 check_decl | skip the program family refusal (`&& 0`) | data.lang, family-and-program.lang (REFUSE_DATA) | KILLED |
| N3 | src/front/check.c:844 cell_pure | `pure` is the first constructor, not the last | 290 rows: INTERNAL wrong number of arguments at the domain def retOption | KILLED |
| N4 | src/front/check.c:953 rule_bind | the pure case of bind is the identity | eval cells cbo1, cbl; free total, leaves; state-free afterState, afterResult; cell-laws leftOption (PATH_END); 3 goldens | KILLED |
| N5 | src/front/eval.c:997 fold_family | give the raw continuation, not the fold of it | cell-laws rightFree (PATH_END); eval free total, totalMap, leaves; 5 state-free rows | KILLED |
| N6 | src/front/check.c:1041 ih_type | the induction type of a continuation field is the field type | cell-laws rightFree (TYPE_MISMATCH), cell-laws.json | KILLED |
| N7 | src/front/check.c:3733 cell_field_type | a Pi field with a codomain that is not X is a plain field | test/check/cell-positive.lang | KILLED |
| N8 | src/front/check.c:3752 cell_fields | skip the carrier test (`0 &&`) | test/check/cell-codomain.lang | KILLED |
| N9 | src/front/eval.c:2470 hcomp_data | put the family guard back in the stuck test | examples/fin-cell.lang (PATH_END), 4 fin-cell eval rows, fin-cell.json | KILLED |
| N10 (M3) | src/front/eval.c:809 the Sum map (CARRIER_SUM) | map at `inl`, not at `inr` | eval `cells sumMap => inr 5`, cells.json, monad.json | KILLED |

Named killers that do not kill:

- N1: examples/cells.lang uses only domain cells.
- N3: an INTERNAL arity error at the domain def retOption stops the gate before the cells values cbo1, cbl and cn.
- N4: cells cbe does not kill, because `be` is a throw and the pure case does not run.
- N6: free leaves does not kill.
- N10: cells ce does not kill (D10).

## S3

S3 adds n-cells to the `cell` declaration: path cells and square cells. The kickoff rulings K1 to K13 are all (a) (docs/PLAN.md, `### USER ruling (S3 kickoff)`, 2026-10-22). The claims C7 to C12 are in ~/Documents/virtu-lang-work/S3-claims.md (outside the repo).

### Gate

`make -B -j 2 check` exits 0 (~/Documents/virtu-lang-work/s3b-gate8.log).

| gate line | S2 | K12 | S3 | added in S3 |
|---|---|---|---|---|
| examples | 32 | 35 | 35 | examples/ncell.lang, state.lang, loc.lang |
| parse refusals | 14 | 15 | 15 | test/parse/cell-boundary-open.lang |
| check refusals | 86 | 93 | 93 | 7 files test/check/cell-*.lang |
| evals | 128 | about 140 | 145 | 17 rows in test/eval/expect.txt (D2) |
| build | 32 | 35 | 35 | test/json/ncell.json, state.json, loc.json |
| build refusals | 5 | 5 | 5 | none |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 158 | 164 | 164 | 4 cell boundary round trips, the P6 map, the P5 square face order |

Only the evals line differs from K12: 17 rows, not about 12 (D2).

### K rows

- K1: parse_cell parses a system after the type of an operation (src/front/parser.c:599).
- K2: a path operation is a path constructor of the cell family. cell_fields copies the boundary (src/front/check.c:3865). cell_rebuild adds the interval binders (check.c:867). cell_fold checks the type of each path case with path_case_type (rebuild_ok, check.c:897).
- K3: the library cells Trunc1C, Trunc0C and QuotC in domain/domain.lang, with the maps to and from the S1 families in examples/ncell.lang. cell_path_type changes a cell operation field `Path X a b` to `Path SELF a b` (open point 4).
- K4: no new set-target rule. The square case has the type isSet RES. test/check/cell-noset.lang (PATH_UIP) and cell-nosquare.lang (TYPE_MISMATCH) refuse a non-set target.
- K5: the domain cell StateC (getC, putC, the 4 law 1-cells GP, GG, PP, PG and the square) and examples/state.lang (run, inv, round trip 1, round trip 2 by its point cases). Open point 1 gives the gap.
- K6: examples/loc.lang, the nullification at `Fin 2` (cell Null2) in the form of Rijke, Shulman and Spitters 2020, Definition 2.14.
- K7: the dbc8e1a fold_image fix stays. P1 and P2 are KILLED.
- K8: the ua paths of OptionC, Either, ListC and NatC (S2 F1 to F3) are for a later kernel slice. No S3 change.
- K9: a fold with a missing case names the family, the number of cases and the first constructor with no case (TYPE_MISMATCH, no golden moved). `NAME.pure` stays LEX_CHAR. The direct form `bind (pure a) f` checks (open point 3).
- K10: the added tests below.
- K11: the mutants below.
- K12: the gate above.
- K13: docs/PLAN.md 2.2 cites Rijke, Shulman and Spitters 2020 (Definition 2.14, Theorem 2.18) next to the CORS citation, and the State row says "for sets `S` and `X`". PLAN section 1 records both.

### Claims (S3-claims.md)

- C7: `Trunc1` is a HIT. CONFIRMED (HoTT book 6.9). The cell twin is Trunc1C.
- C8: `Trunc0` is a HIT with a 2-dimensional constructor. CONFIRMED (6.9, 7.3). Trunc0C, QuotC and StateC use the 6.9 square. The 7.3 hub and spoke form is not in S3.
- C9: set quotients are a HIT. CONFIRMED (6.10). The cell twin is QuotC.
- C10: localization at a map is a HIT. CONFIRMED. The constructors are in Rijke, Shulman and Spitters 2020 (Definition 2.14, Theorem 2.18), not in Christensen, Opie, Rijke and Scoccola 2020.
- C11: a reflective localization is accessible exactly when it is the localization at a small set of maps. CONFIRMED (S1 kickoff C3).
- C12: the free model of the theory of state is `S -> S * X`. CONFIRMED for the monad (Plotkin and Power 2002). No source gives the CTT fold equivalence. examples/state.lang is the evidence, with the round trip 2 gap (open point 1).

### Kernel changes

- src/front/parser.c: parse_cell parses the system of an n-cell (K1).
- src/front/check.c: cell_fields, cell_rebuild, rebuild_ok and cell_fold (K2); cell_path_type (K3); the fold arity message (K9).
- domain/domain.lang: the cells Trunc1C, Trunc0C, QuotC and StateC.
- docs/PLAN.md: `### USER ruling (S3 kickoff)` and the K13 edits.

### Added tests (K10)

- examples: ncell.lang, state.lang, loc.lang.
- test/parse/cell-boundary-open.lang (PARSE_EXPECT).
- test/check: cell-path-end.lang (PATH_END), cell-noset.lang (PATH_UIP), cell-nosquare.lang (TYPE_MISMATCH), cell-curried.lang (CELL_POSITIVE), cell-pure-boundary.lang (TYPE_INFER, D1), cell-three-intervals.lang (HIT_BOUNDARY), cell-overlap.lang (SYSTEM_OVERLAP). The codes come from real runs.
- test/json: ncell.json, state.json, loc.json.
- test/eval/expect.txt: 17 rows: ncell e1Map, e1Bind, e1End, e1Back, e0Map, e0Face, e0Face2, eqTo, eqBind; state exAt4, afterAt4, gpAt, exFirst, sectAt, roundAt; loc nAt, nEnd.
- test/path-regressions.c: "Trunc1C squash path round-trips", "Trunc0C squash square round-trips", "QuotC relation path round-trips", "StateC law path with a continuation round-trips", "map through a path cell at a variable interval round-trips" (the P6 killer), "square fold through a neutral case keeps the face order" (the P5b killer).

### Deviations

- D1: test/check/cell-pure-boundary.lang gives TYPE_INFER "pure needs a declared type Option A, List A or Sum E A". The message does not name a cell.
- D2: evals is 145, not about 140 (K12): 16 rows in chunk B and the row `ncell e1Back` (D3).
- D3, a designed print, not a bug (finding F-s3-1): print_typed (src/front/eval.c:3950-3970, S2) prints a cell pure value as a typed identity `((fun (_case0 : C) => _case0) (pure v))`, so that the printed term checks again (`pure` needs a declared type). Each cell pure value prints in this form (`def u : Trunc1C Nat := unit1C Nat 5` too). The golden `stream nats` has the same form, and so has the t0p type string in test/json/ncell.json. The row is `ncell e1Back => ((fun (_case0 : Trunc1C Nat) => _case0) (pure 5))`.
- D4: P5 survives (Mutants).
- D5: round trip 2 of State as one elim is refused (open point 1).
- D6: transp at the path constructor gpC at a neutral interval stays stuck (open point 2).
- D7: P10 survives, except for the build kill (Mutants).
- D8: the mutant runner is outside the repo. It restores each file from the main checkout, because the S3 changes were not in the index (as S2 D10).

### Open points (S3-build-PROMPT.md)

1. Round trip 2 of State (`inv (run t) = t`) as one elim: REFUSED. The probe ~/Documents/virtu-lang-work/s4-op/op1.lang uses the motive `fun x => Path (StateC Nat Nat) (inv (run x)) x`, the cases getAt, putAt, retAt, `refl` for each of the 4 law cases and `refl` for the square. It gives TYPE_MISMATCH "rt2: refl needs a path type with a constant line, found PathP (fun _ => PathP (fun _x0 => StateC Nat Nat) ...". A law case is a PathP over the law line in a family of propositions. It needs isProp of the motive (from the square stSquashC) and a transport lemma. These are not built. The C12 gap stays: round trip 2 has only its point cases (retAt, getAt, putAt).
2. transp at a cell type with a continuation field: the point constructors compute. Along a line in S (ua at `Fin 3`, loopRot1), the transport of `getC (Fin 3) Nat (fun s => putC ... s (ret3 7))` gives `getC (Fin 3) Nat (fun x => putC (Fin 3) Nat (hcomp ... (finAdd 3 ... 2) ... 1) (pure 7))`. Along a line in X, the transport of `getC Nat (Fin 3) (fun s => retF 1)` gives `getC Nat (Fin 3) (fun x => pure 2)`. At the path constructor gpC at a neutral interval, `fun i => transp (...) i0 (gpC (Fin 3) Nat (ret3 7) i)` checks and stays a stuck transp (field_transport_boundary to transp_stuck). No wrong value. The stuck path case is a gap (D6).
3. `bind (pure a) f` in the direct form: RESOLVED. It checks and gives `pure 4` at Trunc1C and at StateC, the same value as the typed helper form.
4. A cell operation field of type `Path X x y`: RESOLVED by K3 (a). cell_path_type gives `Path SELF a b`. The probe pe-square gave CELL_POSITIVE before the change and passes now.
5. Null2 (K6), a boundary end `f b` and a continuation over S1: RESOLVED in examples/loc.lang.
6. The refusal codes: pure in a law boundary gives TYPE_INFER (D1); overlapping faces that disagree give SYSTEM_OVERLAP (cell-overlap.lang, the P3 killer). The square fold regression kills P5b, not P5 (D4).
7. The JSON form of the new examples: RESOLVED. ncell, state and loc build (json 35).

### Mutants (K11)

The runner ~/Documents/virtu-lang-work/S3-mutants.zsh (outside the repo) copies the main checkout to a scratch copy (rsync). It applies each mutant at a line that it finds by content, and runs `make -B -j 2 check`. When the gate stops before the path regressions, the runner builds build/path-regressions again and runs it. Then it restores the file from the main checkout and compares the two files with cmp. Output: ~/Documents/virtu-lang-work/s3c-mutants.out; logs: s3c-mutants/mlogs. Each restore is equal. The gate before the runs (s3b-gate8.log) and the final gate on the scratch copy (mlogs/final.log) exit 0 at the S3 counts, so each killer passes before and after. Lines are lines of the S3 work tree. Result: 8 of 10 KILLED; P5 and P10 SURVIVED. P5b and P10b are extra variants.

| id | site | mutation | killer | result |
|---|---|---|---|---|
| P1 | src/front/check.c:1236 fold_image | `else if (field->continuation)` to `else if (0)` | examples/state.lang, 6 state eval rows, state.json (INTERNAL); 3 path regressions ("fold boundary maps a point constructor continuation" and 2 elim boundary checks) | KILLED |
| P2 | src/front/check.c:1241 fold_image | `quote_value(m, level + 1u, image)` to `level` | examples/state.lang (PATH_END), examples/loc.lang (INTERNAL), 8 eval rows, loc.json; 2 path regressions | KILLED |
| P3 | src/front/check.c:3426 boundary_core | `if (0 && !system_overlaps(...))` | test/check/cell-overlap.lang (exit 0, not SYSTEM_OVERLAP) | KILLED (SURVIVED at the kickoff) |
| P4 | src/front/check.c:1329 path_case_type | `interval_count == 2u` to `3u` | 41 gate failures (examples dec, elim, ncell, quot, state and more); 3 path regressions | KILLED |
| P5 | src/front/eval.c:1058 fold_square | swap `inner[0]` and `inner[1]` | none | SURVIVED (D4) |
| P5b | src/front/eval.c:1058 fold_square | the interval order `s @ r1 @ r2` to `s @ r2 @ r1` | path regression "square fold through a neutral case keeps the face order"; examples/quot.lang (PATH_UIP), eval quot toTrunc3, quot.json | KILLED |
| P6 | src/front/check.c:867 cell_rebuild | drop the interval CORE_PLAM (`for (i = 0u; ...)`) | path regression "map through a path cell at a variable interval round-trips"; examples ncell and state (INTERNAL); 19 gate failures | KILLED |
| P7 | src/front/eval.c:997 fold_family | the path arm gives the raw field | build: check-clang `-Wunused-function` on fold_path_field; the direct run: 3 path regressions ("fold boundary maps a point constructor path field", "square fold through a neutral case keeps the face order", "block fold: Box, a sibling recursive field at its target") | KILLED |
| P8 | src/front/parser.c:599 parse_cell | skip the cell system (`ctor->boundary = NULL;`) | PARSE_EXPECT at domain/domain.lang:248 (the first domain cell with a system): 325 gate failures, 163 path regressions | KILLED |
| P9 | src/front/check.c:3865 cell_fields | `out->boundary = NULL;` | HIT_BOUNDARY "Trunc1C: the path constructor squash1C needs a boundary": 320 gate failures (examples/ncell.lang too), 159 path regressions | KILLED |
| P10 | src/front/check.c:926 cell_fold | drop the rebuild_ok call | build only: check-clang `-Wunused-parameter` (`st`, check.c:917); the direct run: 164 path regressions pass | SURVIVED (D7) |
| P10b | src/front/check.c:903 rebuild_ok | `if (1)`: each case passes | none: the gate exits 0 | SURVIVED |

Survivors:

- P5: path_apply (src/front/eval.c:713) reads the given ends only at i0 or i1. fold_square runs only for a constructor value at neutral intervals. At an end, the cell boundary fires first. Thus the inner ends go into a VAL_PAPP (quote_papp keeps them in CORE_PAPP). Each interval substitution in the evaluator applies a closure again, and the boundary fires before the fold. conv (eval.c:3437) compares the path and the interval, not the ends, and the printer prints no ends. Only val_isubst (eval.c:2831, glue_at) puts an interval in a value with no new evaluation. No source program was found that takes it to a stuck square fold. P5 is a survivor, not a proved equivalent. e0Face, e0Face2 and the P5b regression do not kill it.
- P10 and P10b: rebuild_ok checks the type of the rebuilt path case `c a.. i`. cell_rebuild makes this case from the constructor, so its ends are the constructor boundary at the rebuilt fields, and the case type needs the same ends. No test makes rebuild_ok refuse; test/check/cell-path-end.lang refuses before cell_fold. The check stays as a guard. It is not a proved equivalent.

### Findings

- Field names are global (rule R4). Trunc0C cannot use the Trunc1C field names again, so each cell has its own names (sq1CFrom, sq0CFrom).
- Under P4, langc prints `(null): :`: path_case_type gives NULL with no diagnostic. Only the mutant gets to this path.

### Staged review fixes

- MEDIUM: a valid square boundary containing an earlier path constructor
  checked, but `map` and `bind` failed with HIT_BOUNDARY because fold_image
  accepted only point constructors. It now maps earlier path and square
  cases at their intervals, with mapped faces as path endpoints.
- Two C regressions round-trip map and bind through squares whose faces
  contain earlier paths and squares, while changing the carrier from Nat
  to Bool. Both fail with HIT_BOUNDARY before the fix. A refusal regression
  checks that a square case with an unequal path face still gives PATH_UIP.
- The review gate passes with 167 path regressions. The other S3 gate
  counts above stay the same. No test or CI check was removed or weakened.

## S4
The initial candidate and its gate evidence are recorded below. The current implementation includes the corrections in [Staged review corrections](#staged-review-corrections), especially the narrower conversion rule.



S4 adds the `cocell` declaration, the dual of `cell`. A cocell makes a layer family and a cofamily, with the comonad builtins `extract`, `extend` and `duplicate`. The kickoff rulings K1 to K12 are all (a) (docs/PLAN.md, `### USER ruling (S4 kickoff)`, 2026-10-23). A USER ruling of 2026-10-23 changes K2 for the name of the node (D1). A USER ruling of 2026-10-23 (R5 (a)) adds the slot guard (Kernel changes). The claims C13 to C17 are in ~/Documents/virtu-lang-work/S4-claims.md (outside the repo).

### Gate

`make -B -j 2 check` exits 0 (~/Documents/virtu-lang-work/s7b/gate-s11-close.log).

| gate line | S3 (pin) | K11 | S4 | difference and cause |
|---|---|---|---|---|
| examples | 35 | 36 | 36 | +1: examples/cocell.lang |
| parse refusals | 15 | 16 | 16 | +1: test/parse/cocell-no-ops.lang |
| check refusals | 93 | 101 | 101 | +8: 8 files in test/check (Added tests) |
| evals | 145 | about 155 | 158 | -1: K7 removes the algebra row `stop` (`down` keeps its golden with `take`); +14: the cocell rows |
| build | 35 | 36 | 36 | +1: test/json/cocell.json |
| build refusals | 5 | 5 | 5 | none |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 167 | 178 | 180 | -3: K7 removes the 3 rows of the kit `unfold` into `List`; +16 rows (Added tests) |

The S3 table gives 164 path regressions. The S3 review fixes add 3 rows, so the pin f43c4b7 has 167 (K11). Two lines differ from K11:

- evals: 158, not about 155. examples/cocell.lang has 14 eval rows, not about 12, and the algebra change is -1, not -2.
- path regressions: 180, not 178. The 2 extra rows are the D1 ruling row (a program `def nodeStreamK` gives REFUSE_NAME) and the R5 killer row (Mutants (K10)). K11 counted "transp and hcomp keep the rule off" as one row; that row is the hcomp accepts row (D4).

### K rows

- K1: `cocell` is a keyword (src/front/lexer.c, lexer.h). parse_cocell has the form of parse_cell (src/front/parser.c). A cocell with no op is PARSE_EXPECT.
- K2: check_cocell (src/front/check.c) makes the layer family and the cofamily. cocell_fields splits a Sigma observation into 2 fields. The builtins `extract` (1), `extend` (2) and `duplicate` (1) make the canonical node in cocell_unfold: one projection of `out s` for each field after `extract`. check_program reserves 2 family slots for each cocell. The node is the surface constructor `nodeD` (D1).
- K3: the type of the step is the guard. There is no new rule. test/check/unfold-kit.lang and unfold-no-hint.lang give TYPE_INFER.
- K4: domain/domain.lang has the cocells EnvK, StreamK and CofreeK, the layer StoreF and `cofamily Store S A := StoreF S A`. examples/cocell.lang gives Store by hand (posT, peekT, seekT, exT) and the maps between `StreamK X` and `Stream X`.
- K5: conv_counfold, conv_bisim, bisim_check and bisim_field (src/front/eval.c). A neutral converts with an unfold by eta. Two unfolds convert when the hypothesis (g1, g2, h) passes one check at a fresh variable. h is the identity when the seeds convert; else, with one seed a variable, h is the read back of the other seed with that variable abstracted. Each check spends 1 fuel. L3 and coassociativity at a variable are refl. The map form of coassociativity and test/check/codata-bisim.lang stay refused.
- K6: PropEnvK with the parameter pair `(P : Type 0) (pp : isProp P)` is in examples/cocell.lang at `P = Not Empty`. The idempotence is open point 5.
- K7: the kit `unfold` into `List` is removed (rule_unfold, OP_UNFOLD, json.c). examples/algebra.lang `down` uses `take`.
- K8: the c1 killer (`kids : Nat -> List X`, CODATA_POSITIVE) and the c6 killer (`cofamily Bad (A : Type 1) := BadF A`, CODATA_FORM) are path regressions. CODATA_FORM for a layer in a block is a path regression (D3). test/check/unfold-no-hint.lang is the fixture for `unfold g s` with no hint. The K8 kernel items: (1) the D1 message of test/check/cell-pure-boundary.lang names the cell: `langc: TYPE_INFER: PB: pure needs a declared type Option A, List A, Sum E A or a cell type C ps here` (PB is the cell of the fixture). No change; test/check/expect.txt holds only the code (line 91). (2) path_case_type gives HIT_BOUNDARY when an end of the boundary does not compute (Kernel changes). The evidence: P4 (src/front/check.c:1388, `interval_count == 2u` to `3u`) with this check off makes examples/dec.lang and ncell.lang print `langc: (null): :`. P4 with the check names the constructor: dec toNat squash0, elim toTruncE quotSquash, ncell to0 squash0C, quot map0 squash0, state run stSquashC. The killers for P5, P10 and P10b: open point 8.
- K9: the added tests below. There is no new node, Python or shell harness.
- K10: the mutants R1 to R9 (`### Mutants (K10)`): 9 of 9 KILLED. R5 survived the first run; the USER ruling R5 (a) adds the slot guard and the R5 row.
- K11: the gate above.
- K12: the PLAN 2.3 edits (the Stream dual `Nat * X`, the Store row, the law sources).

### Claims (S4-claims.md)

- C13: each comonad of PLAN 2.3 is the cofree comonad of its cocells. CONFIRMED for Env, Stream and Cofree, with a correction to the Stream dual (`Nat * X`, K12). NOT CONFIRMED for Store (C14).
- C14: the coalgebras of Store are the very well-behaved lenses. CONFIRMED. Store is the comonad of the costate co-operations with the lens laws as coequations. It is not the cofree comonad of the bare costate signature. Thus Store is a library comonad with a direct layer (K4).
- C15: an M-type in HoTT is the final coalgebra of a polynomial functor, with coinduction. CONFIRMED (S4-claims.md source A, Definition 4, Theorem 7, Theorem 18).
- C16: the coreflection `P * -`. CONFIRMED (S1 kickoff C2).
- C17: in Cubical Agda, a path in a coinductive type comes from copatterns and corecursion over an interval variable, and bisimilarity is equivalent to path equality. CONFIRMED. Guarded CTT gives this for guarded types only. Claim gap: K5 (a) makes bisimilarity a judgmental conversion. This needs a strict final coalgebra in the model. No source for the cubical model was read (open point 10).

### Kernel changes

- src/front/lexer.c, lexer.h, ast.h, parser.c: the `cocell` keyword, DECL_COCELL and parse_cocell (K1).
- src/front/check.c: check_cocell, cocell_fields, the BUILTINS rows `extract`, `extend` and `duplicate`, cocell_unfold, the 2 family slots in check_program, the node name `nodeD` (K2, D1); the removal of the kit `unfold` (K7).
- src/front/core.h: the `cocell` flag; OP_UNFOLD is removed (K7).
- src/front/eval.c: the bisimulation rule (K5); the removal of OP_UNFOLD (K7).
- src/json.c: the removal of OP_UNFOLD (K7).
- src/front/check.c: the slot guard in check_program (R5 (a), USER ruling 2026-10-23). When the checker uses more family, constructor or def slots than check_program reserves, check_program gives INTERNAL "the checker used more slots than check_program reserves" (check.c:4307 and 4308). Before, family_cap, ctor_cap and def_cap were not read.
- src/front/check.c: path_case_type gives HIT_BOUNDARY "the case of the path constructor %s has no type: an end of its boundary does not compute" when an end of the boundary does not compute (check.c:1393 to 1395, K8 (2)).
- domain/domain.lang: EnvK, StreamK, CofreeK, StoreF and Store (K4).
- docs/PLAN.md: `### USER ruling (S4 kickoff)`, K12, and the D1 ruling in K2 (the node `nodeD`). formers/FORMERS.md and domain/README.md have no text on the `cell` declaration (the 3 FORMERS.md hits are table cells), thus they get no cocell text.

### Added tests (K9)

- examples/cocell.lang: EnvK, StreamK, CofreeK, Store by hand, PropEnvK, the maps between StreamK and Stream, and the laws L1, L2, L3 and coassociativity for StreamK, EnvK, CofreeK and Store (closed step), and PropEnvK L2.
- test/parse/cocell-no-ops.lang (PARSE_EXPECT).
- test/check: cocell-carrier.lang (CELL_CARRIER), cocell-extract.lang (COCELL_EXTRACT), cocell-domain.lang (COCELL_DOMAIN, an op with no `X ->`), cocell-positive.lang (CODATA_POSITIVE), unfold-kit.lang (TYPE_INFER), unfold-no-hint.lang (TYPE_INFER), cocell-nats-evens.lang (PATH_END, D2), cocell-map-coassoc.lang (PATH_END). The codes come from real runs.
- test/check/codata-bisim.lang: a new comment gives the two reasons for the refusal (open point 6).
- test/eval/expect.txt: 14 cocell rows: headK, thirdK, doubledK, sumsK, backSecond, roundList, envAsk, envSum, treeShape, treeChild, dblEx, dblSeek, dblNext, penvNext.
- test/json/cocell.json (14 first-order instances, Nat and List Nat); test/json/algebra.json loses its kit unfold instance (K7).
- test/path-regressions.c, 16 rows: the c1 and c6 killers, a layer in a block (CODATA_FORM), the round trips of StreamK, EnvK and CofreeK, Cofree L1 at a variable, a cocell after a cell, PropEnvK, eta at a variable, L3 at a variable, coassociativity at a variable, the D1 ruling row, the fuel row bisim_fuel (t = 11: success at t, EVAL_FUEL at t - 1), eta for an hcomp at a cofamily, and the R5 row "cocell: 64 cocells have their family slots (R5)" (64 cocells after the domain; on the R5 mutant the slot guard gives INTERNAL).

### Deviations

- D1 (K2): the node of a cocell D is the surface constructor `nodeD`, a core name under rule R4 (USER ruling 2026-10-23). K2 said "A program cannot write them". With a hidden node, a program cannot make a cocell value (s4 probes: TYPE_MISMATCH "expected StreamK.layer Nat (Stream Nat)"). `D.layer`, `D.extract` and `D.Y` stay hidden. A program `def nodeStreamK` gives REFUSE_NAME.
- D2: test/check/cocell-nats-evens.lang uses a variable seed n (`natsK n` against `evensK n`). With the closed seed 0, no seed is a variable, so conv_bisim refuses before the hypothesis check, and the fixture does not test R6. The refusal comes from the child seeds `succ x` and `succ (succ x)`.
- D3: CODATA_FORM for a layer in a block is a path regression, not a check fixture: a program cannot declare a layer (REFUSE_DATA).
- D4: transp and hcomp at a cofamily get the rule by eta. K11 said "transp and hcomp keep the rule off". A transp at a line that changes is a neutral. At a type that is not a HIT, an hcomp is hcomp_stuck, a neutral for scrut_of; only a HIT gets the formal hcomp. Thus conv_counfold gives both eta. This is sound under the strict-final-coalgebra assumption (open point 10).
- D5 (K10 R3): the R3 mutant is AS BUILT `cores[p + 1u] = ((void)head, s);` in cocell_unfold (src/front/check.c:1866): the node drops the head. It is not the "fold node" form of the kickoff, because a case on the layer has no one-line C form.

### Open points (S4-build-PROMPT.md)

1. Transitivity: NOT transitive. At StreamK (StreamK (StreamK X)), a = `extend extract (extend duplicate w)`, b = `extend duplicate w` and c = `duplicate (duplicate w)`. a = b passes (L3), b = c passes (coassociativity), a = c is PATH_END. The seeds of a = c are `extend duplicate w` and `duplicate w`. Neither seed is a variable and the seeds do not convert, so conv_bisim has no h. This is an incompleteness of the rule, not a soundness fault. USER question for S5.
2. Termination: `extend (fun s => extend extract (extract s)) w` against w at `StreamK (StreamK Nat)` is accepted in 0.03 s. The check meets an unfold against a neutral in the extract field and calls the rule again (L3 at `extract x`). It ends under the fuel bound (each check spends 1).
3. transp and hcomp at a cofamily: the rule applies by eta (D4).
4. The read back with one variable abstracted. A seed with two variables: the swap stream (seed `pair x y`, step `scons (first p) (pair (second p) (first p))`) against `unfold (fun k => scons k k) x` is PATH_END, as it must be. The step that keeps `pair (first p) (second p)` converts, which is correct. A seed that is not a variable: the closed round trip `Path (Stream Nat) (toS Nat (toK Nat nats)) nats` is PATH_END (the seeds are `toK Nat nats` and 0); the same round trip at a variable passes.
5. PropEnvK idempotence: `fun i => unfold (fun (s : X) => nodePropEnvK P pp X X s (pp p q @ i)) d` is a path between the values at p and at q. Accepted.
6. The map form of coassociativity (test/check/cocell-map-coassoc.lang) is PATH_END. test/check/codata-bisim.lang stays PATH_END at the closed seed 0 (the rule does not apply) and at a variable seed n (the heads agree; the child seeds are n and `succ n`).
7. An all-pairs hypothesis (fresh x and y, independent) is not in K5 (a), and no probe was run. It also closes codata-bisim.lang and could close the a = c case of open point 1. USER question for S5.
8. The killers for P5, P10 and P10b (K8): one try for each (s10, ~/Documents/virtu-lang-work/s10b). The 3 mutants SURVIVED. S3 D4 (P5) and D7 (P10) stay open. P10 (src/front/check.c:937 cell_fold, drop the rebuild_ok call) and P10b (check.c:914 rebuild_ok, `if (m->ctors[inst].interval_count == 0u)` to `if (1)`): the try s10b/try/p10-try.lang maps `natAdd n 1` through the StateC law path gpC at a variable interval and binds through gpC. The base, P10 and P10b give rc 0. The cause is the S3 cause: cell_rebuild makes the case from the constructor, thus its ends are the boundary at the rebuilt fields, and this is the case type of rebuild_ok. A continuation field (gpC) does not change this. P5 (src/front/eval.c:1058 fold_square, swap `inner[0]` and `inner[1]`): the try s10b/try/p5-try.lang declares the cell Coher2 of the S3 review regression ("cell map preserves earlier path and square boundary cases") and folds with neutral cases. The case type holds the fold of c2s at (j, j), a stuck square fold in a quoted value. The base and P5 give rc 0. The cause: in a boundary face only one interval is free. At j = i0 or i1, the new evaluation makes the outer path (the CLOS_FOLD_FACE VAL_PLAM) concrete first, thus path_apply does not read the stored inner ends. The inner ends are read only when r1 stays neutral and r2 becomes an end with no new evaluation (val_isubst, glue_at, eval.c:2832). No program for this was found. Runners s10b/p-runs/s10-1-kill.zsh and s10-1-p5.zsh; each restore is cmp equal.
9. The JSON form: test/json/cocell.json has the 14 first-order instances only. A cofamily value has no JSON form (src/json.c).
10. Claim gap: a judgmental bisimulation rule needs a strict final coalgebra in the model. No source for the cubical model was read. It stays open (C17).
### Staged review corrections

The S4 gate table, K rows and mutant table elsewhere in this section record the initial candidate before staged review. The review makes these corrections:

- HIGH: layer eta and the observation rule treated every cofamily layer as a record, although cofamily declarations allow several point constructors. For `TwoF X := zeroF | oneF (n : Nat)`, the rule inspected only the zero-field first constructor and accepted `Path Two (unfold zeroStep 0) (unfold oneStep 0) := refl`. It also equated a constant zero-constructor unfold with an arbitrary variable and equated that constructor with an arbitrary layer. Record eta and nonstructural unfold conversion now require exactly one layer constructor. Three refusal regressions cover these false paths; positive regressions retain reflexivity and `out` computation for both constructors.

- HIGH: the graph conversion rule was not stable under substitution and was not transitive. With `g n = scons Nat Nat 0 n`, the variable-seed proofs relating `unfold g (succ n)`, `unfold g n` and `unfold g (natAdd n 2)` checked pairwise through the middle term, while the outer pair failed. Specializing the first accepted proof at zero checked, but direct `refl` at the same closed endpoints failed. The variable graph construction is removed. Recursive cofamilies now compare convertible seeds of the same type and check the identity relation uniformly. Typed eta contracts at closed seeds as well as neutrals. Finite cofamilies with no recursive Y field compare their observations directly, without a coinductive hypothesis. L1, L2, L3, Store laws and EnvK coassociativity still check. Full StreamK/CofreeK coassociativity and the two stream round trips are explicit refusal regressions; examples check their head projections. A stronger path or conversion rule remains future work. This replaces the characterization of open point 1 as harmless incompleteness.
- HIGH: inferred `extend` and cocell `map` codomains could inhabit a higher universe, bypassing the carrier parameter's `Type 0` bound. The checker now rechecks the inferred codomain at `Type 0`, using full source readback with fresh local aliases. A private lexer entrypoint accepts generated dotted names during this internal replay. Public lexing keeps its existing refusal. Positive result-type examples and negative inferred, hinted, function, Sigma, neutral and dependent-result fixtures cover both operations.
- Test coverage: `algebra stop => nil` and its JSON instance are restored using `take Nat 0`. The three removed bounded-List-unfold inputs are retained as refusal regressions for the intentionally removed syntax. The fuel regression still checks an independently measured cost and failure with one unit less, including the new type and final seed comparisons.

The initial K10 mutant table is historical evidence for the pre-review candidate; its source line numbers and graph-rule sites do not describe the revised evaluator. The revised implementation is validated by a fresh full gate and targeted before/after probes recorded with this review. No commit is made by the review.

The revised `make -j 2 check` exits 0: 37 examples, 16 parse refusals, 117 check refusals, 159 evals, 37 JSON builds, 5 build refusals, 6 host regressions, 2 printer regressions and 193 path regressions. TinyCC builds and the strict C99 syntax check with `-Wall -Wextra -Wswitch-enum -Werror` pass. Full capture: ~/Documents/gpt5/virtu-staged-review/staged/.kanon-exec/run-zoku8U. An independent final review recompiles the five-case two-constructor C probe and checks higher-universe refusals plus dependent Sigma, captured Path and internal-layer carrier examples. It reports no remaining medium-or-higher findings.

### Mutants (K10)

The runner ~/Documents/virtu-lang-work/S4-mutants.zsh (outside the repo) has the form of the S3 runner. It finds each site by a unique anchor text in the main checkout plus an offset, applies the mutant on a scratch copy and runs `make -B -j 2 check`. When the gate stops before the path regressions, the runner builds build/path-regressions again and runs it. Then it restores the file from the main checkout and compares the two files with cmp. Output: ~/Documents/virtu-lang-work/s7b/S4-mutants.out; logs: s7b/mlogs. Each restore is equal. The final gate on the scratch copy exits 0 (179 path regressions, before the R5 row), so each killer passes before and after. R5 has its own runs: s8b (the R5 (a) ruling and the slot guard) and s9b (the killer row; runner s9b/p-runs/s9-1-kill.zsh, logs s9b/logs). The lines are lines of the S4 work tree; they did not move after the run. Result: 9 of 9 KILLED; R5 is KILLED after the R5 (a) ruling.

| id | site | mutation | killer | result |
|---|---|---|---|---|
| R1 | src/front/check.c:3868 codata_positive | `t = t->b, ix++` to `t = t->b` | path regression "cofamily: a Pi chain field with X under List (c1)" (CODATA_POSITIVE) | KILLED |
| R2 | src/front/check.c:3889 cofamily_layer | `if (!conv_values(...))` to `if (0 && !conv_values(...))`: the parameter check is off | path regression "cofamily: a parameter in Type 1 against a layer parameter in Type 0 (c6)" (CODATA_FORM) | KILLED |
| R3 | src/front/check.c:1866 cocell_unfold | `head == NULL ? s : core_app(...)` to `((void)head, s)` (D5) | examples/cocell.lang (PATH_END streamL1), test/check/cocell-map-coassoc.lang (exit 0), the cocell eval rows: 17 gate failures; 4 path regressions ("cocell: Cofree L1 at a variable", "cocell: PropEnvK, the coreflection P * - at a proposition (K6)", "bisimulation rule: L3 at a variable", "bisimulation rule: coassociativity at a variable") | KILLED |
| R4 | src/front/check.c:4185 check_cocell | `if (strcmp(d->ctors[i].name, "extract") == 0)` to `if (0 && ...)` | test/check/cocell-extract.lang (REFUSE_NAME, not COCELL_EXTRACT): 1 gate failure | KILLED |
| R5 | src/front/check.c:4278 check_program | `? 1u : 0u` to `? 0u : 0u`: 1 family slot for each cocell, not 2 | the first run: none, the gate exits 0 (the slot caps were not read, and the spare slots of the defs hide the fault). After R5 (a): path regression "cocell: 64 cocells have their family slots (R5)" (INTERNAL "the checker used more slots than check_program reserves") | KILLED (SURVIVED in the first run) |
| R6 | src/front/eval.c:3426 bisim_field | add `a = b;` after `b = b == NULL ? NULL : rel_apply(m, h, b);`: the check compares b with b | test/check/codata-bisim.lang, cocell-nats-evens.lang and cocell-map-coassoc.lang (exit 0, not PATH_END): 3 gate failures | KILLED |
| R7 | src/front/eval.c:3458 conv_bisim | `if (!spend(m, 1))` to `if (0 && !spend(m, 1))` | path regression "the bisimulation check spends 1 fuel" (bisim_fuel) | KILLED |
| R8 | src/front/eval.c:3492 conv_counfold | `} else if (scrut_of(v) != SCRUT_NEUTRAL)` to `if (1 \|\| ...)` | examples/cocell.lang (PATH_END roundS), the cocell eval rows: 16 gate failures; 4 path regressions, with the fuel row, "bisimulation rule: eta at a variable" and "bisimulation rule: eta for an hcomp at a cofamily" | KILLED |
| R9 | src/front/eval.c:3460 conv_bisim | `if (conv_values(m, level, s1.s, s2.s))` to `if (1 \|\| ...)`: h is always the identity | 16 gate failures; 3 path regressions, with the fuel row and "unfold: refl of g at 0 and g at 1 after eval" (PATH_END) | KILLED |

### USER questions for S5

- Q1 (open point 1): the conversion is not transitive on unfolds. Accept this incompleteness, or add a rule (for example the all-pairs hypothesis of Q2)?
- Q2 (open point 7): add an all-pairs hypothesis (fresh x and y, independent) to K5 (a)?
