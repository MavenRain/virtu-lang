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
