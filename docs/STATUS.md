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

## S5

S5 adds the base span: the carriers `Text` and `Credits`, the four classes as reference cells, the program monad `Virt` with 28 0-cells, the world `W`, the runner `run` and the JSON output of the world. The kickoff proposals K1 to K14 are all (a) (docs/PLAN.md, `### USER ruling (S5 kickoff)`). The claims C18 to C21 are in ~/Documents/virtu-lang-work/S5-claims.md and the build records are in ~/Documents/virtu-lang-work/s5-build/ (outside the repo).

### Gate

The gate runner ~/Documents/virtu-lang-work/s5-build/b-gate.zsh exits 0 (log s5-build/b-gate.log). The S4 pin is 6c0c0ed: the revised S4 gate of [Staged review corrections](#staged-review-corrections).

| gate line | S4 (pin) | K11 | S5 | difference and cause |
|---|---|---|---|---|
| examples | 37 | 39 | 39 | +2: examples/virt.lang, examples/carriers.lang |
| parse refusals | 16 | 16 | 16 | +1: test/parse/string-unterminated.lang; -1: test/parse/text-literal.lang (a string in double quotes is now a token, K1) |
| check refusals | 117 | about 124 | 125 | +9: 9 files in test/check (Added tests); -1: test/check/text-removed.lang (`Text` is back, K1) |
| evals | 159 | about 185 | 210 | +51: 33 rows for examples/virt.lang, 18 rows for examples/carriers.lang |
| build | 37 | 38 | 39 | +2: test/json/virt.json, test/json/carriers.json |
| build refusals | 5 | 7 | 7 | +2: test/emit/credits-overflow.lang, test/emit/unknown-id.lang |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 193 | about 203 | 220 | +27 rows (Added tests) |

Four lines differ from K11 (D1).

### K rows

- K1: `Text` is a kernel builtin again, with string literals in double quotes (src/front/lexer.c) and `textEq : Text -> Text -> Option Unit` (src/front/check.c:179). domain/domain.lang gives `Role := Text`, `Value := Text` and `Credits := Nat` with `creditsAdd`, `creditsSub` (to `Option`), `creditsMul` and `creditsLe` (domain.lang:198 to 206). `creditsAdd` and `creditsMul` call the builtin `creditsCheck` (check.c:178), which gives EVAL_CREDITS_OVERFLOW at a value above 2^64 - 1 (src/front/eval.c:3146). `Entry` and `Listing` are `Prod` chains (domain.lang:261 and 262). The kernel fixtures that are not cell monads, duals or carriers are in domain/fixtures.lang, which only the gate loads ([Gate build](#gate-build)).
- K2: the reference cells `AvatarC`, `VirtuCardC`, `VirtualTerminalC` and `VirtualItemC`, each with one 0-cell `Nat -> X`, and `Avatar := AvatarC Empty` and the same for the other three (domain.lang:209 to 216). `refUnknown : (A : Type 0) -> Nat -> A` (check.c:181) gives EVAL_REF_UNKNOWN for an id with no instance (eval.c:3165).
- K3: `cell Virt (X : Type 0)` (domain.lang:355) has 28 0-cells (27 rows; row 4 gives `avatarCard` and `attachCard`). The op fields have no names.
- K4: `W := List Instance` (domain.lang:268). `InstanceC` has `avatarI`, `cardI`, `terminalI` and `itemI`, each with the state record of PLAN 4 (domain.lang:263 to 267).
- K5: `run : (X : Type 0) -> Virt X -> W -> Prod W X` (domain.lang:385) is a library fold with one case for each 0-cell. `langc build` finds `def main : Virt Unit` (src/json.c:1026), runs it from the empty world and writes the final world.
- K6: src/json.c writes `{"virtu-lang":1,"instances":[...]}` with the keys `id`, `class` and `state` (open point 8). A program with no `main` keeps the S1 to S4 form.
- K7: `SecurityLevelC` (4), `RarityC` (5) and `EntryKindC` (7) are cells with nullary 0-cells at `Empty` (domain.lang:225 to 227), each with a path to `Fin k` by `ua` and `isoToEquiv`. `multiplier : Rarity -> Credits` is a fold (domain.lang:256).
- K8: [K8, K13 and K14 notes](#k8-k13-and-k14-notes).
- K9: the added tests below. There is no new node, Python or shell harness.
- K10: [Mutants (K10)](#mutants-k10-1): 17 of 18 KILLED; U2 is an equivalent mutant (open point 9).
- K11: the gate above.
- K12: the PLAN edits: 2.5 (Plotkin and Pretnar for the runner, the qualified Katsumata, Rivas and Uustalu cite, the Jacobs details), 2.5 and 5.1 ("28 0-cells (27 rows)"), 6 (the RFC 8259 notes). The S4 kickoff ruling and the S4 staged review correction stay.
- K13, K14: [K8, K13 and K14 notes](#k8-k13-and-k14-notes).

### Claims (S5-claims.md)

- C18: the runner as a monad and comonad interaction law (Katsumata, Rivas and Uustalu, LICS 2020). PARTIAL. The abstract gives stateful runners; the State/Store interaction law is not checked. PLAN 2.5 keeps the cite with this qualifier (K12). The runner does not depend on the claim: it is a fold (C20).
- C19: an instance is a coalgebra of its class (Jacobs, 1996). CONFIRMED (abstract). A reference type plus a state in the world is the identifier and local state form of the source. S5 does not build the creation by the terminal coalgebra (the world is a list, K4).
- C20: the runner as a handler (Plotkin and Pretnar, LMCS 2013). CONFIRMED (abstract), with a uniqueness note. `run` is the fold of the free `Virt` algebra into the model `W -> Prod W X`. The beta equations give the homomorphism; its uniqueness needs an eta equation (Ahman, arXiv 1710.02594).
- C21: the JSON output of PLAN 6 (RFC 8259). CONFIRMED. A `Credits` value up to 2^64 - 1 is a valid JSON number, but a number above 2^53 - 1 is not interoperable. The member order of `attributes` is in the text only; a parser can drop it.

### Kernel changes

- src/front/lexer.c, lexer.h: the string token in double quotes, the escapes, and the codes LEX_STR_OPEN, LEX_STR_ESCAPE and LEX_STR_CONTROL (open point 7).
- src/front/ast.h, parser.c: the string literal term.
- src/front/core.h: the ops OP_TEXT, OP_TEXT_EQ, OP_CREDITS_CHECK and OP_REF_UNKNOWN.
- src/front/check.c: the BUILTINS rows `creditsCheck`, `textEq` and `refUnknown` (check.c:178 to 181); check.c:2441 knows the two new EVAL codes (a conversion that evaluates a closed term can give them, open point 1).
- src/front/eval.c: Text values and `textEq`, `creditsCheck` and `refUnknown` with their codes.
- src/json.c: the world writer (K6): the `main` lookup, the run from the empty world, the state keys, the enumeration names (ENUM_NAMES, json.c:803) and the JSON escapes of a Text value.
- gen/embed.c: `embed IN... OUT` writes the files in order, with a newline between two files. One IN gives the same bytes as before.
- Makefile: GATE_DOMAIN, build/domain-gate.c and build/langc-gate. build/path-regressions links build/domain-gate.c. `check` also builds build/langc-gate.
- test/gate.sh: `langc_for FILE`. test/regressions.js: gateCompiler for the Vec rows of the test "printed types preserve binding under normalization".
- domain/domain.lang: the S5 library (K1 to K7). domain/fixtures.lang: the 22 moved fixtures.
- examples/mealy.lang, state.lang and state-free.lang: the program def `run` is now `runMealy` or `runState`, because the library `run` (K5) is a core name (rule R4).

### Gate build

build/langc embeds domain/domain.lang only. build/langc-gate embeds domain/domain.lang and then domain/fixtures.lang (GATE_DOMAIN in the Makefile). domain/fixtures.lang holds the 22 kernel fixtures that are not cell monads, duals or carriers (K1): S1, Trunc1, Trunc0, Quot, Vec, Charter (with Ratified), StreamF, Stream, ColistF, Colist, MealyF, Mealy, transport, subst, J, JRefl, trans, uaArrow, finSub, isSetFin, decEqProd and decEqSigmaFin. The other definitions stay in domain.lang because a kept definition needs them (for example `finPath` needs `ua`, and `isSetNat` is used by examples/state.lang).

test/gate.sh checks, evaluates and builds each file of test/gate-domain.txt with build/langc-gate, and each other file with build/langc. test/gate-domain.txt has 44 files: 21 examples, 22 files in test/check and 1 in test/emit. A new test file that uses a fixture goes in test/gate-domain.txt (the exact path). build/path-regressions loads the gate domain.

Diagnostics (real runs in a scratch copy): src/front/front.c gives the embedded text the one source name domain/domain.lang. A check diagnostic at a fixture def names the def and gives no source line (`langc: TYPE_MISMATCH: isSetFin: ...`). A lexer diagnostic gives `domain/domain.lang:LINE:COL`, where LINE counts in the joined text: line k of fixtures.lang is line N + 1 + k, with N the line count of domain.lang (line 90 of fixtures.lang gave domain/domain.lang:539 at N = 448) (D2).

### Added tests (K9)

- examples/virt.lang and examples/carriers.lang, with 51 eval rows in test/eval/expect.txt.
- test/json/virt.json and test/json/carriers.json.
- test/parse/string-unterminated.lang (LEX_STR_OPEN). test/parse/text-literal.lang is removed.
- test/check: text-op.lang (NAME_UNKNOWN), ref-class.lang (TYPE_MISMATCH), virt-op-name.lang (REFUSE_NAME), text-eq-nat.lang (TYPE_MISMATCH), string-escape.lang (LEX_STR_ESCAPE), string-control.lang (LEX_STR_CONTROL), credits-overflow.lang (EVAL_CREDITS_OVERFLOW), ref-unknown.lang (EVAL_REF_UNKNOWN), ref-unknown-type.lang (TYPE_MISMATCH). test/check/text-removed.lang is removed. The codes come from real runs.
- test/emit: credits-overflow.lang (EVAL_CREDITS_OVERFLOW) and unknown-id.lang (EVAL_REF_UNKNOWN).
- test/path-regressions.c, 27 rows: Text 7 (`textEq` at equal bytes, at unequal bytes and at a variable; each lexer escape; an unknown escape; a raw control byte; the JSON escapes of a Text value), Credits 3 (`creditsCheck` keeps 2^64 - 1, refuses 2^64, is stuck at a variable), reference 3 (`refUnknown` refuses a literal id, is stuck at a variable id, takes `A : Type 0` only), world writer 6 (no `main` keeps the S1 to S4 form; `main` at `pure unit`; id, class and state in table order; a `main` of a type that is not `Virt Unit`; an avatar with no card; two avatars, two cards and a transfer), the bound builtin at `langc build` 2, a card reference at an avatar instance 1, the `main` lookup 2 (`main` after another def; a name with the prefix `main`), a string literal to JSON 1, the U6 killer 2 (no eta at a seed type with a different parameter, and the eta at the seed type of the cofamily; open point 9).
- The "credits:" rows and the "bound builtin:" rows use the same `creditsCheck` terms on two routes: a Path by conversion at `langc check`, and a def body at `langc build` with the JSON number. Both pairs stay.

### Deviations

- D1 (K11): four gate lines differ from K11. Check refusals: 125, not about 124 (K11 counted about 8 new files; S5 adds 9). Evals: 210, not about 185 (K11 counted about 26 new rows; some 0-cells have more than one row, and examples/carriers.lang has 18 rows). Build: 39, not 38 (test/gate.sh builds each example against test/json/NAME.json, thus examples/carriers.lang needs a golden, open point 6). Path regressions: 220, not about 203 (K11 forecast about 10 new rows; S5 adds 27).
- D2: a lexer diagnostic at a fixture line names domain/domain.lang and a line in the joined text ([Gate build](#gate-build)). S5 does not change front.c.

### Open points (S5-build-PROMPT.md)

1. The codes are EVAL_CREDITS_OVERFLOW and EVAL_REF_UNKNOWN. `refUnknown : (A : Type 0) -> Nat -> A`. `langc check` does not evaluate a def body: a closed def that overflows passes `check`, and `build` refuses it (exit 1, no output). `check` refuses a closed term that overflows when a conversion evaluates it (test/check/credits-overflow.lang, a refl); the same for `refUnknown` (test/check/ref-unknown.lang). `creditsCheck` refuses a value of 2^64 or more, not of 2^53 or more.
2. The order of the fold cases is the PLAN 5.1 row order, then `pure`: 29 cases. `check` accepts it.
3. `pure unit` at `Virt Unit` needs no hint, at a def type and in a 0-cell field (path regressions).
4. An enumeration value (a 0-cell with no field) is its 0-cell name. ENUM_NAMES (json.c:803) maps `transferOut` to "transfer-out" and `transferIn` to "transfer-in".
5. The load route is build/langc-gate ([Gate build](#gate-build)). 44 files use a fixture (s5-build/b-s8-users.log); the other 18 examples run through build/langc.
6. json 39: test/gate.sh builds each examples/*.lang, thus examples/carriers.lang needs test/json/carriers.json.
7. A string literal has the escapes `\"`, `\\`, `\n`, `\t` and `\xHH` (lexer.c:201). The JSON writer gives each escaped byte in the `\u` form (`"`, `\`, `\u000a`, `\u0009`). `langc eval` prints `"a\"b\\c\x0ad\x09eA"`.
8. The state keys are the keys of PLAN 6. `attributes` is a JSON object in List order. The writer does not merge a key that occurs two times: the library keeps a key at the place of its first set.
9. U2 and U6 ([Mutants (K10)](#mutants-k10-1)). U2 is an equivalent mutant. At a cofamily type, only an unfold or a neutral gets to the branch: the branch above takes an unfold, and hcomp_stuck and the transp neutral are neutrals. No trap value exists: val_trap (src/front/eval.c:143) is called only at the eval of CORE_TRAP (eval.c:3336), and only the quote of a trap value writes CORE_TRAP (eval.c:3866). conv_values does not use the type, and conv_pathp compares the endpoints before the line (eval.c:3401). Thus a type mismatch can give the branch a value that is not neutral (a Nat, a Text or a constructor). Then the line compare fails, and the two binaries give the same TYPE_MISMATCH. The probes ~/Documents/virtu-lang-work/s5-build/u2/p1.lang to p9.lang give the same output on the main build/langc-gate and on the U2 build. U6 has a killer row. No field type of the cocell `PhK (P : Type 0) (X : Type 0)` uses P, so a step at the seed type `PhK (List Nat) Nat` passes the field check. Without the seed type check, eta contracts the unfold at the type `PhK Nat Nat`, and the refl is accepted, not PATH_END (path regression "bisimulation rule: no eta at a seed type with a different parameter").

### Mutants (K10)

The runner ~/Documents/virtu-lang-work/S5-mutants.zsh (outside the repo) has the form of the S4 runner. Each anchor text must have exactly 1 match in the main checkout. The runner applies each mutant with `sd -F` on a scratch copy (s5-build/mscratch, an rsync of the main checkout with no .git) and runs `make -B -j 2 check`. When the check stops before test/gate.sh or the path regressions, the runner runs them apart. Then it restores the file from the main checkout and compares the two files with cmp. Output: ~/Documents/virtu-lang-work/s5-build/S5-mutants.out; logs: s5-build/mlogs. The base gate on the scratch copy exits 0 (218 path regressions). Each restore is equal. The final gate on the scratch copy exits 0 (0 gate failures, 218 path regressions, 0 failed), so each killer passes before and after. The lines are lines of the S5 work tree. Result: 17 of 18 KILLED; U2 SURVIVED, and it is an equivalent mutant (open point 9). The U6 killer row has its own run (runner s5-build/u6/u6-row.zsh): with U6, 2 of 220 path regressions fail (the fuel row and the U6 row); after the restore (cmp equal), 0 of 220 fail.

| id | site | mutation | killer | result |
|---|---|---|---|---|
| U1 | src/front/eval.c:3587 conv_counfold | `ctor_count != 1u)` to `< 1u)` | path 2 of 218: "cofamily: conversion preserves distinct layer constructor tags", "cofamily: a constant constructor step is not eta for a sum" | KILLED |
| U2 | eval.c:3583 conv_counfold | `} else if (scrut_of(v) != SCRUT_NEUTRAL)` to `} else if (0)` | none: 0 gate failures, path 0; the probes s5-build/u2/p1 to p9 give the same output | SURVIVED: an equivalent mutant (open point 9) |
| U3 | eval.c:3520 conv_bisim | remove `&& conv_values(m, level, s1.s, s2.s)` | path 5: "unfold: refl of g at 0 and g at 1 after eval" and 4 "bisimulation rule" rows (unequal variable seeds; mirror graph; nonvariable graph pair; closed graph pair) | KILLED |
| U4 | eval.c:3504 bisim_check | `spend(m, 1) ?` to `1 ?` | path 1: "the bisimulation check spends 1 fuel" (t 12, model 13) | KILLED |
| U5 | eval.c:3494 bisim_field | the return to `a != NULL && b != NULL` | gate 2: test/check/codata-bisim.lang, cocell-nats-evens.lang (exit 0, not PATH_END); path 1: the fuel row (t 10) | KILLED |
| U6 | eval.c:3529 counfold_eta | `conv_values(m, level, u->arg, u->dom) &&` to `1 &&` | path 1 in the run: the fuel row (t 11, model 13); after open point 9 also "bisimulation rule: no eta at a seed type with a different parameter" (PATH_END) | KILLED |
| U7 | eval.c:3589 conv_counfold | `if (finite_layer(...))` to `if (0 && ...)` | gate 16: examples/cocell.lang (PATH_END at envCo) and its eval rows; path 1: "cofamily: finite observations can agree at different seed types" | KILLED |
| U8 | src/front/check.c:1985 rule_extend | remove `!cocell_result_ok(c, b)` of fv | gate 4: test/check cocell-extend-universe, -pi-, -sigma-, -neutral-universe (exit 0, not TYPE_MISMATCH); path 0 | KILLED |
| U9 | check.c:1998 cocell_map | remove `!cocell_result_ok(c, b)` of gv | gate 4: the 4 cocell-map-*universe inputs; path 0 | KILLED |
| U10 | src/front/lexer.c:316 lex_source | `diag, 0);` to `diag, 1);` | gate 1: cocell-internal-name.lang (TYPE_ARITY, not LEX_CHAR); path 0 | KILLED |
| V1 | eval.c:3144 credits_check (the bound test) | `if (big_fits_u64(b, &x))` to `... \|\| 1)` | gate 2: test/check/credits-overflow.lang (exit 0), build test/emit/credits-overflow.lang; path 2, with "bound builtin: build refuses creditsCheck 2^64" | KILLED |
| V2 | eval.c:3163 ref_unknown (the reference refusal) | `if (!big_of(...) \|\| !big_text(...))` to `if (1 \|\| ...)` (OOM, not EVAL_REF_UNKNOWN) | gate 2: test/check/ref-unknown.lang, build test/emit/unknown-id.lang; path 2, with "reference refusal: a card reference at an avatar instance" | KILLED |
| V3 | lexer.c:226 lex_str (the string token) | `if (lx->pos >= lx->len) {` to `if (0 && ...)` (LEX_STR_OPEN off) | gate 1: test/check/string-unterminated.lang (exit 0, not LEX_STR_OPEN); path 0 | KILLED |
| V4 | src/json.c:54 put_quoted (the `\u` writer) | remove `ch != '"' &&` | gate 1: build examples/carriers.lang differs from test/json/carriers.json; path 2: "Text escapes: a string literal to JSON", "text: the JSON escapes of a Text value" | KILLED |
| V5 | lexer.c:171 escape_byte (a Text escape) | `case 'n': return '\n';` to `return 'n';` | gate 2: eval carriers escaped, build carriers.lang; path 2: "Text escapes: a string literal to JSON", "text: each lexer escape gives its byte" | KILLED |
| V6 | json.c:1026 json_document (the `main` lookup) | `def_index(m, "main", 1)` to `(m, "main", 0)` | gate 3 (the build rows of world documents); path 7, with "world writer: main at pure unit gives the empty world" | KILLED |
| V7 | json.c:810 enum_name (ENUM_NAMES) | `if (strcmp(...) == 0)` to `if (0 && ...)` | gate 1: build examples/virt.lang differs from test/json/virt.json; path 2: "world writer: id, class and state in table order", "world writer: two avatars, two cards and a transfer" | KILLED |
| V8 | json.c:956 world_item_json (the world writer keys) | `",\"class\":"` to `",\"klass\":"` | gate 1: build examples/virt.lang; path 4: the "world writer" rows (table order; an avatar with no card; two avatars, two cards and a transfer) and "main lookup: main after another def writes the world" | KILLED |

### K8, K13 and K14 notes

- K8 (a): S4 open points 8, 9 and 10 and the S4 review leftovers stay open. The S4 K10 table stays historical: its lines (eval.c:3426, 3458, 3492 and 3460) are lines of the candidate before the review, and at 6c0c0ed they are not the sites. The cites of S5-kickoff.md survey f (lines at 6c0c0ed) have these current lines: `nat_sub` (`x < y ? 0 : x - y`) eval.c:1275 is now eval.c:1288; R6 `bisim_field` is eval.c:3481; R7 the `spend` in `bisim_check` is eval.c:3504; R8 `} else if (scrut_of(v) != SCRUT_NEUTRAL)` in `conv_counfold` is eval.c:3583; R9 the seed check in `conv_bisim` is eval.c:3520; S1 M1 `rule_fin_case` check.c:2696 is now check.c:2727; `is_enum` is json.c:155. gen/embed.c and the Makefile now take more than one domain file ([Gate build](#gate-build)).
- K13 (a): S5 accepts that conversion is incomplete at full StreamK and CofreeK coassociativity. A stronger path rule needs a source for the cubical model first.
- K14 (a): Q2 (the all-pairs hypothesis) is dropped. The all-pairs idea is future work that needs a soundness argument first.
### S5 staged review corrections

Review of all 44 staged paths found three MEDIUM defects. CI and test routing were checked first. The two removed Text refusal fixtures cover syntax and a type that S5 intentionally enables; the remaining kernel fixture tests still run through the gate compiler.

- Text homogeneous composition omitted VAL_TEXT in both the literal head comparison and the data reduction rule. For example, `hcomp Text i0 (fun (i : I) => []) "Ada"` stayed stuck, so JSON refused a closed Text and a reflexivity proof failed. Both rules now handle Text literals, including a constant live face. A negative equality test still rejects different literals.
- The lexer accepted `"\xff"` and the JSON writer emitted the raw invalid UTF-8 byte. A UTF-8 reader rejected that successful build. Decoded string literals now require valid UTF-8 scalar values, with LEX_STR_UTF8 for malformed bytes, overlong forms, surrogates and out-of-range code points. Valid multibyte text and escaped NUL still serialize correctly.
- A self transfer, a transfer with insufficient funds, or a purchase with no matching listing could skip checking a supplied reference and return none. For example, a self transfer at card id 0 in the empty world succeeded instead of EVAL_REF_UNKNOWN. cardTransfer and purchaseIn now validate their references before these branches, including the referenced class.

The C runner has 15 added cases: four reference refusals, three Text composition cases, seven malformed UTF-8 refusals, and valid UTF-8 with escaped NUL. The reproduced failures precede the fixes. The corrected runner passes 235 cases. The original S5 gate table and mutant results above remain historical records of the candidate before this review.

## S6

S6 adds the L5 laws of PLAN 5.3 (two reads give the same value) as 11 1-cells of `Virt`, one for each read row of PLAN 5.1, with a `refl` path case in `run` for each. L1 to L4 are laws at known references with closed-world rows; their proofs at a variable world go to the new slice S6R (PLAN section 7). The kickoff proposals K1 to K11 and the questions Q1 to Q4 are all (a) (docs/PLAN.md, `### USER ruling (S6 kickoff)`). The claims C22 to C25 are in ~/Documents/virtu-lang-work/S6-claims.md and the build records are in ~/Documents/virtu-lang-work/s6-build/ (outside the repo). src/ does not change.

### Gate

`make -B -j 2 check` exits 0 (log ~/Documents/virtu-lang-work/s6-build/gate.log). The S5 pin is 0b6cd7b (tree b89e1f93; gate log s6k-gate0.log).

| gate line | S5 (pin) | K9 | S6 | difference and cause |
|---|---|---|---|---|
| examples | 39 | 40 | 40 | +1: examples/virt-laws.lang |
| parse refusals | 16 | 16 | 16 | none |
| check refusals | 125 | 130 | 130 | +5: the 5 law-*.lang files in test/check (Added tests) |
| evals | 210 | 210 + n | 212 | +2 (n = 2): `virt-laws readTwo => 10`, `virt-laws readOne => 10` |
| json (builds) | 39 | 40 | 40 | +1: test/json/virt-laws.json |
| build refusals | 7 | 8 | 8 | +1: test/emit/law-path-main.lang |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 235 | 255 | 255 | +20 rows (Added tests) |

Each line is equal to the K9 prediction.

### K rows

- K1: the laws of PLAN 5.3. L4 has the continuation form `avatarSet a k v (avatarGet a k t) = avatarSet a k v (t (some v))` and L5 has the Prod form `r a (fun x => r a (fun y => k (pair x y))) = r a (fun x => k (pair x x))` for `k : Prod R R -> X`. C22: L1 to L3 are schema 3, L4 is schema 4 and L5 is schema 2 of the 7 global state schemas.
- K2: no kernel change for the declaration. Each 1-cell takes X explicitly, as in StateC. Each field name is new at the global level (open point 2).
- K3: L5 only. The 1-cells `rrName`, `rrRole`, `rrCard`, `rrGet`, `rrInv`, `rrBal`, `rrSec`, `rrHist`, `rrTInv`, `rrVal` and `rrIGet` (domain/domain.lang:389 to 399) come after the last 0-cell and before `pure`. `cell Virt` (domain.lang:361) has 40 constructors.
- K4: each L5 path case of `run` (domain.lang:405) is `refl`: a read does not change the world, thus the two images are convertible. L1 to L4 get no `run` path in S6.
- K5: no kernel change in S6. The routes R1 to R4 go to the slice S6R.
- K6: the S5 leftovers ([Open points](#open-points-s6-build-promptmd)).
- K7: [Added tests](#added-tests-k7-1).
- K8: [Mutants (K8)](#mutants-k8): 5 of 7 KILLED; W3 SURVIVED (likely equivalent) and W6 SURVIVED (equivalent).
- K9: the gate above.
- K10: the PLAN edits: section 1 `### USER ruling (S6 kickoff)`; 5.3 (L5 only in S6, the L4 and L5 forms, the known reference condition, R1 to R4, the C22 and C23 cites); section 7 (the S6 row and the new S6R row); an O23 note.
- K11: Q1 (a): R3 is the propagation of `refUnknown` in the kernel, in S6R; a refusal stays a refusal. Q2 (a): L5 only in S6. Q3 (a): 5.3 has 3 of the 7 schemas. Q4 (a): the 1-cell cycle probe is on a scratch copy only (open point 5).

The order of the fold cases of `run`: the 28 0-cells in the PLAN 5.1 row order; the 11 L5 laws in the read row order (rows 2, 3, 4, 6, 7, 9, 10, 15, 20, 25 and 27: `avatarName`, `avatarRole`, `avatarCard`, `avatarGet`, `avatarInventory`, `cardBalance`, `cardSecurity`, `cardHistory`, `terminalInventory`, `itemValue`, `itemGet`); then `pure`. 40 cases, thus 41 fold arguments with the scrutinee, under SPINE_MAX 64 (src/front/check.c:13).

### Claims (S6-claims.md)

- C22: L1 to L5 as the equations of global state (Plotkin and Power, FoSSaCS 2002, section 3). CONFIRMED: 3 of the 7 schemas (2, 3 and 4). Schema 1 and the commutation schemas 5 to 7 are not in 5.3 (Q3). The completeness result is for a store `V^L`; `Virt` is not that monad, thus S6 does not claim that L1 to L5 are all the laws of `Virt`.
- C23: a handler is correct only when it satisfies the equations of the theory (Plotkin and Pretnar, LMCS 2013, section 1.3). CONFIRMED. The kernel checks the condition: a fold over a cell with a 1-cell needs a path case (test/check/law-no-case.lang), and the path case type has the two images as its ends (W1).
- C24: the S5 C20 uniqueness note. CARRIED: S6 claims no uniqueness of `run`.
- C25: the PLAN sources for a law. NONE IN SECTION 6: PLAN 6 cites RFC 8259 for JSON notes only. PLAN 5.3 now cites C22 and C23.

### Added tests (K7)

- examples/virt-laws.lang with test/json/virt-laws.json and 2 eval rows (`readTwo`, `readOne`: the two ends of rrBal at a closed world, 10 and 10). The example has `main` (rrBal at i0), `lawsPath` (the law in `Virt Unit`) and `lawsRun` (the runner path at a variable world).
- test/check, 5 files: law-curried.lang (CELL_POSITIVE: a curried L5), law-false-refl.lang (PATH_END: a `refl` case at a false law, the W1 killer), law-no-case.lang (TYPE_MISMATCH: a fold with no path case), law-pure-boundary.lang (TYPE_INFER: `pure` in a boundary, D2), law-no-boundary.lang (HIT_BOUNDARY: an interval field with no boundary, the W4 killer).
- test/emit/law-path-main.lang (JSON_MAIN_TYPE: a `main` of a Path type, the W7 killer).
- test/path-regressions.c, 20 rows: 11 law uses, "law L5 rrName: the law in Virt and its runner path at a variable world (S6)" and the same for the other 10 reads; 6 closed-world rows, "law L1 at a closed world, setSecurity (S6)", L2 setRarity, L3 avatarSet, L3 itemSet, L4 avatarSet, L4 itemSet; 3 two-dimensional rows (the W5 killers), "law 2-dim: the fold path case at an inner point, variable t (S6 W5)", the same at a closed t, and "law L5 2-dim: run of rrBal at an inner point (S6)".
- The codes come from real runs (s6-build/b1.log). No file uses a fixture, thus test/gate-domain.txt does not change.

### Deviations

- D1: examples/laws.lang and test/json/laws.json exist at the pin (the S1 normalization laws). The new example is examples/virt-laws.lang with test/json/virt-laws.json. K7 named laws.lang.
- D2: K7 predicted HIT_BOUNDARY (fold_image, check.c:1273) for `pure` in a boundary. Three forms (s6-build/pure/pA.lang to pC.lang) each give TYPE_INFER: "LzPC: pure needs a declared type Option A, List A, Sum E A or a cell type C ps here". A boundary slot has the carrier type X, thus `pure` has no declared type there, and the check stops before a fold. The row is law-pure-boundary.lang (TYPE_INFER); the count stays 130. The reason for the L4 continuation form is TYPE_INFER, not the fold_image refusal of the S6 kickoff survey c.
- D3: one `main` is at one end only. examples/virt-laws.lang `main` uses rrBal at i0. A scratch copy at i1 (s6-build/run/virt-laws-i1.lang) builds with rc 0, and its JSON is equal (cmp rc 0).

### Open points (S6-build-PROMPT.md)

1. Fuel: no gate file stops at EVAL_FUEL_STEPS. The gate time is not measured.
2. The L5 field names give no REFUSE_NAME.
3. The evals count n is 2 (`readTwo`, `readOne`).
4. The `refl` run case of each of the 11 reads checks at the real `Virt` (each gate file loads the domain), and the 11 law-use rows pass.
5. Q4 (scratch only, s6-build/q4/cycle.lang: the 1-cell `lzQA` names the later 1-cell `lzQB` at ~i, and `lzQB` names `lzQA` at i): rc 1 in about 1 s, NAME_UNKNOWN "LzQC: unknown name lzQB". A boundary cannot name a later 1-cell, thus the 1-cell cycle of Q4 cannot be declared.
6. The 2-dimensional row at `run` passes; K7 (a) stays.
7. F11: `check` accepts a cell whose 1-cell boundary names a later 0-cell when no fold runs over the cell (s6k-mutk/k3b.lang, rc 0). Only a fold (fold_image, check.c:1273) refuses it. This stays open.
8. The S5 open points: 1 (and F4): the laws hold at a known reference; at an unknown reference the two sides refuse at a closed world, and at a variable world they are two different `refUnknown` neutrals (PLAN 5.3). 2: the fold now has 40 cases (the order above). 5: the gate domain loads the 11 1-cells and each gate file checks their path cases (open point 1). 6: the new example has a golden (D1). 8: L3 and L4 depend on the replace of attrSet; in S6 they are closed-world rows only.
9. R1 to R4 (a `List` eliminator or `W` as a list cell; `natEq n n`; the propagation of `refUnknown`; the `textEq` rule): the slice S6R, each route with a probe before code.
10. S5 open point 9 and the S5 mutants U1 to U10 stay as they are (cofamily sites).

### Mutants (K8)

The runner ~/Documents/virtu-lang-work/s6-build/s6-mut.zsh (outside the repo) has the form of the S6 kickoff runner s6k-mut.zsh. Scratch: s6-build/mscratch, an rsync of the main checkout with no .git and no build; logs: s6-build/mout; output: s6-build/s6-mut.log. src/ does not change in S6, thus the sites are the sites of the S6 kickoff (S6-kickoff.md e2). The base build (build/langc and build/path-regressions) has make rc 0 and 255 path regressions with 0 failed. For each mutant: the anchor has exactly 1 match, `sd -F`, `make -B -j 2` with rc 0, the killer on the base binary and then on the mutant binary, the restore, and cmp with the main checkout (rc 0 for each). After the last restore: make rc 0, 255 path regressions with 0 failed, and check.c, eval.c and json.c are equal to the main checkout. Result: 5 of 7 KILLED; W3 and W6 SURVIVED.

| id | site | mutation | killer | base | mutant | result |
|---|---|---|---|---|---|---|
| W1 | src/front/check.c:1397 path_case_type | `val_iend(m, side)` to `val_iend(m, 0)` in `case_image(...)` | test/check/law-false-refl.lang | rc 1 PATH_END: "mzRunA: the path at i1 is fun s => r (natAdd b (natAdd a s)), but the type needs fun s => r (natAdd b s)" | rc 0 | KILLED |
| W2 | check.c:1424 the fold_family selector | `interval_count == 0u ?` to `interval_count == 1u ?` | examples/virt-laws.lang (each gate file that loads the domain) | rc 0 | rc 1 HIT_BOUNDARY: "toOption: a fold of noneC, a constructor with 0 interval fields, is not in S4" | KILLED |
| W3 | check.c:1273 fold_image | `v->inst - base >= done)` to `v->inst - base > done)` | s6k-mutk/k3.lang (a 1-cell that names itself), s6k-mutk/k3b.lang (a 1-cell that names a later 0-cell), s6-build/q4/cycle.lang (open point 5) | k3 rc 1 NAME_UNKNOWN; k3b rc 0; cycle rc 1 NAME_UNKNOWN | the same three results | SURVIVED (likely equivalent) |
| W4 | check.c:3785 check_ctor | `if (points < ctor->field_count && ctor->boundary == NULL)` to `if (0 && ...)` | test/check/law-no-boundary.lang | rc 1 HIT_BOUNDARY: "MzCC: the path constructor mzCL needs a boundary [ ~i -> t0, i -> t1 ] after its interval fields" | rc 0 | KILLED |
| W5 | src/front/eval.c:1013 the path case of fold_family | `acc = fold_path(m, family, cases, ncases, x, acc);` to `acc = (0 && acc != NULL) ? fold_path(...) : acc;` | build/path-regressions; `langc check` of s6-build/rows/18-k5b.lang, 19-k5d.lang, 20-run2.lang | 255 checked, 0 failed; the three rows rc 0 | 10 of 255 failed, with the 3 two-dimensional S6 rows; the three rows rc 1 INTERNAL: "the evaluator found an application of a value that is not a function" | KILLED |
| W6 | check.c:905 path_value_ok | `if (v == NULL \|\| !conv_values(m, level, v, e->args[side]))` to `if (v == NULL)` | s6k-mutk/k6.lang, s6k-mutk/k1.lang, test/check/law-false-refl.lang | rc 1 PATH_END (each) | the same three messages | SURVIVED (equivalent) |
| W7 | src/json.c:996 world_json | `if (!is_virt_unit(m, d->type)) {` to `if (0) {` | `langc build test/emit/law-path-main.lang` | rc 1 JSON_MAIN_TYPE: "main: main has the type PathP (fun _ => Nat) 0 0, not Virt Unit" | rc 1 INTERNAL: "main: the evaluator found a fold of a value that is not in the family" | KILLED |

Notes:

- W3: `v->inst - base == done` is true only when a boundary term of the constructor at slot `done` names that constructor, and the name of a constructor is not in scope in its own boundary (k3.lang). A boundary cannot name a later 1-cell (open point 5), and a later 0-cell gets past `check` when no fold runs (F11, open point 7). Thus W3 is likely an equivalent mutant. This is not proven.
- W5: the 7 other rows that fail on the mutant are older fold rows: "Glue endpoint substitution preserves interval connections", "fold boundary maps a point constructor continuation", "fold boundary maps a point constructor path field", "map through a path cell at a variable interval round-trips", "cell map preserves earlier path and square boundary cases", "cell bind preserves earlier path and square boundary cases", "cong at i1 of a fold with a neutral path case reads the i1 end".
- W6: path_value_ok has one caller, rebuild_ok (check.c:927), and the case that it checks is cell_rebuild (check.c:942), which the kernel writes. The PATH_END of the three killers comes from end_fail (check.c:472) by another caller. Thus W6 is an equivalent mutant at a correct cell_rebuild (the class of S5 U2).

## S6R

S6R does the routes R1 to R4 of PLAN 5.3. It gives the proofs of L1 to L4 at a variable world and adds L1 to L4 as 6 1-cells of `Virt`, with a path case in `run` for each. Thus O23 (a) is complete. The kickoff proposals K1 to K11 and the questions Q1 to Q3 are all (a) (docs/PLAN.md, `### USER ruling (S6R kickoff)`). Two more rulings came during the build: Q-S6R-s9-1 (i) and Q-S6R-s11-1 (a) (PLAN section 1). The claims C26 to C28 are in ~/Documents/virtu-lang-work/S6R-claims.md. The build records are in ~/Documents/virtu-lang-work/s6r-build/ (outside the repo, below: B). src/ changes in check.c, eval.c and json.c.

### Gate (original build)

`make -B -j 2 check` exits 0 (log ~/Documents/virtu-lang-work/s6r-build/s16-gate.log). The part logs come from runs on the main checkout after chunk B: B/s12-main-gate.log (`sh test/gate.sh`, `gate: 0 failures`), B/s12-main-pr.log (path regressions), B/s15-main-regressions.log and B/s15-main-printer.log (rc 0 each). The S6 pin is 0966438 (tree 4a6fea35; gate log s6-build/gate.log).

| gate line | S6 (pin) | K9 | S6R | difference and cause |
|---|---|---|---|---|
| examples | 40 | 41 | 40 | none: no new example (D2) |
| parse refusals | 16 | 16 | 16 | none |
| check refusals | 130 | 133 | 132 | +2: test/check/ref-unknown-program.lang (Q1) and test/check/x6.lang (K4); 2 files of K7 are not in (D3) |
| evals | 212 | 212 + n | 212 | none (n = 0): no new example (D2) |
| json (builds) | 40 | 41 | 40 | none: no new golden (D2) |
| build refusals | 8 | 8 | 8 | none |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 255 | about 273 | 261 | +6 rows (Added tests); the K7 law use and lemma rows are not in (D3) |

Five lines are not equal to the K9 prediction: examples, evals and json (D2), check refusals and path regressions (D3). B/s5-gate-main.log (S6R build s5, the main checkout before K1) has check refusals 131, not 130: test/check/ref-unknown-program.lang came in S6R build s3. The S6 column is the S6 pin.

### K rows

- K1: the world is the list cell `WorldC` (`W := WorldC Unit`; the empty world `worldNil` is `pure unit`), and `Attributes` is the list cell `AttrC`. The list functions over the world and the attributes are folds over the cells. world_json (src/json.c) walks the cells. The JSON goldens did not change: after K1, the `sh test/gate.sh` output of main and of the scratch copy were byte-equal (S6R build s7). The kernel gets no `List` eliminator. The examples change only for the cell world: `nil` to `worldNil` (examples/virt.lang, examples/virt-laws.lang) and `listSize Instance` to `worldCount` (examples/virt.lang).
- K2: the library has a Bool cell, and the compares `natEqB` and `textEqB` give it. The domain has `Bool := Fin 2` already, thus the cell takes new names. The world lemmas split by `elim` on the Bool cell. The kernel gets no rule for `natEq n n`.
- K3: a refusal stays a refusal. The kernel propagates `refUnknown` at a neutral scrutinee (PLAN section 3, R3). The result type comes from the motive. The family fold (`fold_family`, eval.c:1004) has the rule. `elim_family` (eval.c:1158) also has it, at the motive applied to the scrutinee; K3 did not name this site (D4). Conv of two `refUnknown` values is the generic structural conv of stuck values (eval.c:2158, `case OP_REF_UNKNOWN:` with the other stuck ops): it compares the type and the id. K3 said "by their ids". This is a fact, not a defect.
- K4: `textEq t u` gives `some unit` when `t` and `u` are convertible, at the conv level of the machine. test/check/x6.lang is the refusal row: `g (fun x => x)` and `g (fun y => w)` are not convertible, thus the claim is PATH_END. In S6R build s3, a mutant of the conv level (`conv_values(m, 0u, a[0], a[1])` in text_eq) accepted the file (rc 0). The file name comes from that s3 probe. It is not the X6 of the mutant table below.
- K5: the build order was the order of S6R-build-PROMPT.md: chunk A (K4, K3 with Q1), then K2 with K1 in one patch (B/s5-k1-patch.py), then the world lemmas and the 6 1-cells (chunk B, B/s8-b-patch.py). K5 said K2, K1, K4, K3 (D5).
- K6: `Virt` gets 6 1-cells after the 11 L5 1-cells and before `pure`: `wwSec` (L1), `wwRar` (L2), `wwASet` and `wwISet` (L3), `wrASet` and `wrISet` (L4). `Virt` has 46 constructors: 28 0-cells, 17 1-cells and `pure`. The fold of `run` has 46 cases, thus 47 fold arguments with the scrutinee, under SPINE_MAX 64. Each new path case of `run` applies a world lemma at a variable world: `secLaw` (L1), `rarLaw` (L2), `asetLaw` and `isetLaw` (L3), `agetLaw` and `igetLaw` (L4) (domain/domain.lang, defs :480, :544, :711, :723, :737 and :749; run cases :834 to :845). The case of L1 or L2 is `k (lemma ... w @ i)`. The case of L3 or L4 is the lemma path at `i`, with the continuation in the lemma. `attrSet` is restated with the one fold `attrSetF` over the `AttrC` cell (Q-S6R-s9-1 (i), S6R build s10). The other options were (ii) a kernel `Option` elim and (iii) a restriction of the lemmas.
- K7: see Added tests (K7) below.
- K8: see Mutants (K8) below: 6 of 6 KILLED.
- K9: the gate above.
- K10: the PLAN edits: section 1 (`### USER ruling (S6R kickoff)` and the two build rulings); section 3 (a soundness note for each kernel rule; R3 names the fold and `elim` over a domain family); 5.3 (the S6R paragraph with the 6 1-cells, the routes and the world lemmas; the sentence on two different `refUnknown` neutrals is an S6 statement now, and the row `reference: setSecurity twice at a card of the empty world (S6R K3)` shows that the two sides of L1 are convertible at the empty world and a variable card); section 7 (the S6R row, Built).
- K11: the S6 open points 7 and 10 stay. F20 is open point 6 below.
- Q1 (a): `check` refuses `refUnknown` in a program with the new code REFUSE_REF_UNKNOWN. Only the domain library names it. D1 is the hole of the first form.
- Q2 (a): `W` and `Attributes` changed to cells in one slice (K1).
- Q3 (a): the R3 route is K3 (a).
- Q-S6R-s11-1 (a): the arena of `accepts()` in test/path-regressions.c is 512 MB (it was 128 MB), in the test only. After chunk B, the row `law L5 2-dim: run of rrBal at an inner point (S6)` needed more than 128 MB. The cost: the path regressions run of main took 5.96 s with max RSS 285792 before chunk B (S6R build s11) and 15.99 s with max RSS 592384 after chunk B with the 512 MB arena (B/s12-run1.out; zsh `%M` units). The CLI arena (1 GB, src/main.c) does not change: `langc check` of that row as a program (B/s13-lz2.lang) gives rc 0 in 0.29 s, max RSS 414752. The control B/s13-lz2-bad.lang gives rc 1 TYPE_MISMATCH. B/s13-lz2-neg.lang (rc 0) is not a control: `run` at `w` reads the same balance for the two reads, thus the two ends are convertible and the row body `refl` is correct.

### Added tests (K7, original build)

- test/check, 2 files: ref-unknown-program.lang (REFUSE_REF_UNKNOWN: a program names `refUnknown` in a def type, Q1 and D1) and x6.lang (PATH_END: the K4 refusal row).
- test/check/ref-unknown.lang keeps its code. It now gets `refUnknown` through the library only (Q1).
- test/path-regressions.c, 6 rows: the 4 K3 rows "reference: a fold over refUnknown at a variable id is refUnknown (S6R K3)", "reference: refUnknown at a function type, applied, is refUnknown (S6R K3)", "reference: withCard over refUnknown W is refUnknown W (S6R K3)" and "reference: setSecurity twice at a card of the empty world (S6R K3)"; the D1 row "reference: a program cannot name refUnknown in a def type (S6R D1)"; the Q1 row "reference: a program cannot name refUnknown in a def body (S6R Q1)".
- K1 changed the closed worlds of the old rows from `nil` to `worldNil` (for example the world `pzW` of the rows `law L1 to L4 at a closed world (S6)`). No row was removed.
- The 6 world lemmas and the 6 run path cases are in domain/domain.lang. Each gate file loads the domain, thus each gate run checks them. The mutants X1 to X6 are killed at that check.

### Deviations in the original build

- D1 (found in S6R build s2, closed in s3, inside Q1 (a), no ruling needed): the first Q1 refusal fired only in a def body. check_def set the origin of the def after check_type, thus a program that named `refUnknown` in a def type passed. The fix: the Checker has an `origin` field, check_decl sets it at the start of each declaration, and rule_ref_unknown reads it. The D1 row and test/check/ref-unknown-program.lang hold it.
- D2 (closed in staged review below): K7 named one example with a law use at a variable world and its JSON golden. S6R adds no example: examples 40, evals 212 (n = 0) and json 40. The S6R build records (the prompts S6R-build-s2 to s15) give no reason for this. Thus it is a gap against K7 (a), not a ruling (open point 8).
- D3 (closed in staged review below): K7 named tests that are not in. Check refusals: the false list lemma (the r1a-known-nil form, PATH_END) and `elim` on `List` (the r1b form, TYPE_MISMATCH) are not in test/check. x6.lang (K4) is in, and K7 did not name it. Thus 132, not 133 (130 + Q1 + x6.lang; the S6R build s4 note counted 134 with the two files). Path regressions: the 4 K3 rows and the D1 and Q1 rows are in. These K7 rows are not in: the 6 law uses, the 6 world lemmas at a variable world, the r1a-form rows and the R4 rows (r4-kk, r4-attr). The domain check covers the 6 lemmas and the 6 run path cases (Added tests). Thus 261, not about 273 (open point 8).
- D4: K3 named the List fold, the family fold, the Option elim, `finCase` and apply_value. `elim_family` (eval.c:1158) also propagates `refUnknown`. This is an extension of K3. PLAN section 3 R3 names it (S6R build s14).
- D5: the build order is not the K5 order (K5 above).

### Open points (S6R-build-PROMPT.md)

1. The conv level of the K4 rule is the level of the machine. x6.lang refuses the level-0 mutant (K4).
2. The R3 type slot: the motive at the family fold and the result type at a nondependent fold. Conv compares the type and the id (K3).
3. The world lemmas check at each gate file (each gate file loads the domain). The split of each lemma is in domain/domain.lang.
4. Fuel: no gate file stops at EVAL_FUEL_STEPS (the gate has 0 failures). The time of the path regressions run grew (Q-S6R-s11-1 (a)).
5. The JSON goldens are byte-equal after the cell world (K1).
6. F20: for a program, Q1 (a) closes it. The S6R build s4 probes through the library (before K1) each refuse: f20a (`withCard (Path Nat 0 1) nil (cardRef Empty 7) f`, the body of f is `refl`) gives PATH_END; f20b (the body of f is `refUnknown`) gives REFUSE_REF_UNKNOWN; f20c (a large elim on `instanceAt nil 7`, checked at `Path Nat 0 1`) gives EVAL_REF_UNKNOWN; f20d (the same term at the type `m20 (instanceAt nil 7)`) gives EVAL_REF_UNKNOWN. For the terms of the domain library, F20 stays open (PLAN section 3, K11).
7. The S6 open points 7 (F11) and 10 (S5 open point 9, the S5 mutants U1 to U10) stay (K11).
8. D2 and D3 are closed by the staged review below: the example, golden, refusal files and K7 path regression rows are now included.

### Mutants (K8)

The mutant script is B/s12-mutant.py (argv[1] = the root of a scratch copy, argv[2] = X1 to X6). The runner is B/s12-run2.zsh (output B/s12-run2.out; make logs B/s12-mut-X1-make.log to B/s12-mut-X6-make.log). Scratch: B/s12mut, a copy of the main checkout. Each mutant changes one new path case of `run` in domain/domain.lang. For each mutant: the anchor has exactly 1 match, make with rc 0, `langc check` of a tiny program that loads the domain (the domain check is the killer), and the restore. After the last restore: make rc 0, the tiny check gives `ok`, and the domain of the scratch is equal to main (cmp rc 0). Result: 6 of 6 KILLED.

| id | site | mutation | killer | base | mutant | result |
|---|---|---|---|---|---|---|
| X1 | domain/domain.lang:835, the `wwSec` case | `k (secLaw c u v w @ i))` to `k (secLaw c v u w @ i))` | the domain check | rc 0 | rc 1 PATH_END: "run: the path at i0 is ..." | KILLED |
| X2 | domain.lang:837, the `wwRar` case | `k (rarLaw z u v w @ i))` to `k (rarLaw z v u w @ i))` | the domain check | rc 0 | rc 1 PATH_END at i0 | KILLED |
| X3 | domain.lang:839, the `wwASet` case | `asetLaw X a y u v k w @ i)` to `asetLaw X a y v u k w @ i)` | the domain check | rc 0 | rc 1 PATH_END at i0 | KILLED |
| X4 | domain.lang:841, the `wwISet` case | `isetLaw X z y u v k w @ i)` to `isetLaw X z y v u k w @ i)` | the domain check | rc 0 | rc 1 PATH_END at i0 | KILLED |
| X5 | domain.lang:843, the `wrASet` case | the case body `fun (i : I) => fun (w : W) => agetLaw X a y v f w @ i` to `refl` | the domain check | rc 0 | rc 1 PATH_END at i1 | KILLED |
| X6 | domain.lang:845, the `wrISet` case | the case body `fun (i : I) => fun (w : W) => igetLaw X z y v f w @ i` to `refl` | the domain check | rc 0 | rc 1 PATH_END at i1 | KILLED |

Notes:

- X1 to X4 swap the two written values `u` and `v` of a write-write law. X5 and X6 make a write-read case a constant path. The check gives the first end that fails.
- S6R chunk C has no mutant of src/. The kernel rules have rows (Added tests). The S6R build s3 probe of the K4 conv level (K4 above) is the only kernel mutant of S6R.

### Staged review (2026-10-25)

MEDIUM, fixed: quoting a neutral Option elimination or List fold lost the
result type added for K3. Applying the quoted function to a stuck
`refUnknown` then left the eliminator stuck, while applying the original
function propagated the refusal. eval.c now retains the result type in
the neutral value and restores it in `quote_op`. A List fold retains the
type on its neutral tail before applying the constructor steps.

The C regressions compare original and quoted functions for Option,
List and a List with a constructor prefix. They pass on HEAD, fail on the
original staged evaluator and pass with the fix.

D2 and D3 are closed by this review. examples/virt-write-laws.lang has a
write-read law at a variable world, its JSON golden and two eval rows
(both ends observe "east"). The two refusal files are
test/check/world-set-missing.lang (the false empty-world update lemma,
PATH_END) and test/check/list-elim.lang (TYPE_MISMATCH). Path regressions
add the six law uses and six world lemma applications at variable worlds,
two world update/read rows, the two R4 rows, and Option and finCase
refusal propagation with a result type different from the scrutinee type.

Validation: `make -B -j 2 check` exits 0. Examples 41, parse refusals 16,
check refusals 134, evals 214, JSON builds 41, build refusals 8,
regressions 6, printer regressions 2 and path regressions 282. The original
gate and deviations above record the build before this review. No CI
check was removed or weakened. No remaining review blocker.

## S7

S7 is the close slice of PLAN section 7: the marketplace example, the mutants Y1 to Y5 and the docs. The kickoff proposals K1 to K8 and the questions Q-S7-1 to Q-S7-3 are all (a) (docs/PLAN.md, `### USER ruling (S7 kickoff)`). The kickoff record is ~/Documents/virtu-lang-work/S7-kickoff.md. The build records are in ~/Documents/virtu-lang-work/ (outside the repo, below: W). S7 has no change in src/ and domain/.

### Gate

`make -B -j 2 check` exits 0 (log W/s7-build-gate.log). The S6R commit is c654753 (tree 3b6c2dac; gate log W/s7-base-gate.log).

| gate line | S6R commit | K6 | S7 | difference and cause |
|---|---|---|---|---|
| examples | 41 | 42 | 42 | +1: examples/market.lang (K1) |
| parse refusals | 16 | 16 | 16 | none |
| check refusals | 134 | 134 | 134 | none (K4) |
| evals | 214 | 219 | 219 | +5: the market rows (K2) |
| json (builds) | 41 | 42 | 42 | +1: test/json/market.json (K1) |
| build refusals | 8 | 8 | 8 | none |
| regressions | 6 | 6 | 6 | none |
| printer regressions | 2 | 2 | 2 | none |
| path regressions | 282 | 282 | 282 | none (K4) |

Each line is equal to the K6 prediction.

### K rows

- K1 (Q-S7-1 (a)): examples/market.lang is W/s7-probe/market.lang byte for byte (cmp rc 0). The story uses 12 of the 28 0-cells. Ada buys a lamp at the Bazaar (row 21, `purchaseItem`), gives Bo 30 credits (row 16, `transfer`), and Bo buys the rare vase at the Gallery (row 22, `purchase`: the card owner is the buyer). `langc check` gives `ok` (rc 0, 0.06 s real, 62416 max RSS, zsh units). Thus the name `market` and each def name pass R4 (REFUSE_NAME). `langc build examples/market.lang -o test/json/market.json` gives rc 0. The output is byte-equal to the kickoff probe W/s7-market.json (cmp rc 0, 8 instances). The file is not in test/gate-domain.txt: it runs through build/langc, as examples/virt.lang does.
- K2: 5 rows in test/eval/expect.txt (script W/s7-k2.py): `market marketBalances => pair 50 40`, `market marketBags => pair 1 1`, `market marketEntries => pair 3 3`, `market marketShort => 50` (O20 (a): a transfer of 500 gives `none` and no change) and `market marketSize => 8`.
- K3 (Q-S7-2 (a)): no guest probe of the quote route. The C row `reference: quoting option preserves refusal propagation` is the killer of Y5 (open point 3).
- K4: no check refusal file and no path regression row. S7 adds no rule and no refusal.
- K5: see Mutants (K5) below: 5 of 5 KILLED.
- K6: the gate above.
- K7: README.md (the stale line on the base classes and the runner is replaced by the content of S5 to S7), PLAN section 1 (`### USER ruling (S7 kickoff)`), PLAN section 7 (row S7, Built) and this section. PLAN section 8 does not change: each of O1 to O24 occurs in section 1, and `### USER ruling (S0)` rules them all (a).
- K8: see Open points below.
- Q-S7-3 (a): after the S7 commit the plan is CLOSED.

### Added tests

- examples/market.lang and its golden test/json/market.json (examples + 1, json + 1).
- test/eval/expect.txt, 5 rows (K2).

### Deviations

- D1: the S7 build prompt and the kickoff give 36 lines for W/s7-probe/market.lang. The file has 31 lines (`wc -l`). It is byte-equal to the scratch copy that the kickoff probed (W/s7-scratch/probe/market.lang, cmp rc 0), and its build is byte-equal to the probe golden. Thus the count in the prompt is wrong, not the file.

### Mutants (K5)

The mutant script is W/s7-mutant.py (argv[1] = the root of a copy, argv[2] = Y1 to Y4). Y5 is a file copy: src/front/eval.c from the staged tree d0d31636 (the review fix reverted, 22 diff lines). The runner is W/s7-build-mut.zsh, a copy of the kickoff runner W/s7-probes2.zsh with the final example (log W/s7-build-mut.log). Scratch: W/s7-scratch, refreshed from main after K1 and K2 (rsync with no .git; src, domain, examples and test equal to main). Copy: W/s7-mut. For each mutant: apply it, `make -B -j 2` (rc 0), `langc build examples/market.lang` and cmp with test/json/market.json, the 5 market evals, `sh test/gate.sh`. The control Y0 (no mutation): gate.sh rc 0 with 0 FAIL, examples 42, evals 219, json 42; path regressions 282 checked, 0 failed. After the last restore, src and domain of the copy are equal to main (diff rc 0). Result: 5 of 5 KILLED.

| id | site | mutation | killer (gate.sh FAIL lines) | result |
|---|---|---|---|---|
| Y1 | domain/domain.lang:363, `transfer` | `(transferIn Empty)` and `(transferOut Empty)` swapped (the entry kinds of the target and the source) | the market golden only (1 FAIL) | KILLED |
| Y2 | domain.lang:805, `listVirtualItem` | the listing price `creditsMul v (multiplier r)` to `v` | the market golden, `market marketBalances` (`pair 50 70`) and `virt listedValue` (want 40, got 10) (3 FAIL) | KILLED |
| Y3 | domain.lang:812, `purchase` (row 22) | the buyer (the card owner by `withCard`) to `avatarRef Empty 0` | the market golden, `market marketBags` (`pair 2 0`), and examples/virt-write-laws.lang: its check, its 2 evals and its build (EVAL_REF_UNKNOWN in `writeReadRun`) (6 FAIL) | KILLED |
| Y4 | src/json.c:803, `ENUM_NAMES` | the names `"transferOut"` and `"transferIn"` swapped | the market golden only (1 FAIL) | KILLED |
| Y5 | src/front/eval.c, the staged review fix | eval.c from tree d0d31636 (the 3 hunks reverted) | the C row `reference: quoting option preserves refusal propagation` only: path regressions rc 1, "280 checked, 1 failed". gate.sh rc 0, the market golden cmp rc 0 | KILLED |

Notes:

- Y1 and Y4 are killed only by the new golden. Before S7, no golden had a transfer entry. Old tests also kill Y2 and Y3.
- Cost on the copy: gate.sh 28 s to 36 s real, max RSS about 1048400; path regressions 18 s to 19 s real, max RSS 489456 (Y0) and 604672 (Y5) (zsh `%M` units).

### Open points (S7-build-PROMPT.md)

1. S6R open point 6: F20 for the terms of the domain library (PLAN section 3). It stays open (K8).
2. S6R open point 7: the S6 open points 7 (F11) and 10 (S5 open point 9, the S5 mutants U1 to U10). They stay open (K8).
3. Y5: no guest test kills the staged review fix. The kickoff probes b1 (a `cong` line whose path ends have a neutral Option elimination, applied at an unknown reference) and b2 (its control with no `cong`) give `ok` with the fix and with the fix reverted. Only the C row kills it (K3 (a)). It stays open (K8).

The plan is CLOSED after the S7 commit (Q-S7-3 (a)).
