#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include "front/front.h"
#include "front/check.h"
#include "json.h"

static int cases_run = 0;

static int checked(Arena *arena, Machine *m, Diag *diag, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load(arena, "path-regression", source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int big_fold_fuel(void) {
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag,
    "def r : Nat := fold (fun (acc : Nat) => acc) 0 18446744073709551616\n");
  if (ok) {
    m.fuel = 20u;
    ok = def_value(&m, m.def_count - 1u) == NULL && diag.set && strcmp(diag.code, "EVAL_FUEL") == 0;
  }
  if (!ok) {
    fprintf(stderr, "FAIL a big Nat fold uses the evaluation fuel\n");
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* S4 chunk B: out (unfold g s) spends 1 fuel, then g s spends its own (Q4).
   K is the cost of step at a variable: there out is stuck before the spend.
   A is the cost of g s at the seed of the tail. Thus the second out, the out
   of the tail, gives a layer with K + A + 1 fuel and traps with K + A. */
static int out_fuel(void) {
  Arena arena;
  Machine m;
  Diag diag;
  const Value *step, *layer, *tail;
  uint64_t k = 0u, a = 0u;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag,
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  step = ok ? def_value(&m, m.def_count - 1u) : NULL;
  layer = step != NULL ? apply_value(&m, step, def_value(&m, m.def_count - 2u)) : NULL;
  tail = layer != NULL && val_is(layer, OP_CTOR) && layer->argc == 4u ? layer->args[3] : NULL;
  ok = tail != NULL && val_is(tail, OP_COUNFOLD) && tail->argc == 2u;
  if (ok) {
    m.fuel = EVAL_FUEL_STEPS;
    ok = apply_value(&m, step, val_var(&m, 0u)) != NULL;
    k = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, tail->args[0], tail->args[1]) != NULL;
    a = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = k + a + 1u;
    ok = ok && apply_value(&m, step, tail) != NULL;
  }
  if (ok) {
    m.fuel = k + a;
    ok = apply_value(&m, step, tail) == NULL && diag.set && strcmp(diag.code, "EVAL_FUEL") == 0;
  }
  if (!ok) {
    fprintf(stderr, "FAIL the second out traps when the fuel is 1 step short\n");
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* S4 chunk B, K10 R7: each check of the bisimulation rule spends 1 fuel.
   The case is unfold g w against the variable w, with g = fun s => out s
   (eta). The parts are measured through the API: the seed and result
   types, the final seed comparison; g x,
   the first layer; the head and tail projections of the two layers (the
   cost of shead (out s) less the cost of out s); the conversion of each
   field at the level of the check. The second layer is out x by
   reduce_out: x is a neutral, so it is stuck before the spend and costs
   0 (the API cannot call reduce_out alone). The full cost is 1 for the
   conv_loop iteration, 1 for the check, and the parts. At 1 step short
   the conversion traps. */
static int bisim_fuel(void) {
  Arena arena;
  Machine m;
  Diag diag;
  const Value *w, *x, *u, *h1, *h2, *t1, *t2;
  uint64_t types = 0u, seeds = 0u, app = 0u, step = 0u, ph = 0u, pt = 0u, head = 0u, tail = 0u, t = 0u, model = 0u;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag,
    "def eta : Stream Nat -> Stream Nat := fun (w : Stream Nat) => unfold (fun (s : Stream Nat) => out s) w\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (s : Stream Nat) => out s\n"
    "def hd : Stream Nat -> Nat := fun (s : Stream Nat) => shead (out s)\n"
    "def tl : Stream Nat -> Stream Nat := fun (s : Stream Nat) => stail (out s)\n");
  w = ok ? val_var(&m, 0u) : NULL;
  x = ok ? val_var(&m, 1u) : NULL;
  u = w != NULL ? apply_value(&m, def_value(&m, m.def_count - 4u), w) : NULL;
  ok = u != NULL && x != NULL && val_is(u, OP_COUNFOLD) && u->argc == 2u;
  h1 = ok ? apply_value(&m, def_value(&m, m.def_count - 2u), x) : NULL;
  h2 = ok ? apply_value(&m, def_value(&m, m.def_count - 2u), x) : NULL;
  t1 = ok ? apply_value(&m, def_value(&m, m.def_count - 1u), x) : NULL;
  t2 = ok ? apply_value(&m, def_value(&m, m.def_count - 1u), x) : NULL;
  ok = ok && h1 != NULL && h2 != NULL && t1 != NULL && t2 != NULL;
  if (ok) {
    m.fuel = EVAL_FUEL_STEPS;
    ok = conv_values(&m, 1u, u->arg, u->dom);
    types = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && conv_values(&m, 1u, w, w);
    seeds = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, u->args[0], x) != NULL;
    app = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, def_value(&m, m.def_count - 3u), x) != NULL;
    step = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, def_value(&m, m.def_count - 2u), x) != NULL;
    ph = EVAL_FUEL_STEPS - m.fuel - step;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, def_value(&m, m.def_count - 1u), x) != NULL;
    pt = EVAL_FUEL_STEPS - m.fuel - step;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && conv_values(&m, 2u, h1, h2);
    head = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && conv_values(&m, 2u, t1, t2);
    tail = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && conv_values(&m, 1u, u, w);
    t = EVAL_FUEL_STEPS - m.fuel;
    model = 2u + types + seeds + app + 2u * (ph + pt) + head + tail;
    ok = ok && t == model;
  }
  if (ok) {
    m.fuel = t;
    ok = conv_values(&m, 1u, u, w);
  }
  if (ok) {
    m.fuel = t - 1u;
    ok = !conv_values(&m, 1u, u, w) && diag.set && strcmp(diag.code, "EVAL_FUEL") == 0;
  }
  if (!ok) {
    fprintf(stderr, "FAIL the bisimulation check spends 1 fuel (t %llu, model %llu: types %llu, seeds %llu, g x %llu, head proj %llu, tail proj %llu, head %llu, tail %llu)\n",
      (unsigned long long)t, (unsigned long long)model, (unsigned long long)types, (unsigned long long)seeds, (unsigned long long)app,
      (unsigned long long)ph, (unsigned long long)pt, (unsigned long long)head, (unsigned long long)tail);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int accepts(const char *label, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* SOURCE after the extra domain text DOMAIN, which can declare a family (a
   program cannot: REFUSE_DATA). */
static int domain_checked(Arena *arena, Machine *m, Diag *diag, const char *domain, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load_domain(arena, "path-regression", domain, source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int domain_accepts(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* 1 when DOMAIN and SOURCE fail and the first diagnostic has CODE. */
static int domain_refuses(const char *label, const char *domain, const char *source, const char *code) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = !domain_checked(&arena, &m, &diag, domain, source) && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected %s\n", label, code);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int system_conversion(void) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  Value lines[2] = {{0}, {0}};
  Value negative;
  IConj conjunction;
  uint32_t negative_lit = 1;
  const Value *points[2];
  const Value *args[6];
  const Value *i;
  const Value *ni;
  const Value *original;
  const Value *other;
  int ok;
  uint32_t k;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, "def seed : Nat := 0\n");
  if (ok) {
    for (k = 0; k < 2u; k++) {
      points[k] = val_nat(&m, k);
      lines[k].kind = VAL_PLAM;
      lines[k].tag = CLOS_CONST;
      lines[k].args = &points[k];
      lines[k].argc = 1;
    }
    i = val_ivar(&m, 0);
    negative = *i;
    conjunction.lits = &negative_lit;
    conjunction.count = 1;
    negative.conj = &conjunction;
    ni = &negative;
    args[0] = i; args[1] = &lines[0];
    args[2] = ni; args[3] = &lines[1];
    original = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    args[0] = ni; args[1] = &lines[1];
    args[2] = i; args[3] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = conv_values(&m, 2, original, other);
    args[1] = &lines[0]; args[3] = &lines[1];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = ok && !conv_values(&m, 2, original, other);
    args[0] = i; args[1] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 2);
    ok = ok && !conv_values(&m, 2, original, other);
    args[2] = ni; args[3] = &lines[1];
    args[4] = face_meet(&m, i, val_ivar(&m, 1)); args[5] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 6);
    ok = ok && conv_values(&m, 2, original, other) && !diag.set;
  }
  if (!ok) {
    fprintf(stderr, "FAIL system conversion: order, values, coverage or refinement\n");
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int domain_roundtrip(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m, replay;
  Diag diag;
  char type[8192], value[8192], program[32768];
  int ok;
  uint32_t index;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  index = ok ? m.def_count - 1u : 0u;
  if (ok)
    ok = value_print(&m, NULL, 0, m.defs[index].type, type, sizeof type)
      && value_print(&m, NULL, 0, def_value(&m, index), value, sizeof value);
  if (ok) {
    snprintf(program, sizeof program, "def replay : %s := %s\n", type, value);
    ok = domain_checked(&arena, &replay, &diag, domain, program);
    if (!ok) fprintf(stderr, "%s\n", program);
  }
  if (ok)
    ok = conv_values(&m, 0, m.defs[index].type, replay.defs[replay.def_count - 1u].type)
      && conv_values(&m, 0, def_value(&m, index), def_value(&replay, replay.def_count - 1u));
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int roundtrip(const char *label, const char *source) {
  return domain_roundtrip(label, "", source);
}

static int domain_json_contains(const char *label, const char *domain, const char *source, const char *expected) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  const char *text = NULL;
  size_t len = 0;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source)
    && json_document(&m, &text, &len)
    && strstr(text, expected) != NULL;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected JSON containing %s\n", label, expected);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* 1 when DOMAIN and SOURCE check and json_document refuses with CODE. */
static int domain_json_refuses(const char *label, const char *domain, const char *source, const char *code) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  const char *text = NULL;
  size_t len = 0;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source) && !json_document(&m, &text, &len)
    && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected %s\n", label, code);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int json_contains(const char *label, const char *source, const char *expected) {
  return domain_json_contains(label, "", source, expected);
}

/* Same-name instances of the same type must retain distinct path data. */
static int distinct_path_json(const char *label, const char *first, const char *second) {
  Arena arena;
  Machine a, b;
  Diag diag;
  const char *left = NULL;
  const char *right = NULL;
  size_t left_len = 0, right_len = 0;
  uint32_t index;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &a, &diag, first) && checked(&arena, &b, &diag, second);
  index = ok ? a.def_count - 1u : 0u;
  if (ok)
    ok = a.def_count == b.def_count
      && conv_values(&a, 0, a.defs[index].type, b.defs[index].type)
      && !conv_values(&a, 0, def_value(&a, index), def_value(&b, index)) && !diag.set
      && json_document(&a, &left, &left_len) && json_document(&b, &right, &right_len)
      && (left_len != right_len || strcmp(left, right) != 0);
  if (!ok) {
    fprintf(stderr, "FAIL %s: distinct paths need distinct JSON\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* A failed limb allocation must not turn the digit span into UINT64_MAX. */
static int nat_literal_oom(void) {
  char digits[200001];
  Term type = {.kind = TERM_VAR, .name = "Nat"};
  Term body = {.kind = TERM_NAT, .name = digits, .nat = UINT64_MAX};
  Decl decl = {.kind = DECL_DEF, .origin = ORIGIN_PROGRAM,
    .name = "large", .type = &type, .body = &body};
  DeclList decls = {.items = &decl, .count = 1, .cap = 1};
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  cases_run++;
  memset(digits, '9', sizeof digits - 1u);
  digits[sizeof digits - 1u] = '\0';
  arena_init(&arena, (size_t)1 << 16);
  diag_init(&diag);
  ok = !check_program(&arena, &decls, &m, &diag)
    && diag.set && strcmp(diag.code, "OOM") == 0;
  if (!ok) {
    fprintf(stderr, "FAIL big Nat allocation failure: expected OOM, got %s\n",
      diag.set ? diag.code : "an accepted truncated literal");
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  int failed = 0;
  signal(SIGPIPE, SIG_IGN);
  failed += !nat_literal_oom();
  failed += !accepts("big Nat literal composition",
    "def same : PathP (fun (j : I) => Path Nat (hcomp Nat j (fun (_ : I) => [ j -> 18446744073709551616 ]) 18446744073709551616) 18446744073709551616) refl refl := fun (j : I) => refl\n");
  failed += !system_conversion();
  failed += !accepts("Glue composition with reordered faces",
    "def G : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (j : I) => G @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same : PathP (fun (j : I) => Path (G @ j) (hcomp (G @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ j, j -> g @ j ]) (g @ j)) (g @ j)) refl refl := fun (j : I) => refl\n");
  failed += !accepts("Glue composition under a stronger face",
    "def G0 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g0 : PathP (fun (j : I) => G0 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G1 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g1 : PathP (fun (j : I) => G1 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G : Path (Path (Type 0) (Fin 3) (Fin 3)) G0 G1 := fun (i : I) => fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j /\\ i -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (i : I) => PathP (fun (j : I) => G @ i @ j) 0 0) g0 g1 := fun (i : I) => fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same0 : PathP (fun (j : I) => Path (G0 @ j) (hcomp (G0 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g0 @ j, j -> g0 @ j ]) (g0 @ j)) (g0 @ j)) refl refl := fun (j : I) => refl\n"
    "def same1 : PathP (fun (j : I) => Path (G1 @ j) (hcomp (G1 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g1 @ j, j -> g1 @ j ]) (g1 @ j)) (g1 @ j)) refl refl := fun (j : I) => refl\n"
    "def same : PathP (fun (i : I) => PathP (fun (j : I) => Path (G @ i @ j) (hcomp (G @ i @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ i @ j, j -> g @ i @ j ]) (g @ i @ j)) (g @ i @ j)) refl refl) same0 same1 := fun (i : I) => fun (j : I) => refl\n");
  failed += !roundtrip("PathP binder capture",
    "def original : Sigma (PathP : Nat) (Path Nat PathP PathP) := pack 0 refl\n");
  failed += !roundtrip("anonymous funExt argument",
    "def original : (h : (_ : Nat) -> Path Nat 0 0) -> Path (Nat -> Nat) (fun n => 0) (fun n => 0) := fun h => funExt h\n");
  failed += !roundtrip("application of a path of functions",
    "def original : (p : Path (Nat -> Nat) (fun n => n) (fun n => n)) -> Path Nat 0 0 := fun p => fun (i : I) => (p @ i) 0\n");
  failed += !roundtrip("dependent congruence",
    "def original : (p : Path (Charter Nat) (genesis Nat) (genesis Nat)) -> PathP (fun (i : I) => Ratified Nat (p @ i)) (ratify Nat (genesis Nat) 7) (ratify Nat (genesis Nat) 7) := fun p => cong (fun (c : Charter Nat) => ratify Nat c 7) p\n");
  failed += !roundtrip("dependent symmetry",
    "def original : (A : Path (Type 0) Nat Nat) -> (p : PathP (fun (j : I) => A @ j) 0 1) -> PathP (fun (i : I) => A @ ~i) 1 0 := fun A p => sym p\n");
  failed += !roundtrip("dependent function extensionality",
    "def original : (A : Path (Type 0) Nat Nat) -> (h : (n : Nat) -> PathP (fun (j : I) => A @ j) n n) -> PathP (fun (i : I) => Nat -> A @ i) (fun n => n) (fun n => n) := fun A h => funExt h\n");
  failed += !roundtrip("stuck transport fill",
    "def original : PathP (fun (i : I) => (p : Path (Type 0) Nat Nat) -> p @ i) (fun (p : Path (Type 0) Nat Nat) => 0) (fun (p : Path (Type 0) Nat Nat) => transport Nat Nat p 0) := fun (i : I) => fun (p : Path (Type 0) Nat Nat) => transp (fun (j : I) => p @ (i /\\ j)) (~i) 0\n");
  failed += !roundtrip("stuck hcomp with two keys",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path Nat n n := fun n p => fun (j : I) => hcomp Nat (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> p @ i ]) (p @ j)\n");
  failed += !roundtrip("stuck transport in a Path line",
    "def original : (A : Type 0) -> (x : A) -> (y : A) -> (r : Path A x y) -> (p : Path A x x) -> Path A y y := fun A x y r p => transp (fun (i : I) => Path A (r @ i) (r @ i)) i0 p\n");
  failed += !roundtrip("dead constructor branch",
    "def original : (x : Prod Nat Nat) -> (n : Nat) -> (p : Path Nat n n) -> Path (Prod Nat Nat) (pair n 0) (pair n 0) := fun x n p => fun (j : I) => hcomp (Prod Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> pair n 0, j -> pair (p @ i) 0, i0 -> x ]) (pair n 0)\n");
  failed += !roundtrip("dependent Sigma composition",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path (Sigma (x : Nat) (Path Nat x x)) (pack n refl) (pack n refl) := fun n p => fun (j : I) => hcomp (Sigma (x : Nat) (Path Nat x x)) (~j \\/ j) (fun (i : I) => [ ~j -> pack n refl, j -> pack (p @ i) refl ]) (pack n refl)\n");
  failed += !roundtrip("universe function composition",
    "def original : (p : Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat)) -> Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat) := fun p => fun (j : I) => hcomp (Nat -> Type 0) (~j \\/ j) (fun (i : I) => [ ~j -> (fun n => Nat), j -> p @ i ]) (fun n => Nat)\n");
  failed += !roundtrip("universe Sigma composition",
    "def original : (p : Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat)) -> Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat) := fun p => fun (j : I) => hcomp (Sigma (x : Nat) (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pack 0 Nat, j -> p @ i ]) (pack 0 Nat)\n");
  failed += !roundtrip("universe product composition",
    "def original : (p : Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat)) -> Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat) := fun p => fun (j : I) => hcomp (Prod Nat (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pair 0 Nat, j -> pair 0 Nat ]) (pair 0 Nat)\n");
  failed += !roundtrip("universe path composition",
    "def original : (p : Path (Path (Type 0) Nat Nat) refl refl) -> Path (Path (Type 0) Nat Nat) refl refl := fun p => fun (j : I) => hcomp (Path (Type 0) Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> refl, j -> p @ i ]) refl\n");
  failed += !roundtrip("transport of a universe path",
    "def original : (p : Path (Type 0) Nat Nat) -> Path (Type 0) Nat Nat := fun p => transp (fun (_ : I) => Path (Type 0) Nat Nat) i0 p\n");
  failed += !roundtrip("face action on a local type",
    "def original : (P : Path (Type 0) Nat Nat) -> PathP (fun (j : I) => (n : P @ j) -> P @ j) (fun (n : Nat) => n) (fun (n : Nat) => n) := fun P => fun (j : I) => fun (n : P @ j) => hcomp (P @ j) (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> n ]) n\n");
  failed += !roundtrip("a branch on a disjunctive face",
    "def original : (p : Path Nat 0 1) -> PathP (fun (j : I) => Path Nat (p @ j) (p @ j)) refl refl := fun p => fun (j : I) => hcomp (Path Nat (p @ j) (p @ j)) (~j \\/ j) (fun (i : I) => [ ~j \\/ j -> refl ]) refl\n");
  failed += !roundtrip("stuck Fin addition",
    "def original : (r : Fin 11) -> Fin 11 := fun (r : Fin 11) => finAdd 11 r 3\n");
  /* S5a item 2: a finAll def prints as finCase and round-trips. */
  failed += !roundtrip("finite case function printing",
    "def original : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite case path printing",
    "def p : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n"
    "def original : Path (Fin 2 -> Fin 2) (fun k => finAdd 2 k 0) (fun k => k) := funExt p\n");
  failed += !roundtrip("finite case beyond the ordinary spine limit",
    "def original : (k : Fin 63) -> Path (Fin 63) (finAdd 63 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite function case in an application",
    "def choose : Fin 2 -> Nat -> Nat := fun k => finCase 2 k (fun n => natAdd n 1) (fun n => natAdd n 2)\n"
    "def original : Fin 2 -> Nat -> Nat := fun k n => choose k n\n");
  failed += !accepts("finite case refines dependent local types",
    "def f : (k : Fin 2) -> (x : finCase 2 k Nat (Fin 3)) -> Nat := fun k x => finCase 2 k x (finVal 3 x)\n"
    "def zero : Path Nat (f 0 7) 7 := refl\n"
    "def one : Path Nat (f 1 2) 2 := refl\n");
  /* S5a item 5: a finCase at a closed scrutinee checks at the top level
     (the empty env is not an allocation failure). */
  failed += !accepts("finite case at a closed literal scrutinee",
    "def a : Path Nat (finCase 2 1 0 1) 1 := refl\n"
    "def b : Nat := finCase 2 (finEq 3 1 1) 0 1\n");
  failed += !roundtrip("Glue type at a neutral face",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (f : Equiv A A) -> Path (Type 0) A A := fun A e f => fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A f ])\n");
  failed += !roundtrip("stuck unglue",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (x : A) -> (y : A) -> (g : PathP (fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A e ])) x y) -> Path A (witness e x) (witness e y) := fun A e x y g => fun (i : I) => unglue (g @ i)\n");
  failed += !roundtrip("univalence at a Glue endpoint",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def original : G := transport G G (ua G G (idEquiv G)) u\n");
  failed += !roundtrip("unglue at a nested Glue face",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def p : PathP (fun (i : I) => Glue G (~i \\/ i) (fun (_ : I) => [ ~i -> pack G (idEquiv G), i -> pack G (idEquiv G) ])) u u := fun (i : I) => glue (fun (_ : I) => [ ~i -> u, i -> u ]) u\n"
    "def original : PathP (fun (i : I) => Path G (unglue (p @ i)) u) refl refl := fun (i : I) => refl\n");
  failed += !json_contains("transport table with function points",
    "def singletonFunPath : (h : Fin 1 -> Fin 1) -> (x : Fin 1) -> Path (Fin 1) (h 0) (h x) := fun h x => refl\n"
    "def toFun : Fin 1 -> (Fin 1 -> Fin 1) := fun x => fun (_ : Fin 1) => x\n"
    "def fromFun : (Fin 1 -> Fin 1) -> Fin 1 := fun h => h 0\n"
    "def toFromFun : (h : Fin 1 -> Fin 1) -> Path (Fin 1 -> Fin 1) (toFun (fromFun h)) h := fun h => fun (i : I) => fun x => singletonFunPath h x @ i\n"
    "def fromToFun : (x : Fin 1) -> Path (Fin 1) (fromFun (toFun x)) x := fun x => refl\n"
    "def finToFun : Path (Type 0) (Fin 1) (Fin 1 -> Fin 1) := ua (Fin 1) (Fin 1 -> Fin 1) (isoToEquiv (Fin 1) (Fin 1 -> Fin 1) toFun fromFun toFromFun fromToFun)\n",
    "\"transport\":[\"fun x => 0\"]");
  failed += !json_contains("stuck transport table uses path syntax",
    "def neutralLoop : Path (Type 0) (Fin 1) (Fin 1) := hcomp (Path (Type 0) (Fin 1) (Fin 1)) i0 (fun (_ : I) => [ ]) refl\n",
    "\"term\":\"fun i =>");
  failed += !domain_accepts("path constructor declaration",
    "family S1x := basex | loopx (i : I) [ ~i -> basex, i -> basex ]\n",
    "def p : Path S1x (loopx i0) (loopx i1) := fun (i : I) => loopx i\n");
  failed += !domain_refuses("path constructor boundary of a wrong type",
    "family S1x := basex | loopx (i : I) [ ~i -> 0, i -> basex ]\n",
    "def u : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("path constructor boundary that does not cover",
    "family S1x := basex | loopx (i : I) [ ~i -> basex ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  failed += !domain_refuses("path constructor with no boundary",
    "family S1x := basex | loopx (i : I)\n",
    "def u : Nat := 0\n", "HIT_BOUNDARY");
  failed += !domain_json_contains("path tree fallback after a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch leaf (loop j) j\n",
    "\"value\":{\"from\":{\"tag\":\"leaf\"},\"to\":{\"tag\":\"leaf\"},\"term\":\"fun j => branch leaf (loop j) j\"}}]}");
  failed += !domain_json_contains("path tree with a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch (branch leaf base j) base j\n",
    "\"value\":{\"path\":\"branch\",\"args\":[{\"path\":\"branch\",\"args\":[{\"tag\":\"leaf\"},\"base\"]},\"base\"]}}]}");
  failed += !roundtrip("formal hcomp at a HIT",
    "def original : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !distinct_path_json("formal hcomp retains its tube",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> base ]) (loop j)\n",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !roundtrip("path constructor at a neutral interval",
    "def original : PathP (fun (i : I) => Path S1 base (loop i)) refl (fun (j : I) => loop j) := fun (i : I) => fun (j : I) => loop (i /\\ j)\n");
  failed += !roundtrip("Glue endpoint substitution preserves interval connections",
    "def code : S1 -> S1 -> Type 0 := fun (y : S1) => fun (x : S1) =>\n"
    "  fold (Path S1 y y) (ua (Path S1 y y) (Path S1 y y) (idEquiv (Path S1 y y))) x\n"
    "def transportSelf : (y : S1) -> Path S1 y y := fun (y : S1) =>\n"
    "  transp (fun (i : I) => code y (loop i)) i0 (fun (_ : I) => y)\n"
    "def p : PathP (fun (j : I) => Path S1 (loop (j /\\ ~j)) (loop (j /\\ ~j)))\n"
    "  (transportSelf base) (transportSelf base) := fun (j : I) => transportSelf (loop (j /\\ ~j))\n");
  failed += !roundtrip("transport of a formal hcomp at Trunc1 along a parameter line",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (a : A) -> (b : A) ->\n"
    "  Path (Trunc1 B) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (a : A) => fun (b : A) => fun (j : I) =>\n"
    "  transp (fun (i : I) => Trunc1 (p @ i)) i0 (hcomp (Trunc1 A) (~j \\/ j)\n"
    "    (fun (i : I) => [ ~j -> trunc1 A a, j -> squash1 A (trunc1 A b) (trunc1 A a) i ]) (squash1 A (trunc1 A a) (trunc1 A b) j))\n");
  failed += !domain_roundtrip("transport preserves a parameter-dependent HIT boundary",
    "family ParamHit (a : Trunc1 (Fin 3)) := paramPoint (paramValue : Trunc1 (Fin 3)) |\n"
    "  paramLoop (i : I) [ ~i -> paramPoint a a, i -> paramPoint a a ]\n",
    "def endpoint : ParamHit (trunc1 (Fin 3) 1) :=\n"
    "  transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramPoint (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 0))\n"
    "def original : Path (ParamHit (trunc1 (Fin 3) 1)) endpoint endpoint :=\n"
    "  fun (j : I) => transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramLoop (trunc1 (Fin 3) 0) j)\n");
  /* S4c item 17 (R7 (a)): the transport of a dimension-2 constructor along
     a parameter line stays stuck. Its boundary branches are path
     applications, not recursive fields, so a field rebuild needs a
     boundary correction (R7 (c)). */
  failed += !roundtrip("transport of squash0 along a parameter line stays stuck",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) =>\n"
    "  fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j)\n");
  failed += !domain_refuses("transport of squash0 is not the field rebuild", "",
    "def same : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)))\n"
    "    (fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j))\n"
    "    (fun (i : I) => fun (j : I) => squash0 B (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y)\n"
    "      (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (u @ k)) (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (v @ k)) i j) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) => refl\n", "PATH_UIP");
  failed += !domain_refuses("dimension-2 boundary that does not cover the square",
    "family Sq2 (A : Type 0) := sq2Point (sq2Of : A)\n"
    "  | sq2Square (sq2From : Sq2 A) (sq2To : Sq2 A) (sq2Left : Path (Sq2 A) sq2From sq2To) (sq2Right : Path (Sq2 A) sq2From sq2To) (i : I) (j : I)\n"
    "    [ ~i -> sq2Left @ j, i -> sq2Right @ j, ~j -> sq2From ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  /* Generated cell constructors must print as ordinary source syntax. */
  failed += !roundtrip("cell pure round-trips",
    "def original : OptionC Nat := pure 3\n");
  failed += !roundtrip("polymorphic cell pure round-trips",
    "def original : (V : Type 0) -> V -> Either Nat V := fun (V : Type 0) (v : V) => pure v\n");
  failed += !roundtrip("nested cell pure round-trips",
    "def original : ListC Nat (OptionC Nat) := consC Nat (OptionC Nat) 2 (pure (pure 3))\n");
  failed += !roundtrip("neutral cell bind round-trips",
    "def original : OptionC Nat -> OptionC Nat := fun (o : OptionC Nat) => bind o (fun (n : Nat) => pure (succ n))\n");
  /* Boundary images must map continuation and path fields as evaluation does. */
  failed += !domain_roundtrip("fold boundary maps a point constructor continuation",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (i : I) [ ~i -> hcNode (fun (n : Nat) => hcLeaf), i -> hcLeaf ]\n",
    "def original : Path Nat 0 0 := fun (i : I) => fold 0 (fun (k : Nat -> Nat) => k 0) refl (hcLoop i)\n");
  failed += !domain_roundtrip("elim boundary supplies a point constructor continuation IH",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (i : I) [ ~i -> hcNode (fun (n : Nat) => hcLeaf), i -> hcLeaf ]\n",
    "def original : Path Nat 0 0 := fun (i : I) => elim (fun (x : HC) => Nat) 0 (fun (k : Nat -> HC) (h : Nat -> Nat) => h 0) refl (hcLoop i)\n");
  failed += !domain_roundtrip("fold boundary maps an applied continuation field",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (hcArg : Nat -> HC) (i : I) [ ~i -> hcNode hcArg, i -> hcNode hcArg ]\n",
    "def original : HC -> Nat := fun (x : HC) => fold 0 (fun (k : Nat -> Nat) => k 0) (fun (k : Nat -> Nat) => refl) x\n");
  failed += !domain_roundtrip("elim boundary maps an applied continuation field to its IH",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (hcArg : Nat -> HC) (i : I) [ ~i -> hcNode hcArg, i -> hcNode hcArg ]\n",
    "def original : HC -> Nat := fun (x : HC) => elim (fun (y : HC) => Nat) 0 (fun (k : Nat -> HC) (h : Nat -> Nat) => h 0) (fun (k : Nat -> HC) (h : Nat -> Nat) => refl) x\n");
  failed += !domain_refuses("fold continuation boundary still checks unequal ends",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (i : I) [ ~i -> hcNode (fun (n : Nat) => hcLeaf), i -> hcLeaf ]\n",
    "def original : Nat := fold 0 (fun (k : Nat -> Nat) => 1) refl hcLeaf\n", "PATH_END");
  failed += !domain_refuses("elim continuation boundary still checks unequal ends",
    "family HC := hcLeaf | hcNode (hcK : Nat -> HC) | hcLoop (i : I) [ ~i -> hcNode (fun (n : Nat) => hcLeaf), i -> hcLeaf ]\n",
    "def original : Nat := elim (fun (x : HC) => Nat) 0 (fun (k : Nat -> HC) (h : Nat -> Nat) => 1) refl hcLeaf\n", "PATH_END");
  failed += !domain_roundtrip("fold boundary maps a point constructor path field",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY) | hhLoop (i : I) [ ~i -> hhNode hhBase hhBase refl, i -> hhNode hhBase hhBase refl ]\n",
    "def original : Path (Fin 2) 0 0 := fun (t : I) => fold 0 (fun (x : Fin 2) (y : Fin 2) (p : Path (Fin 2) x y) => p @ t) refl (hhLoop t)\n");
  failed += !domain_roundtrip("fold square boundary preserves captured intervals in path fields",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY)\n"
    "  | hhSquare (hhU : HH) (hhV : Path HH hhU hhU) (i : I) (j : I) [ ~i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)),\n"
    "      i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)), ~j -> hhNode hhU hhU refl, j -> hhNode hhU hhU refl ]\n",
    "def original : HH -> HH := fun (z : HH) => fold hhBase (fun (x : HH) (y : HH) (p : Path HH x y) => hhNode x y p) (fun (x : HH) (p : Path HH x x) => fun (a : I) => fun (b : I) => hhSquare x p a b) z\n");
  /* S3 K10: cell source round trips with a boundary. The ends of each path
     come from the cell system, so the printed type must check again. */
  failed += !roundtrip("Trunc1C squash path round-trips",
    "def original : (a : Trunc1C Nat) -> (b : Trunc1C Nat) -> Path (Trunc1C Nat) a b := "
    "fun (a : Trunc1C Nat) (b : Trunc1C Nat) => fun (i : I) => squash1C Nat a b i\n");
  failed += !roundtrip("Trunc0C squash square round-trips",
    "def original : (a : Trunc0C Nat) -> (b : Trunc0C Nat) -> (p : Path (Trunc0C Nat) a b) -> (q : Path (Trunc0C Nat) a b) -> "
    "Path (Path (Trunc0C Nat) a b) p q := "
    "fun (a : Trunc0C Nat) (b : Trunc0C Nat) (p : Path (Trunc0C Nat) a b) (q : Path (Trunc0C Nat) a b) => "
    "fun (i : I) => fun (j : I) => squash0C Nat a b p q i j\n");
  failed += !roundtrip("QuotC relation path round-trips",
    "def original : (R : Nat -> Nat -> Type 0) -> (a : Nat) -> (b : Nat) -> (r : R a b) -> "
    "Path (QuotC Nat R Nat) (qinC Nat R Nat a) (qinC Nat R Nat b) := "
    "fun (R : Nat -> Nat -> Type 0) (a : Nat) (b : Nat) (r : R a b) => fun (i : I) => quotEqC Nat R Nat a b r i\n");
  failed += !roundtrip("StateC law path with a continuation round-trips",
    "def original : (t : StateC Nat Nat) -> Path (StateC Nat Nat) (getC Nat Nat (fun (s : Nat) => putC Nat Nat s t)) t := "
    "fun (t : StateC Nat Nat) => fun (i : I) => gpC Nat Nat t i\n");
  /* S3 P6: map through a path cell at a variable interval keeps the interval
     binder of the rebuilt case. */
  failed += !roundtrip("map through a path cell at a variable interval round-trips",
    "def original : (a : Trunc1C Nat) -> (b : Trunc1C Nat) -> "
    "Path (Trunc1C Nat) (map (fun (n : Nat) => natAdd n 1) a) (map (fun (n : Nat) => natAdd n 1) b) := "
    "fun (a : Trunc1C Nat) (b : Trunc1C Nat) => fun (i : I) => map (fun (n : Nat) => natAdd n 1) (squash1C Nat a b i)\n");
  /* S3 P5: a square fold through a neutral case at neutral intervals. The
     printed form applies the case at i, then j; with the order swapped, the
     replay fails at the ends of the declared square. */
  failed += !roundtrip("square fold through a neutral case keeps the face order",
    "def f0 : ((a : Trunc0 Nat) -> (b : Trunc0 Nat) -> (p : Path (Trunc0 Nat) a b) -> (q : Path (Trunc0 Nat) a b) -> Path (Path (Trunc0 Nat) a b) p q) -> Trunc0C Nat -> Trunc0 Nat := "
    "fun (sq : (a : Trunc0 Nat) -> (b : Trunc0 Nat) -> (p : Path (Trunc0 Nat) a b) -> (q : Path (Trunc0 Nat) a b) -> Path (Path (Trunc0 Nat) a b) p q) (t : Trunc0C Nat) => "
    "fold sq (fun (v : Nat) => trunc0 Nat v) t\n"
    "def original : (sq : (a : Trunc0 Nat) -> (b : Trunc0 Nat) -> (p : Path (Trunc0 Nat) a b) -> (q : Path (Trunc0 Nat) a b) -> Path (Path (Trunc0 Nat) a b) p q) -> "
    "(a : Trunc0C Nat) -> (b : Trunc0C Nat) -> (p : Path (Trunc0C Nat) a b) -> (q : Path (Trunc0C Nat) a b) -> "
    "Path (Path (Trunc0 Nat) (f0 sq a) (f0 sq b)) (fun (k : I) => f0 sq (p @ k)) (fun (k : I) => f0 sq (q @ k)) := "
    "fun (sq : (a : Trunc0 Nat) -> (b : Trunc0 Nat) -> (p : Path (Trunc0 Nat) a b) -> (q : Path (Trunc0 Nat) a b) -> Path (Path (Trunc0 Nat) a b) p q) "
    "(a : Trunc0C Nat) (b : Trunc0C Nat) (p : Path (Trunc0C Nat) a b) (q : Path (Trunc0C Nat) a b) => "
    "fun (i : I) => fun (j : I) => f0 sq (squash0C Nat a b p q i j)\n");
  /* A square boundary can contain earlier paths and squares at neutral
     intervals. Map their cases, including the endpoint paths of a square. */
  {
    const char *coherence =
      "cell Coher2 (X : Type 0) := c2b : (c2bx : X) -> X\n"
      " | c2l : (c2lx : X) -> (i : I) -> X [ ~i -> c2b X c2lx, i -> c2b X c2lx ]\n"
      " | c2s : (c2sx : X) -> (i : I) -> (j : I) -> X [ ~i -> c2l X c2sx j, i -> c2l X c2sx j, ~j -> c2b X c2sx, j -> c2b X c2sx ]\n"
      " | c2t : (c2tx : X) -> (i : I) -> (j : I) -> X [ ~i -> c2s X c2tx j j, i -> c2l X c2tx j, ~j -> c2b X c2tx, j -> c2b X c2tx ]\n";
    failed += !domain_roundtrip("cell map preserves earlier path and square boundary cases", coherence,
      "def f2 : Coher2 Nat -> Coher2 Bool := fun (t : Coher2 Nat) => map (fun (n : Nat) => true) t\n"
      "def original : (x : Coher2 Nat) -> Path (Path (Coher2 Bool) (c2b Bool (f2 x)) (c2b Bool (f2 x))) "
      "(fun (j : I) => c2s Bool (f2 x) j j) (fun (j : I) => c2l Bool (f2 x) j) := "
      "fun (x : Coher2 Nat) => fun (i : I) => fun (j : I) => f2 (c2t Nat x i j)\n");
    failed += !domain_roundtrip("cell bind preserves earlier path and square boundary cases", coherence,
      "def f2 : Coher2 Nat -> Coher2 Bool := fun (t : Coher2 Nat) => bind t (fun (n : Nat) => pure true)\n"
      "def original : (x : Coher2 Nat) -> Path (Path (Coher2 Bool) (c2b Bool (f2 x)) (c2b Bool (f2 x))) "
      "(fun (j : I) => c2s Bool (f2 x) j j) (fun (j : I) => c2l Bool (f2 x) j) := "
      "fun (x : Coher2 Nat) => fun (i : I) => fun (j : I) => f2 (c2t Nat x i j)\n");
  }
  failed += !domain_refuses("earlier path boundary still checks unequal square faces",
    "cell Disk (X : Type 0) := db : X\n"
    " | dl : (i : I) -> X [ ~i -> db X, i -> db X ]\n"
    " | ds : (i : I) -> (j : I) -> X [ ~i -> dl X j, i -> db X, ~j -> db X, j -> db X ]\n",
    "def bad : Disk Nat -> S1 := fun (t : Disk Nat) => fold base (fun (i : I) => loop i) "
    "(fun (i : I) => fun (j : I) => loop j) (fun (n : Nat) => base) t\n", "PATH_UIP");
  /* S5b item 7: the refutation of decEqFin is a closure. Its printed form
     must check again at the declared type (J15: Fin 0 and Unit swapped in
     the line fail with TYPE_MISMATCH at i0). */
  failed += !domain_roundtrip("decEqFin inr round-trips", "",
    "def d : Dec (Path (Fin 3) 0 1) := decEqFin 3 0 1\n");
  failed += !roundtrip("decEqNat inr round-trips",
    "def d : Dec (Path Nat 0 1) := decEqNat 0 1\n");
  /* S6a item 1 (R13 (a)): a glue element is neutral for a Nat operation.
     The transport back along the ua line gives finAdd on a glue element;
     the operation stays stuck and rule 6 prints the term (S5c: INTERNAL). */
  failed += !json_contains("Nat operation on a glue element stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 (finAdd 3 (glue (fun (_ : I) => [ ~i -> 0, i -> 1 ]) 1) 1) 2)");
  /* Both value operands of finAdd must stay neutral on Glue elements. */
  failed += !json_contains("finAdd right Glue operand stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 1 r) (fun (r : Fin 3) => finAdd 3 2 r) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 2 (finAdd 3 1 (glue");
  /* S6b item 6, rule 2: the components 1 and 2 are different, so each
     part must come from its own component path. */
  failed += !json_contains("Sigma path prints each component",
    "def p : Path ((e : Fin 2) * Fin 3) (pack 1 2) (pack 1 2) := refl\n",
    "\"value\":{\"witness\":{\"refl\":1},\"payload\":{\"refl\":2}}");
  /* S6b item 6, rule 3 at a ua domain: the pointwise path at x goes from
     f x to g (s x), so the table starts at x = 0 with 1 (asymmetric). */
  failed += !json_contains("path of functions prints a table of paths",
    "def l : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def q : (z : Fin 1) -> PathP (fun (i : I) => (l @ i) -> Fin 3) (fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) := "
    "fun (z : Fin 1) => uaArrow (Fin 3) (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl)) "
    "(fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) (fun (w : Fin 3) => refl)\n"
    "def p : Path ((A : Type 0) * (A -> Fin 3)) (pack (Fin 3) (fun (x : Fin 3) => finAdd 3 x 1)) (pack (Fin 3) (fun (x : Fin 3) => x)) := "
    "fun (i : I) => pack (l @ i) ((q 0) @ i)\n",
    "\"payload\":{\"paths\":[{\"from\":1,\"to\":1,\"term\":");
  /* S6b item 6, rule 1 at the Sigma carrier: a payload-major order gives
     [0,3,1,4,2,5]. */
  failed += !json_contains("ua table at a Sigma carrier",
    "def C : Type 0 := (e : Fin 2) * Fin 3\n"
    "def lift : (x : Fin 3) -> Fin 6 := fun (x : Fin 3) => finCase 3 x 0 1 2\n"
    "def fwd : (p : C) -> Fin 6 := fun (p : C) => finCase 2 (witness p) (lift (payload p)) (finAdd 6 (lift (payload p)) 3)\n"
    "def bwd : (k : Fin 6) -> C := fun (k : Fin 6) => finCase 6 k (pack 0 0) (pack 0 1) (pack 0 2) (pack 1 0) (pack 1 1) (pack 1 2)\n"
    "def sec : (k : Fin 6) -> Path (Fin 6) (fwd (bwd k)) k := fun (k : Fin 6) => refl\n"
    "def retC : (e : Fin 2) -> (x : Fin 3) -> Path C (bwd (fwd (pack e x))) (pack e x) := fun (e : Fin 2) => fun (x : Fin 3) => refl\n"
    "def retAll : (p : C) -> Path C (bwd (fwd p)) p := fun (p : C) => retC (witness p) (payload p)\n"
    "def u : Path (Type 0) C (Fin 6) := ua C (Fin 6) (isoToEquiv C (Fin 6) fwd bwd sec retAll)\n",
    "\"value\":{\"from\":\"Sigma (e : Fin 2) (Fin 3)\",\"to\":\"Fin 6\",\"transport\":[0,1,2,3,4,5]}");
  /* Empty literal carriers still select the table rules. */
  failed += !json_contains("empty Fin universe path prints an empty table",
    "def p : Path (Type 0) (Fin 0) (Fin 0) := refl\n",
    "\"value\":{\"from\":\"Fin 0\",\"to\":\"Fin 0\",\"transport\":[]}");
  failed += !json_contains("empty Sigma base prints an empty transport table",
    "def C : Type 0 := (e : Fin 0) * Fin 2\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("empty Sigma fibers print an empty transport table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("path of functions on Fin zero prints an empty table",
    "def f : Fin 0 -> Nat := fun (x : Fin 0) => 0\n"
    "def p : Path (Fin 0 -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  failed += !json_contains("path of functions on an empty Sigma prints an empty table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  /* Every Sigma fiber must be literal before either table rule applies.
     The second fiber here is infinite, beyond the finite first fiber. */
  failed += !json_contains("mixed Sigma carrier keeps the whole universe path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"value\":{\"refl\":\"Sigma ");
  failed += !json_contains("mixed Sigma function domain keeps the whole path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"refl\":\"fun x => 0\"}");
  /* Literal dependent fibers can have different sizes, including zero. */
  failed += !json_contains("dependent Sigma transport enumerates every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[{\"witness\":0,\"payload\":0},{\"witness\":2,\"payload\":0},{\"witness\":2,\"payload\":1}]}");
  failed += !json_contains("dependent Sigma function paths enumerate every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def f : C -> Nat := fun (x : C) => finVal 3 (witness x)\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[{\"refl\":0},{\"refl\":2},{\"refl\":2}]}");
  /* S2 chunk A (orbit-lang S6 B1 to B4 and S7 B1 rows): indexed families,
     mutual blocks and the elim check. The texts come from orbit
     test/fronttool.c with Size as Nat, sz0 as 0, szS as succ, Unit as Nat
     and fresh names beside the domain names. The fold values of the L/R
     row are checked and not reduced here: the block fold eval is chunk B. */
  failed += !domain_refuses("indexed family: an end at another index",
    "family Vx : Nat -> Type 0 := vnilx : Vx 0 | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize) "
    "| vbadx (badxSize : Nat) (badxTail : Vx badxSize) (i : I) : Vx (succ badxSize) "
    "[ ~i -> vconsx badxSize badxTail, i -> badxTail ]\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("indexed family: a constructor without a result",
    "family Vx : Nat -> Type 0 := vnilx | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize)\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("indexed family: a result head that is not the family",
    "family Vx : Nat -> Type 0 := vnilx : Nat | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize)\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("block: a header names a later sort",
    "family P0 : Q0 -> Type 0 := p0 : P0 q0 and Q0 := q0\n",
    "def u : Nat := 0\n", "BLOCK_ORDER");
  failed += !domain_accepts("block: a field names a later sort",
    "family Ch := gen | adoptx (adoptxOf : Ch) (adoptxBy : Rt adoptxOf) and Rt : Ch -> Type 0 := rat (ratOf : Ch) : Rt ratOf\n",
    "def u : Nat := 0\n");
  failed += !domain_refuses("block fold: an extra argument before the value",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def bad : Nat := fold (Even -> Nat) Nat (fun (e : Even) => 0) (fun (u : Nat) => fun (e : Even) => 1) "
    "(fun (g : Even -> Nat) => 0) 0 ez\n", "TYPE_MISMATCH");
  failed += !domain_refuses("reserved header: a fold of the sort under check",
    "family Ax := ax (axN : Nat) and Bx (x : Ax) (y : fold Nat x) := bx\n",
    "def value : Bx (ax 0) 0 := bx (ax 0) 0\n", "HIT_BOUNDARY");
  failed += !domain_refuses("block universe: at Type 0",
    "family Low := lowx (highx : High) and High := highCtor (big : Type 0)\n",
    "def small : Type 0 := Low\n", "TYPE_MISMATCH");
  failed += !domain_accepts("block universe: at Type 1",
    "family Low := lowx (highx : High) and High := highCtor (big : Type 0)\n",
    "def large : Type 1 := Low\n");
  failed += !domain_accepts("higher-order index telescope: 1",
    "family Fx (A : Type 0) : ((Gx : Type 0) -> Gx -> Gx) -> Type 0 := mkx (mkxArg : (T : Type 0) -> T -> T) : Fx A mkxArg and Gx := gx\n",
    "def u : Nat := 0\n");
  failed += !domain_accepts("higher-order index telescope: 2",
    "family Fx (Gx : Type 0) : Gx -> Type 0 := mkx (mkxArg : Gx) : Fx Gx mkxArg and Gx := gx\n",
    "def u : Nat := 0\n");
  failed += !domain_accepts("block fold: the same parameter telescope",
    "family Lx (A : Type 0) (a : A) := leftLx (lxValue : A) and Rx (A : Type 0) (a : A) := rightRx (rxLeft : Lx A a)\n",
    "def firstL : Nat := fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (leftLx Nat 7 8)\n"
    "def secondR : Nat := fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (rightRx Nat 7 (leftLx Nat 7 8))\n");
  failed += !domain_refuses("block fold: a different parameter telescope",
    "family Fx (A : Type 0) := makeFx (fxValue : A) and Gx (n : Nat) := makeGx\n",
    "def result : Nat := fold Nat Nat (fun (x : Nat) => 0) 0 (makeGx 7)\n", "TYPE_MISMATCH");
  failed += !domain_accepts("elim: a partial family eliminator",
    "family Nx := zerox | succx (predx : Nx)\n",
    "def elimId : Nx -> Nx := elim (fun (n : Nx) => Nx) zerox (fun n ih => succx ih)\n");
  failed += !domain_accepts("elim: mixed motive annotations",
    "family Nx := zerox | succx (predx : Nx)\n"
    "family Vx : Nx -> Type 0 := nilVx : Vx zerox | consVx (lenx : Nx) (tailVx : Vx lenx) : Vx (succx lenx)\n",
    "def elimMixed : (n : Nx) -> Vx n -> Nx := fun n v => elim (fun k => fun (t : Vx k) => Nx) zerox (fun k t ih => succx ih) v\n");
  /* S2 chunk B (orbit-lang S6 B3, S6 B4 and S7 B1 rows): the indexed and
     block fold eval, the fold print order and the transport round trips.
     The texts come from orbit test/fronttool.c with Size as Nat, sz0 as 0,
     szS as succ, Unit as Nat, unit as 0 and fresh names beside the domain
     names. */
  failed += !domain_accepts("indexed fold: index arguments are not parameters",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0 | vcx (itemx : A) : Vx A (succ 0) "
    "| segx (i : I) : Vx A 0 [ ~i -> vnx A, i -> vnx A ]\n",
    "def result : Path Nat (fold 0 (fun a => a) refl (vcx Nat 7)) 7 := refl\n");
  failed += !domain_accepts("block fold: Even/Odd one",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def one : Path Nat (fold Nat Nat 0 (fun (u : Nat) => 1) (fun (n : Nat) => 0) (es (os ez))) 1 := refl\n");
  failed += !domain_accepts("block fold: Even/Odd two",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def two : Path Nat (fold Nat Nat 0 (fun (u : Nat) => natAdd u 1) (fun (n : Nat) => natAdd n 1) (es (os ez))) 2 := refl\n");
  failed += !domain_accepts("block fold: Box, a sibling recursive field at its target",
    "family Boxx := boxx (boxxLeft : Itm) (boxxRight : Itm) (boxxPath : Path Itm boxxLeft boxxRight) and Itm := itm\n",
    "def result : Path Nat 0 0 := fun (i : I) => fold Nat Nat (fun x y p => p @ i) 0 (boxx itm itm refl)\n");
  failed += !domain_accepts("block fold: L/R, the folds reduce",
    "family Lx (A : Type 0) (a : A) := leftLx (lxValue : A) and Rx (A : Type 0) (a : A) := rightRx (rxLeft : Lx A a)\n",
    "def firstL : Path Nat (fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (leftLx Nat 7 8)) 8 := refl\n"
    "def secondR : Path Nat (fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (rightRx Nat 7 (leftLx Nat 7 8))) 0 := refl\n");
  failed += !domain_accepts("transport: the fibre of a transport result",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0 "
    "| vcx (vcxLen : Nat) (vcxHead : A) (vcxTail : Vx A vcxLen) : Vx A (succ vcxLen)\n",
    "def fibre : Vx Nat (succ 0) := transp (fun (i : I) => Vx Nat (succ 0)) i0 (vcx Nat 0 7 (vnx Nat))\n");
  failed += !domain_roundtrip("block fold: the print order of the targets, Even -> Nat",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def printEven : Even -> Nat := fun x => fold Nat Nat 0 (fun u => 1) (fun n => 0) x\n");
  failed += !domain_roundtrip("block fold: the print order of the targets, Odd -> Nat",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def printOdd : Odd -> Nat := fun x => fold Nat Nat 0 (fun u => 1) (fun n => 0) x\n");
  failed += !domain_roundtrip("transport round trip: a constant line at Vec",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0\n",
    "def line : Vx Nat 0 := transp (fun (i : I) => Vx Nat 0) i0 (vnx Nat)\n");
  failed += !domain_roundtrip("transport round trip: an index line",
    "family Nx := zx | sx (px : Nx) | edgex (i : I) [ ~i -> zx, i -> sx zx ]\n"
    "family Vx : Nx -> Type 0 := vnx : Vx zx | vcx : Vx (sx zx)\n",
    "def line : Vx (sx zx) := transp (fun (i : I) => Vx (edgex i)) i0 vnx\n");
  failed += !domain_roundtrip("transport round trip: Even/Odd",
    "family Even (A : Type 0) := ez | es (oddPrev : Odd A) and Odd (A : Type 0) := os (evenPrev : Even A)\n",
    "def line : Even Nat := transp (fun (i : I) => Even Nat) i0 (es Nat (os Nat (ez Nat)))\n");
  failed += !domain_json_contains("indexed constructors retain only parameters in their field environment", "",
    "def vector : Vec Nat 1 := vcons Nat 0 7 (vnil Nat)\n",
    "\"value\":{\"tag\":\"vcons\",\"consLen\":0,\"consHead\":7,\"consTail\":{\"tag\":\"vnil\"}}");
  failed += !domain_json_contains("indexed enum constructor JSON",
    "family Ix : Nat -> Type 0 := ix : Ix 0\n",
    "def result : Ix 0 := ix\n", "\"value\":\"ix\"");
  failed += !domain_json_contains("indexed sibling constructor JSON", "",
    "def result : Ratified Nat (genesis Nat) := ratify Nat (genesis Nat) 7\n",
    "\"value\":{\"ratifyBase\":{\"tag\":\"genesis\"},\"ratifyRule\":7}");
  failed += !domain_json_contains("HIT JSON traverses recursive fields at their sibling sort",
    "family Boxx := boxx (boxxItem : Itm) "
    "| boxxLoop (boxxAt : Itm) (i : I) [ ~i -> boxx boxxAt, i -> boxx boxxAt ] and Itm := itm\n",
    "def result : Path Boxx (boxx itm) (boxx itm) := fun (i : I) => boxxLoop itm i\n",
    "\"value\":{\"path\":\"boxxLoop\",\"args\":[\"itm\"]}");
  /* The world rows run against the library (chunk B): Virt, Instance, W
     and run. JSON_MAIN_RUN cannot occur while the domain has run. */
  failed += !domain_json_contains("world writer: no main keeps the S1 to S4 form", "",
    "def seed : Nat := 3\n",
    "{\"virtu-lang\":1,\"instances\":[{\"name\":\"seed\",\"type\":\"Nat\",\"value\":3}]}\n");
  failed += !domain_json_contains("world writer: main at pure unit gives the empty world", "",
    "def main : Virt Unit := pure unit\n", "{\"virtu-lang\":1,\"instances\":[]}\n");
  failed += !domain_json_contains("world writer: id, class and state in table order", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  createVirtucard Unit a 100 (fun (c : VirtuCard) => attachCard Unit a c (setSecurity Unit c (high Empty)\n"
    "  (terminal Unit \"Bazaar\" \"tools\" (fun (t : VirtualTerminal) => listItem Unit t \"Lamp\" 20 \"a lamp\"\n"
    "  (avatarSet Unit a \"guild\" \"north\" (purchase Unit t \"Lamp\" c (fun (o : Option VirtualItem) => pure unit))))))))\n",
    "{\"virtu-lang\":1,\"instances\":["
    "{\"id\":0,\"class\":\"Avatar\",\"state\":{\"name\":\"Ada\",\"role\":\"explorer\",\"virtucard\":1,"
    "\"attributes\":{\"guild\":\"north\"},\"inventory\":[3]}},"
    "{\"id\":1,\"class\":\"VirtuCard\",\"state\":{\"owner\":0,\"balance\":80,\"security_level\":\"high\","
    "\"history\":[{\"kind\":\"create\",\"amount\":100,\"balance\":100},{\"kind\":\"purchase\",\"amount\":20,\"balance\":80}]}},"
    "{\"id\":2,\"class\":\"VirtualTerminal\",\"state\":{\"name\":\"Bazaar\",\"category\":\"tools\",\"listings\":[]}},"
    "{\"id\":3,\"class\":\"VirtualItem\",\"state\":{\"name\":\"Lamp\",\"type\":\"tools\",\"base_value\":20,"
    "\"rarity\":\"common\",\"attributes\":{}}}]}\n");
  failed += !domain_json_refuses("world writer: main of a type that is not Virt Unit", "",
    "def main : Virt Nat := pure 0\n", "JSON_MAIN_TYPE");
  failed += !domain_json_contains("world writer: an avatar with no card", "",
    "def main : Virt Unit := avatar Unit \"Bo\" merchant (fun (b : Avatar) => pure unit)\n",
    "{\"virtu-lang\":1,\"instances\":[{\"id\":0,\"class\":\"Avatar\",\"state\":{\"name\":\"Bo\",\"role\":\"merchant\","
    "\"virtucard\":null,\"attributes\":{},\"inventory\":[]}}]}\n");
  failed += !domain_json_contains("world writer: two avatars, two cards and a transfer", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  createVirtucard Unit a 100 (fun (c : VirtuCard) => avatar Unit \"Bo\" merchant (fun (b : Avatar) =>\n"
    "  createVirtucard Unit b 10 (fun (d : VirtuCard) => attachCard Unit a c (attachCard Unit b d\n"
    "  (transfer Unit c d 30 (fun (o : Option Unit) => pure unit)))))))\n",
    "{\"virtu-lang\":1,\"instances\":["
    "{\"id\":0,\"class\":\"Avatar\",\"state\":{\"name\":\"Ada\",\"role\":\"explorer\",\"virtucard\":1,\"attributes\":{},\"inventory\":[]}},"
    "{\"id\":1,\"class\":\"VirtuCard\",\"state\":{\"owner\":0,\"balance\":70,\"security_level\":\"low\","
    "\"history\":[{\"kind\":\"create\",\"amount\":100,\"balance\":100},{\"kind\":\"transfer-out\",\"amount\":30,\"balance\":70}]}},"
    "{\"id\":2,\"class\":\"Avatar\",\"state\":{\"name\":\"Bo\",\"role\":\"merchant\",\"virtucard\":3,\"attributes\":{},\"inventory\":[]}},"
    "{\"id\":3,\"class\":\"VirtuCard\",\"state\":{\"owner\":2,\"balance\":40,\"security_level\":\"low\","
    "\"history\":[{\"kind\":\"create\",\"amount\":10,\"balance\":10},{\"kind\":\"transfer-in\",\"amount\":30,\"balance\":40}]}}]}\n");
  /* The bound builtin (S5) at `langc build`: the def body is evaluated and the
     JSON writer gives the number. The "credits:" rows test it at a conversion. */
  failed += !domain_json_contains("bound builtin: build writes creditsCheck 2^64 - 1 as a JSON number", "",
    "def top : Nat := creditsCheck 18446744073709551615\n",
    "{\"virtu-lang\":1,\"instances\":[{\"name\":\"top\",\"type\":\"Nat\",\"value\":18446744073709551615}]}\n");
  failed += !domain_json_refuses("bound builtin: build refuses creditsCheck 2^64", "",
    "def over : Nat := creditsCheck 18446744073709551616\n", "EVAL_CREDITS_OVERFLOW");
  /* The reference refusal: id 0 is an Avatar, not a VirtuCard. */
  failed += !domain_json_refuses("reference refusal: a card reference at an avatar instance", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  cardBalance Unit (cardRef Empty 0) (fun (b : Credits) => pure unit))\n", "EVAL_REF_UNKNOWN");
  failed += !domain_json_refuses("reference refusal: a self transfer still validates its card", "",
    "def main : Virt Unit := transfer Unit (cardRef Empty 0) (cardRef Empty 0) 1 (fun (o : Option Unit) => pure unit)\n",
    "EVAL_REF_UNKNOWN");
  failed += !domain_json_refuses("reference refusal: insufficient funds still validates the target class", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  createVirtucard Unit a 0 (fun (c : VirtuCard) => transfer Unit c (cardRef Empty 0) 1 (fun (o : Option Unit) => pure unit)))\n",
    "EVAL_REF_UNKNOWN");
  failed += !domain_json_refuses("reference refusal: a missing listing still validates the buyer", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  createVirtucard Unit a 0 (fun (c : VirtuCard) => terminal Unit \"Shop\" \"tools\" (fun (t : VirtualTerminal) =>\n"
    "  purchaseItem Unit t (avatarRef Empty 99) \"missing\" c (fun (o : Option VirtualItem) => pure unit))))\n",
    "EVAL_REF_UNKNOWN");
  failed += !domain_json_refuses("reference refusal: a missing listing still validates the card", "",
    "def main : Virt Unit := avatar Unit \"Ada\" explorer (fun (a : Avatar) =>\n"
    "  terminal Unit \"Shop\" \"tools\" (fun (t : VirtualTerminal) =>\n"
    "  purchaseItem Unit t a \"missing\" (cardRef Empty 99) (fun (o : Option VirtualItem) => pure unit)))\n",
    "EVAL_REF_UNKNOWN");
  /* The main lookup is by the full name, not by the place or a prefix. */
  failed += !domain_json_contains("main lookup: main after another def writes the world", "",
    "def mainly : Nat := 3\n"
    "def main : Virt Unit := avatar Unit \"Bo\" merchant (fun (b : Avatar) => pure unit)\n",
    "{\"virtu-lang\":1,\"instances\":[{\"id\":0,\"class\":\"Avatar\",\"state\":{\"name\":\"Bo\",\"role\":\"merchant\","
    "\"virtucard\":null,\"attributes\":{},\"inventory\":[]}}]}\n");
  failed += !domain_json_contains("main lookup: a name with the prefix main is not main", "",
    "def mainly : Nat := 3\n",
    "{\"virtu-lang\":1,\"instances\":[{\"name\":\"mainly\",\"type\":\"Nat\",\"value\":3}]}\n");
  /* The Text escapes: each escape of a string literal, and the JSON form. */
  failed += !domain_json_contains("Text escapes: a string literal to JSON", "",
    "def t : Text := \"a\\\"b\\\\c\\nd\\te\\x41\"\n",
    "{\"virtu-lang\":1,\"instances\":[{\"name\":\"t\",\"type\":\"Text\",\"value\":\"a\\u0022b\\u005cc\\u000ad\\u0009eA\"}]}\n");
  failed += !domain_refuses("elim cannot use unfinished constructor slots in a field type",
    "family Xx := zxx | packxx (pxx : Xx) (qxx : Path Nat (elim (fun (v : Xx) => Nat) 0 0 0 pxx) 0) "
    "| edgexx (i : I) [ ~i -> zxx, i -> zxx ]\n",
    "def p : Path Xx (packxx zxx refl) (packxx zxx refl) := fun (i : I) => packxx (edgexx i) refl\n",
    "HIT_BOUNDARY");
  failed += !domain_refuses("elim cannot use unfinished constructor slots in a boundary",
    "family Xx := zxx | edgexx (i : I) [ ~i -> zxx, i -> elim (fun (v : Xx) => Xx) zxx zxx zxx ]\n",
    "def result : Nat := 0\n", "HIT_BOUNDARY");
  failed += !domain_refuses("diagnostic printing skips unnamed constructor slots",
    "family Ax := ax (axType : Type 0) "
    "and Bx : (elim (fun (x : Ax) => Type 0) Nat (ax Nat)) -> Type 0 := bx (bxVal : Nat) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_accepts("block header uses the settled universe of an earlier sort",
    "family Ax := ax (axType : Type 0) "
    "and Bx : ((fun (T : Type 1) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Type 1 := Ax\n");
  failed += !domain_accepts("block field uses the settled universe of a later sort",
    "family Ax := ax (axField : Bx ((fun (T : Type 1) => Nat) (Bx Nat))) "
    "and Bx : Type 0 -> Type 0 := bx (bxType : Type 0) : Bx bxType\n",
    "def result : Type 1 := Ax\n");
  failed += !domain_refuses("settled block headers keep exact universe equality",
    "family Ax := ax (axType : Type 0) "
    "and Bx : ((fun (T : Type 0) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("settled block fields keep exact universe equality",
    "family Ax := ax (axField : Bx ((fun (T : Type 0) => Nat) (Bx Nat))) "
    "and Bx : Type 0 -> Type 0 := bx (bxType : Type 0) : Bx bxType\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("settled block universes are not cumulative",
    "family Ax := ax (axVal : Nat) "
    "and Bx : ((fun (T : Type 1) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_roundtrip("elim motive inference distinguishes a case from the scrutinee",
    "family Fx := fx0 | fx1 family Gx := gx\n",
    "def runx : Fx -> Gx := fun (x : Fx) => elim (fun (u : Fx) => Gx) gx gx x\n");
  failed += !domain_accepts("bare partial elim supplies the scrutinee before selecting a family",
    "family Fx := fx0 | fx1 family Gx := gx\n",
    "def runx : Fx -> Gx := elim (fun u => Gx) gx gx\n");
  failed += !domain_roundtrip("elim motive inference validates arguments after a function result",
    "family Fx := fx0 | fx1 family Gx := gx family Hx := hx0 | hx1 | hx2\n",
    "def runx : Fx -> Hx -> Gx := fun (x : Fx) (y : Hx) => "
    "elim (fun (u : Fx) => Hx -> Gx) (fun (z : Hx) => gx) (fun (z : Hx) => gx) x y\n");
  failed += !domain_roundtrip("indexed dependent elim motive round trip", "",
    "def identityProof : (n : Nat) -> (v : Vec Nat n) -> Path (Vec Nat n) v v := fun (n : Nat) (v : Vec Nat n) => "
    "elim (fun (k : Nat) (w : Vec Nat k) => Path (Vec Nat k) w w) refl "
    "(fun l h t ih => refl) (fun l x y r ih => fun (i : I) => refl) v\n");
  /* S2 chunk C, the M7 guest test: a fold at a neutral quotEq case g
     stores its ends in the PAPP. A cong line quotes them, and the line at
     i1 reads the stored i1 end (trunc0 2, not the i0 end trunc0 a). */
  failed += !domain_accepts("cong at i1 of a fold with a neutral path case reads the i1 end", "",
    "def Eq4 : Fin 4 -> Fin 4 -> Type 0 := fun (a : Fin 4) (b : Fin 4) => Path (Fin 4) a b\n"
    "def selfPath : (x : Trunc0 (Fin 4)) -> Path (Trunc0 (Fin 4)) x x := fun (x : Trunc0 (Fin 4)) => refl\n"
    "def foldG : ((a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) -> Quot (Fin 4) Eq4 -> Trunc0 (Fin 4) := "
    "fun (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) (z : Quot (Fin 4) Eq4) => "
    "fold (fun (a : Fin 4) => trunc0 (Fin 4) a) g "
    "(fun (x : Trunc0 (Fin 4)) (y : Trunc0 (Fin 4)) (p : Path (Trunc0 (Fin 4)) x y) (q : Path (Trunc0 (Fin 4)) x y) => fun (i : I) => fun (j : I) => squash0 (Fin 4) x y p q i j) z\n"
    "def atEnd : (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) -> (a : Fin 4) -> (r : Path (Fin 4) a 2) -> "
    "Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) 2) (trunc0 (Fin 4) 2) := "
    "fun (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) (a : Fin 4) (r : Path (Fin 4) a 2) => "
    "cong selfPath (fun (i : I) => foldG g (quotEq (Fin 4) Eq4 a 2 r i)) @ i1\n");
  /* S2 chunk C, the N4 killer: fold_res reads the target of the sort of a
     block fold. Odd has a path constructor, so an hcomp at Odd is formal,
     and the fold of it composes at the Odd target Fin 2, not at the Even
     target Nat. The round trip prints the hcomp type and checks it again. */
  failed += !domain_roundtrip("block fold of a formal hcomp at the second sort composes at its own target",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even) | oloop (i : I) : Odd [ ~i -> os ez, i -> os ez ]\n",
    "def oddHcomp : (o : Odd) -> Path (Fin 2) (fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) o) "
    "(fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) o) := "
    "fun (o : Odd) => fun (j : I) => fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) "
    "(hcomp Odd (~j \\/ j) (fun (i : I) => [ ~j -> o, j -> o ]) o)\n");
  /* S3 chunk B: a path at a base type. A big Nat sum normalizes. */
  failed += !roundtrip("a big Nat sum is the literal 2^64",
    "def r : Path Nat (natAdd 18446744073709551615 1) 18446744073709551616 := refl\n");
  failed += !roundtrip("decEqNat at equal big Nats",
    "def r : Sum (Path Nat 18446744073709551616 18446744073709551616) "
    "(Path Nat 18446744073709551616 18446744073709551616 -> Empty) := decEqNat 18446744073709551616 18446744073709551616\n");
  failed += !roundtrip("decEqNat at different big Nats of the same width",
    "def r : Sum (Path Nat 18446744073709551616 18446744073709551617) "
    "(Path Nat 18446744073709551616 18446744073709551617 -> Empty) := decEqNat 18446744073709551616 18446744073709551617\n");
  /* The removed bounded List unfold stays refused at big and neutral
     bounds, including the former partial-evaluation case. */
  failed += !domain_refuses("the removed bounded unfold refuses a big bound", "",
    "def r : List Nat := unfold (fun (s : Nat) => none) (natAdd 18446744073709551615 1) 0\n", "TYPE_INFER");
  failed += !domain_refuses("the removed bounded unfold refuses a neutral step", "",
    "def r : (Nat -> Option (Prod Nat Nat)) -> List Nat := fun (step : Nat -> Option (Prod Nat Nat)) => "
    "unfold step 18446744073709551616 0\n", "TYPE_INFER");
  failed += !domain_refuses("the removed bounded unfold refuses a partially evaluated step", "",
    "def r : (step : Nat -> Option (Prod Nat Nat)) -> Path (List Nat) "
    "(unfold (fun (s : Nat) => finCase 2 (natEq s 0) (step s) (some (pair 7 1))) 18446744073709551616 0) "
    "(cons 7 (unfold (fun (s : Nat) => finCase 2 (natEq s 0) (step s) (some (pair 7 1))) 18446744073709551615 1)) "
    ":= fun (step : Nat -> Option (Prod Nat Nat)) => refl\n", "TYPE_INFER");
  failed += !roundtrip("decEqNat at a big Nat and a word with the same limb count",
    "def r : Sum (Path Nat 18446744073709551616 3) "
    "(Path Nat 18446744073709551616 3 -> Empty) := decEqNat 18446744073709551616 3\n");
  failed += !big_fold_fuel();
  failed += !accepts("big arithmetic carries and borrows through every limb",
    "def a : Path Nat (succ 79228162514264337593543950335) 79228162514264337593543950336 := refl\n"
    "def b : Path Nat (natSub 79228162514264337593543950336 1) 79228162514264337593543950335 := refl\n"
    "def c : Path Nat (natMul 18446744073709551616 18446744073709551616) 340282366920938463463374607431768211456 := refl\n"
    "def d : Path Nat (natSub 18446744073709551616 1) 18446744073709551615 := refl\n"
    "def e : Path Nat (natMul 18446744073709551616 0) 0 := refl\n");
  /* S4 chunk A: the cofamilies. A refused layer is extra domain text, not
     a change to domain/domain.lang. */
  failed += !domain_refuses("cofamily: the body is not a domain family",
    "cofamily Pairs (A : Type 0) := Prod A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: an indexed layer",
    "family IxF (A : Type 0) (X : Type 0) : Nat -> Type 0 := ixz : IxF A X 0\n"
    "cofamily Ix (A : Type 0) := IxF A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: a layer with a path constructor",
    "family CircF (A : Type 0) (X : Type 0) := cbase | cloop (i : I) [ ~i -> cbase A X, i -> cbase A X ]\n"
    "cofamily Circ (A : Type 0) := CircF A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: X in the domain of a Pi field",
    "family BackF (A : Type 0) (X : Type 0) := back (backRead : X -> A)\n"
    "cofamily Back (A : Type 0) := BackF A\n", "", "CODATA_POSITIVE");
  failed += !domain_refuses("cofamily: X under List",
    "family RoseF (A : Type 0) (X : Type 0) := rose (roseLabel : A) (roseKids : List X)\n"
    "cofamily Rose (A : Type 0) := RoseF A\n", "", "CODATA_POSITIVE");
  failed += !domain_accepts("cofamily: a Pi chain field, the Mealy shape",
    "family PairInF (I0 : Type 0) (X : Type 0) := pairIn (pairInNext : I0 -> I0 -> X)\n"
    "cofamily PairIn (I0 : Type 0) := PairInF I0\n",
    "def sum : PairIn Nat := unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) 0\n"
    "def counter : Mealy Nat Nat := unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) 0\n");
  failed += !accepts("out gives one layer F ps (C ps)",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def layer : StreamF Nat (Stream Nat) := out nats\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  failed += !roundtrip("the unfold of a cofamily prints and checks again",
    "def r : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n");
  failed += !domain_refuses("cofamily: a layer above Type 0",
    "family LargeF (X : Type 0) := largeF (largeType : Type 0) (largeNext : X)\n"
    "cofamily Large := LargeF\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: a parameter shadows the layer name",
    "family ShadowF (A : Type 0) (X : Type 0) := shadowF (shadowNext : X)\n"
    "cofamily Shadow (ShadowF : Type 0) := ShadowF ShadowF\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: shadowed parameters are not in order",
    "family DupF (A : Type 0) (B : Type 0) (X : Type 0) := dupF (dupNext : X)\n"
    "cofamily Dup (A : Type 0) (A : Type 0) := DupF A A\n", "", "CODATA_FORM");
  failed += !roundtrip("out of a canonical unfold round trips",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def layer : StreamF Nat (Stream Nat) := out nats\n");
  failed += !roundtrip("coiteration with an Option seed round trips",
    "def seed : Option Nat := none\n"
    "def nats : Stream Nat := unfold (fun (s : Option Nat) => scons Nat (Option Nat) 0 s) seed\n");
  failed += !roundtrip("coiteration with a function seed round trips",
    "def seed : Nat -> Nat := fun (n : Nat) => n\n"
    "def nats : Stream Nat := unfold (fun (s : Nat -> Nat) => scons Nat (Nat -> Nat) (s 0) s) seed\n");
  failed += !domain_accepts("cofamily: dependent parameters keep their order",
    "family DepF (A : Type 0) (a : A) (X : Type 0) := depF (depNext : X)\n"
    "cofamily Dep (A : Type 0) (a : A) := DepF A a\n",
    "def value : Dep Nat 0 := unfold (fun (n : Nat) => depF Nat 0 Nat (succ n)) 0\n"
    "def layer : DepF Nat 0 (Dep Nat 0) := out value\n");
  failed += !roundtrip("coiteration types under binders use fresh names",
    "def make : (A : Type 0) -> Option A -> Stream Nat := "
    "fun (A : Type 0) (_case0 : Option A) => unfold "
    "(fun (s : Option A) => scons Nat (Option A) 0 s) _case0\n");
  failed += !roundtrip("coiteration in path endpoints retains its types",
    "def seed : Option Nat := none\n"
    "def nats : Stream Nat := unfold (fun (s : Option Nat) => scons Nat (Option Nat) 0 s) seed\n"
    "def same : Path (Stream Nat) nats nats := refl\n");
  failed += !roundtrip("coiteration with a cofamily seed round trips",
    "def seed : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def nats : Stream Nat := unfold (fun (s : Stream Nat) => scons Nat (Stream Nat) 0 s) seed\n");
  /* S4 chunk B: the out step. */
  failed += !accepts("out: one layer",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def one : Path (StreamF Nat (Stream Nat)) (out nats) "
    "(scons Nat (Stream Nat) 0 (unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 1)) := refl\n");
  failed += !accepts("out: two layers",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def two : Path (StreamF Nat (Stream Nat)) "
    "(out (elim (fun (l : StreamF Nat (Stream Nat)) => Stream Nat) (fun (h : Nat) (t : Stream Nat) => t) (out nats))) "
    "(scons Nat (Stream Nat) 1 (unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 2)) := refl\n");
  failed += !domain_accepts("out: a Pi chain field maps under its binders",
    "family PairInF (I0 : Type 0) (X : Type 0) := pairIn (pairInNext : I0 -> I0 -> X)\n"
    "cofamily PairIn (I0 : Type 0) := PairInF I0\n",
    "def counter : Mealy Nat Nat := unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) 0\n"
    "def counterOut : Path (MealyF Nat Nat (Mealy Nat Nat)) (out counter) (mealy Nat Nat (Mealy Nat Nat) 0 "
    "(fun (i : Nat) => unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) (natAdd 0 i))) := refl\n"
    "def sum : PairIn Nat := unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) 0\n"
    "def sumOut : Path (PairInF Nat (PairIn Nat)) (out sum) (pairIn Nat (PairIn Nat) "
    "(fun (a : Nat) (b : Nat) => unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) "
    "(natAdd 0 (natAdd a b)))) := refl\n");
  failed += !roundtrip("out of a variable is stuck and round trips",
    "def same : (x : Stream Nat) -> Path (StreamF Nat (Stream Nat)) (out x) (out x) := fun (x : Stream Nat) => refl\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  failed += !roundtrip("transp at a cofamily: a constant line gives the base, a line that changes is neutral",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def constant : Path (Stream Nat) (transp (fun (i : I) => Stream Nat) i0 nats) nats := refl\n"
    "def moved : (A : Type 0) -> (B : Type 0) -> Path (Type 0) A B -> Stream A -> Stream B := "
    "fun (A : Type 0) (B : Type 0) (p : Path (Type 0) A B) (s : Stream A) => transp (fun (i : I) => Stream (p @ i)) i0 s\n");
  failed += !out_fuel();
  failed += !bisim_fuel();
  /* S4 chunk C: refl after eval. */
  failed += !accepts("unfold: refl of two unfolds after a take",
    "def zerosA : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat 0 n) 0\n"
    "def zerosB : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat 0 (succ n)) 0\n"
    "def take : Nat -> Stream Nat -> List Nat := fun (n : Nat) => fold "
    "(fun (rest : Stream Nat -> List Nat) (s : Stream Nat) => fold (fun (h : Nat) (t : Stream Nat) => cons h (rest t)) (out s)) "
    "(fun (s : Stream Nat) => nil) n\n"
    "def same : Path (List Nat) (take 3 zerosA) (take 3 zerosB) := refl\n");
  failed += !domain_refuses("unfold: refl of g at 0 and g at 1 after eval", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat n (succ n)\n"
    "def bad : Path (Stream Nat) (unfold g (natSub 1 1)) (unfold g (natAdd 0 1)) := refl\n", "PATH_END");
  failed += !domain_accepts("cofamily: both constructors retain out computation and reflexivity",
    "family TwoF (X : Type 0) := zeroF | oneF (n : Nat)\ncofamily Two := TwoF\n",
    "def a : Two := unfold (fun (s : Nat) => zeroF Nat) 0\n"
    "def b : Two := unfold (fun (s : Nat) => oneF Nat 1) 0\n"
    "def sameA : Path Two a a := refl\n"
    "def sameB : Path Two b b := refl\n"
    "def outA : Path (TwoF Two) (out a) (zeroF Two) := refl\n"
    "def outB : Path (TwoF Two) (out b) (oneF Two 1) := refl\n");
  failed += !domain_refuses("cofamily: conversion preserves distinct layer constructor tags",
    "family TwoF (X : Type 0) := zeroF | oneF (n : Nat)\ncofamily Two := TwoF\n",
    "def a : Two := unfold (fun (s : Nat) => zeroF Nat) 0\n"
    "def b : Two := unfold (fun (s : Nat) => oneF Nat 1) 0\n"
    "def bad : Path Two a b := refl\n", "PATH_END");
  failed += !domain_refuses("cofamily: a sum layer does not admit record eta",
    "family TwoF (X : Type 0) := zeroF | oneF (n : Nat)\ncofamily Two := TwoF\n",
    "def bad : (w : Two) -> Path (TwoF Two) (zeroF Two) (out w) := fun w => refl\n", "PATH_END");
  failed += !domain_refuses("cofamily: a constant constructor step is not eta for a sum",
    "family TwoF (X : Type 0) := zeroF | oneF (n : Nat)\ncofamily Two := TwoF\n",
    "def bad : (w : Two) -> Path Two (unfold (fun (s : Two) => zeroF Two) w) w := fun w => refl\n", "PATH_END");
  failed += !domain_accepts("cofamily: finite observations can agree at different seed types",
    "family OneF (X : Type 0) := oneL (n : Nat)\ncofamily One := OneF\n",
    "def a : One := unfold (fun (s : Nat) => oneL Nat 7) 0\n"
    "def b : One := unfold (fun (s : Unit) => oneL Unit 7) unit\n"
    "def same : Path One a b := refl\n"
    "def observed : Path (OneF One) (out b) (oneL One 7) := refl\n");
  /* S4 chunk B: the cocells (K4, K6, the K8 killers, K5 (a) at a
     variable). A refused layer is extra domain text. */
  failed += !domain_refuses("cofamily: a Pi chain field with X under List (c1)",
    "family KidsF (A : Type 0) (X : Type 0) := kidsF (kidsLabel : A) (kids : Nat -> List X)\n"
    "cofamily Kids (A : Type 0) := KidsF A\n", "", "CODATA_POSITIVE");
  failed += !domain_refuses("cofamily: a parameter in Type 1 against a layer parameter in Type 0 (c6)",
    "family BadF (A : Type 0) (X : Type 0) := badF (badNext : X)\n"
    "cofamily Bad (A : Type 1) := BadF A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: a layer in a block",
    "family BlkF (A : Type 0) (X : Type 0) := blkF (blkNext : X) and BlkG (A : Type 0) (X : Type 0) := blkG\n"
    "cofamily Blk (A : Type 0) := BlkF A\n", "", "CODATA_FORM");
  failed += !roundtrip("cocell: the unfold of StreamK prints and checks again",
    "def natsK : StreamK Nat := unfold (fun (n : Nat) => nodeStreamK Nat Nat n (succ n)) 0\n");
  failed += !roundtrip("cocell: the unfold of EnvK prints and checks again",
    "def env : EnvK Nat Nat := unfold (fun (u : Prod Nat Nat) => nodeEnvK Nat Nat (Prod Nat Nat) (first u) (second u)) (pair 4 7)\n");
  failed += !roundtrip("cocell: the unfold of CofreeK prints and checks again",
    "def tree : CofreeK Nat (fun (a : Nat) => Nat) Nat := "
    "unfold (fun (n : Nat) => nodeCofreeK Nat (fun (a : Nat) => Nat) Nat Nat n (succ n) (fun (k : Nat) => natAdd n k)) 1\n");
  failed += !accepts("cocell: Cofree L1 at a variable",
    "def cofreeL1 : (A : Type 0) -> (B : A -> Type 0) -> (X : Type 0) -> (C : Type 0) -> (f : CofreeK A B X -> C) -> "
    "(w : CofreeK A B X) -> Path C (extract (extend f w)) (f w) := fun A B X C f w => refl\n");
  failed += !accepts("cocell: a cocell after a cell has its own family slots",
    "cell Twice (X : Type 0) := twice : X -> X\n"
    "cocell PeekK (X : Type 0) := peekK : X -> Nat\n"
    "def p : PeekK Nat := unfold (fun (n : Nat) => nodePeekK Nat Nat n (natAdd n n)) 3\n"
    "def e : Path Nat (extract p) 3 := refl\n"
    "def v : Path Nat (peekK (out p)) 6 := refl\n");
  /* Each def of domain/domain.lang gives a spare family slot (46 at S4), so
     the slot count for cocells needs more cocells than that (R5). */
  {
    static char many[4096];
    size_t used = 0;
    unsigned k;
    for (k = 0; k < 64u; k++)
      used += (size_t)snprintf(many + used, sizeof many - used, "cocell K%u (X : Type 0) := k%u : X -> Nat\n", k, k);
    failed += !accepts("cocell: 64 cocells have their family slots (R5)", many);
  }
  failed += !accepts("cocell: PropEnvK, the coreflection P * - at a proposition (K6)",
    "cocell PropEnvK (P : Type 0) (pp : isProp P) (X : Type 0) := askP : X -> P\n"
    "def penv : PropEnvK (Not Empty) (isPropNot Empty) Nat := "
    "unfold (fun (n : Nat) => nodePropEnvK (Not Empty) (isPropNot Empty) Nat Nat n (fun (e : Empty) => e)) 5\n"
    "def next : Path Nat (extract (extend (fun (w : PropEnvK (Not Empty) (isPropNot Empty) Nat) => succ (extract w)) penv)) 6 := refl\n"
    "def propL2 : (P : Type 0) -> (pp : isProp P) -> (X : Type 0) -> (w : PropEnvK P pp X) -> "
    "Path (PropEnvK P pp X) (extract (duplicate w)) w := fun P pp X w => refl\n");
  failed += !accepts("bisimulation rule: eta at a variable",
    "def eta : (w : Stream Nat) -> Path (Stream Nat) (unfold (fun (s : Stream Nat) => out s) w) w := fun w => refl\n");
  failed += !accepts("bisimulation rule: L3 at a variable",
    "def streamL3 : (X : Type 0) -> (w : StreamK X) -> Path (StreamK X) (extend extract w) w := fun X w => refl\n");
  failed += !domain_refuses("bisimulation rule: coassociativity needs more than seed-preserving conversion", "",
    "def streamCo : (X : Type 0) -> (w : StreamK X) -> Path (StreamK (StreamK (StreamK X))) "
    "(extend duplicate w) (duplicate (duplicate w)) := fun X w => refl\n", "PATH_END");
  failed += !domain_refuses("bisimulation rule: unequal variable seeds do not infer a graph", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat 0 n\n"
    "def ab : (n : Nat) -> Path (Stream Nat) (unfold g (succ n)) (unfold g n) := fun n => refl\n", "PATH_END");
  failed += !domain_refuses("bisimulation rule: the mirror graph is also refused", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat 0 n\n"
    "def bc : (n : Nat) -> Path (Stream Nat) (unfold g n) (unfold g (natAdd n 2)) := fun n => refl\n", "PATH_END");
  failed += !domain_refuses("bisimulation rule: the nonvariable graph pair stays refused", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat 0 n\n"
    "def ac : (n : Nat) -> Path (Stream Nat) (unfold g (succ n)) (unfold g (natAdd n 2)) := fun n => refl\n", "PATH_END");
  failed += !domain_refuses("bisimulation rule: a closed graph pair is refused consistently", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat 0 n\n"
    "def closed : Path (Stream Nat) (unfold g 1) (unfold g 0) := refl\n", "PATH_END");
  failed += !roundtrip("bisimulation rule: seed-preserving eta specializes to a closed seed",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat n (succ n)\n"
    "def eta : (w : Stream Nat) -> Path (Stream Nat) (unfold (fun (s : Stream Nat) => out s) w) w := fun w => refl\n"
    "def closed : Path (Stream Nat) (unfold (fun (s : Stream Nat) => out s) (unfold g 0)) (unfold g 0) := eta (unfold g 0)\n"
    "def direct : Path (Stream Nat) (unfold (fun (s : Stream Nat) => out s) (unfold g 0)) (unfold g 0) := refl\n");
  failed += !domain_refuses("cocell: a program def with the node name nodeStreamK (ruling 2026-10-23)", "",
    "def nodeStreamK : Nat := 0\n", "REFUSE_NAME");
  /* S4 open point 3: an hcomp at a cofamily is stuck (hcomp_stuck, a
     neutral; only a HIT gets the formal hcomp), so the rule applies by eta,
     as for a transp neutral. K11 predicted the rule off. */
  failed += !accepts("bisimulation rule: eta for an hcomp at a cofamily",
    "def hp : (w : Stream Nat) -> Path (Path (Stream Nat) w w) (fun (j : I) => unfold (fun (s : Stream Nat) => out s) (hcomp (Stream Nat) (~j \\/ j) (fun (i : I) => [ ~j -> w, j -> w ]) w)) (fun (j : I) => hcomp (Stream Nat) (~j \\/ j) (fun (i : I) => [ ~j -> w, j -> w ]) w) := fun w => refl\n");
  /* S5 open point 9 (U6): eta contracts an unfold only when its seed type
     is the cofamily type. No field type of PhK uses P, so the step at the
     seed type PhK (List Nat) Nat passes the field check. */
  failed += !domain_refuses("bisimulation rule: no eta at a seed type with a different parameter", "",
    "cocell PhK (P : Type 0) (X : Type 0) := phK : X -> X\n"
    "def same : Path (PhK Nat Nat) (unfold (fun (s : PhK (List Nat) Nat) => nodePhK Nat Nat (PhK (List Nat) Nat) (extract s) (phK (out s))) "
    "(unfold (fun (n : Nat) => nodePhK (List Nat) Nat Nat n (succ n)) 0)) (unfold (fun (n : Nat) => nodePhK Nat Nat Nat n (succ n)) 0) := refl\n",
    "PATH_END");
  failed += !accepts("bisimulation rule: eta at the seed type of the cofamily",
    "cocell PhK (P : Type 0) (X : Type 0) := phK : X -> X\n"
    "def base0 : PhK Nat Nat := unfold (fun (n : Nat) => nodePhK Nat Nat Nat n (succ n)) 0\n"
    "def moved : PhK Nat Nat := unfold (fun (s : PhK Nat Nat) => nodePhK Nat Nat (PhK Nat Nat) (extract s) (phK (out s))) base0\n"
    "def same : Path (PhK Nat Nat) base0 moved := refl\n");
  /* S5 K1: Text literals and textEq, the Credits bound, the reference
     refusal. */
  failed += !accepts("text: textEq gives some at equal bytes",
    "def e : Path (Option Unit) (textEq \"ab\" \"ab\") (some unit) := refl\n");
  failed += !accepts("text: textEq gives none at unequal bytes",
    "def e : Path (Option Unit) (textEq \"ab\" \"ac\") none := refl\n");
  failed += !accepts("text: textEq at a variable is stuck",
    "def v : (a : Text) -> Path (Option Unit) (textEq a \"x\") (textEq a \"x\") := fun a => refl\n");
  failed += !json_contains("text: empty homogeneous composition computes a literal",
    "def t : Text := hcomp Text i0 (fun (i : I) => []) \"Ada\"\n", "\"value\":\"Ada\"");
  failed += !accepts("text: homogeneous composition with a constant live face computes",
    "def p : Path (Path Text \"Ada\" \"Ada\") refl refl := fun (j : I) => fun (k : I) =>\n"
    "  hcomp Text j (fun (i : I) => [ j -> \"Ada\" ]) \"Ada\"\n");
  failed += !domain_refuses("text: composition does not identify different literals", "",
    "def p : Path Text (hcomp Text i0 (fun (i : I) => []) \"Ada\") \"Bo\" := refl\n", "PATH_END");
  {
    const char *const invalid[] = {
      "\\xff", "\\x80", "\\xc0\\x80", "\\xe2\\x82", "\\xed\\xa0\\x80", "\\xf4\\x90\\x80\\x80", "\xff"
    };
    size_t i;
    for (i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
      char source[128];
      snprintf(source, sizeof source, "def t : Text := \"%s\"\n", invalid[i]);
      failed += !domain_refuses("text: malformed UTF-8 cannot reach JSON", "", source, "LEX_STR_UTF8");
    }
  }
  failed += !json_contains("text: valid UTF-8 and escaped NUL survive JSON",
    "def t : Text := \"\\x00\\xc2\\x80\\xe0\\xa0\\x80\\xed\\x9f\\xbf\\xf0\\x90\\x80\\x80\\xf4\\x8f\\xbf\\xbf\"\n",
    "\"value\":\"\\u0000\xc2\x80\xe0\xa0\x80\xed\x9f\xbf\xf0\x90\x80\x80\xf4\x8f\xbf\xbf\"");
  failed += !accepts("text: each lexer escape gives its byte",
    "def e : Path (Option Unit) (textEq \"\\x41\\x0a\\x09\\x22\\x5c\" \"A\\n\\t\\\"\\\\\") (some unit) := refl\n");
  failed += !domain_refuses("text: an unknown escape is a lex refusal", "",
    "def t : Text := \"\\q\"\n", "LEX_STR_ESCAPE");
  failed += !domain_refuses("text: a raw control byte is a lex refusal", "",
    "def t : Text := \"a\x01\"\n", "LEX_STR_CONTROL");
  failed += !json_contains("text: the JSON escapes of a Text value",
    "def t : Text := \"a\\\"b\\\\c\\x01\\x7f\"\n", "\"value\":\"a\\u0022b\\u005cc\\u0001\x7f\"");
  failed += !accepts("credits: creditsCheck keeps 2^64 - 1",
    "def c : Path Nat (creditsCheck 18446744073709551615) 18446744073709551615 := refl\n");
  failed += !domain_refuses("credits: creditsCheck refuses 2^64", "",
    "def c : Path Nat (creditsCheck 18446744073709551616) 0 := refl\n", "EVAL_CREDITS_OVERFLOW");
  failed += !accepts("credits: creditsCheck at a variable is stuck",
    "def v : (n : Nat) -> Path Nat (creditsCheck n) (creditsCheck n) := fun n => refl\n");
  failed += !domain_refuses("reference: refUnknown refuses a literal id", "",
    "def r : Path Nat (refUnknown Nat 7) 0 := refl\n", "EVAL_REF_UNKNOWN");
  failed += !accepts("reference: refUnknown at a variable id is stuck",
    "def r : (n : Nat) -> Path Nat (refUnknown Nat n) (refUnknown Nat n) := fun n => refl\n");
  failed += !domain_refuses("reference: refUnknown takes A : Type 0 only", "",
    "def z : Type 0 := refUnknown (Type 0) 3\n", "TYPE_MISMATCH");
  failed += !accepts("law L5 rrName: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : Avatar) -> (f : Prod (Text) (Text) -> Virt X) -> Path (Virt X) (avatarName X a (fun (x : Text) => avatarName X a (fun (y : Text) => f (pair x y)))) (avatarName X a (fun (x : Text) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Text) (Text) -> Virt X) => fun (i : I) => rrName X a f i\n"
    "def lzRun : (X : Type 0) -> (a : Avatar) -> (f : Prod (Text) (Text) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (avatarName X a (fun (x : Text) => avatarName X a (fun (y : Text) => f (pair x y)))) w) (run X (avatarName X a (fun (x : Text) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Text) (Text) -> Virt X) (w : W) => fun (i : I) => run X (rrName X a f i) w\n");
  failed += !accepts("law L5 rrRole: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : Avatar) -> (f : Prod (Role) (Role) -> Virt X) -> Path (Virt X) (avatarRole X a (fun (x : Role) => avatarRole X a (fun (y : Role) => f (pair x y)))) (avatarRole X a (fun (x : Role) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Role) (Role) -> Virt X) => fun (i : I) => rrRole X a f i\n"
    "def lzRun : (X : Type 0) -> (a : Avatar) -> (f : Prod (Role) (Role) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (avatarRole X a (fun (x : Role) => avatarRole X a (fun (y : Role) => f (pair x y)))) w) (run X (avatarRole X a (fun (x : Role) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Role) (Role) -> Virt X) (w : W) => fun (i : I) => run X (rrRole X a f i) w\n");
  failed += !accepts("law L5 rrCard: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : Avatar) -> (f : Prod (Option VirtuCard) (Option VirtuCard) -> Virt X) -> Path (Virt X) (avatarCard X a (fun (x : Option VirtuCard) => avatarCard X a (fun (y : Option VirtuCard) => f (pair x y)))) (avatarCard X a (fun (x : Option VirtuCard) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Option VirtuCard) (Option VirtuCard) -> Virt X) => fun (i : I) => rrCard X a f i\n"
    "def lzRun : (X : Type 0) -> (a : Avatar) -> (f : Prod (Option VirtuCard) (Option VirtuCard) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (avatarCard X a (fun (x : Option VirtuCard) => avatarCard X a (fun (y : Option VirtuCard) => f (pair x y)))) w) (run X (avatarCard X a (fun (x : Option VirtuCard) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (Option VirtuCard) (Option VirtuCard) -> Virt X) (w : W) => fun (i : I) => run X (rrCard X a f i) w\n");
  failed += !accepts("law L5 rrGet: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : Avatar) -> (k : Text) -> (f : Prod (Option Value) (Option Value) -> Virt X) -> Path (Virt X) (avatarGet X a k (fun (x : Option Value) => avatarGet X a k (fun (y : Option Value) => f (pair x y)))) (avatarGet X a k (fun (x : Option Value) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : Avatar) (k : Text) (f : Prod (Option Value) (Option Value) -> Virt X) => fun (i : I) => rrGet X a k f i\n"
    "def lzRun : (X : Type 0) -> (a : Avatar) -> (k : Text) -> (f : Prod (Option Value) (Option Value) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (avatarGet X a k (fun (x : Option Value) => avatarGet X a k (fun (y : Option Value) => f (pair x y)))) w) (run X (avatarGet X a k (fun (x : Option Value) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : Avatar) (k : Text) (f : Prod (Option Value) (Option Value) -> Virt X) (w : W) => fun (i : I) => run X (rrGet X a k f i) w\n");
  failed += !accepts("law L5 rrInv: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : Avatar) -> (f : Prod (List VirtualItem) (List VirtualItem) -> Virt X) -> Path (Virt X) (avatarInventory X a (fun (x : List VirtualItem) => avatarInventory X a (fun (y : List VirtualItem) => f (pair x y)))) (avatarInventory X a (fun (x : List VirtualItem) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (List VirtualItem) (List VirtualItem) -> Virt X) => fun (i : I) => rrInv X a f i\n"
    "def lzRun : (X : Type 0) -> (a : Avatar) -> (f : Prod (List VirtualItem) (List VirtualItem) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (avatarInventory X a (fun (x : List VirtualItem) => avatarInventory X a (fun (y : List VirtualItem) => f (pair x y)))) w) (run X (avatarInventory X a (fun (x : List VirtualItem) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : Avatar) (f : Prod (List VirtualItem) (List VirtualItem) -> Virt X) (w : W) => fun (i : I) => run X (rrInv X a f i) w\n");
  failed += !accepts("law L5 rrBal: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (Credits) (Credits) -> Virt X) -> Path (Virt X) (cardBalance X a (fun (x : Credits) => cardBalance X a (fun (y : Credits) => f (pair x y)))) (cardBalance X a (fun (x : Credits) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (Credits) (Credits) -> Virt X) => fun (i : I) => rrBal X a f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (Credits) (Credits) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (cardBalance X a (fun (x : Credits) => cardBalance X a (fun (y : Credits) => f (pair x y)))) w) (run X (cardBalance X a (fun (x : Credits) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (Credits) (Credits) -> Virt X) (w : W) => fun (i : I) => run X (rrBal X a f i) w\n");
  failed += !accepts("law L5 rrSec: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (SecurityLevel) (SecurityLevel) -> Virt X) -> Path (Virt X) (cardSecurity X a (fun (x : SecurityLevel) => cardSecurity X a (fun (y : SecurityLevel) => f (pair x y)))) (cardSecurity X a (fun (x : SecurityLevel) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (SecurityLevel) (SecurityLevel) -> Virt X) => fun (i : I) => rrSec X a f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (SecurityLevel) (SecurityLevel) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (cardSecurity X a (fun (x : SecurityLevel) => cardSecurity X a (fun (y : SecurityLevel) => f (pair x y)))) w) (run X (cardSecurity X a (fun (x : SecurityLevel) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (SecurityLevel) (SecurityLevel) -> Virt X) (w : W) => fun (i : I) => run X (rrSec X a f i) w\n");
  failed += !accepts("law L5 rrHist: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (List Entry) (List Entry) -> Virt X) -> Path (Virt X) (cardHistory X a (fun (x : List Entry) => cardHistory X a (fun (y : List Entry) => f (pair x y)))) (cardHistory X a (fun (x : List Entry) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (List Entry) (List Entry) -> Virt X) => fun (i : I) => rrHist X a f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtuCard) -> (f : Prod (List Entry) (List Entry) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (cardHistory X a (fun (x : List Entry) => cardHistory X a (fun (y : List Entry) => f (pair x y)))) w) (run X (cardHistory X a (fun (x : List Entry) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtuCard) (f : Prod (List Entry) (List Entry) -> Virt X) (w : W) => fun (i : I) => run X (rrHist X a f i) w\n");
  failed += !accepts("law L5 rrTInv: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtualTerminal) -> (f : Prod (List Listing) (List Listing) -> Virt X) -> Path (Virt X) (terminalInventory X a (fun (x : List Listing) => terminalInventory X a (fun (y : List Listing) => f (pair x y)))) (terminalInventory X a (fun (x : List Listing) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtualTerminal) (f : Prod (List Listing) (List Listing) -> Virt X) => fun (i : I) => rrTInv X a f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtualTerminal) -> (f : Prod (List Listing) (List Listing) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (terminalInventory X a (fun (x : List Listing) => terminalInventory X a (fun (y : List Listing) => f (pair x y)))) w) (run X (terminalInventory X a (fun (x : List Listing) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtualTerminal) (f : Prod (List Listing) (List Listing) -> Virt X) (w : W) => fun (i : I) => run X (rrTInv X a f i) w\n");
  failed += !accepts("law L5 rrVal: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtualItem) -> (f : Prod (Credits) (Credits) -> Virt X) -> Path (Virt X) (itemValue X a (fun (x : Credits) => itemValue X a (fun (y : Credits) => f (pair x y)))) (itemValue X a (fun (x : Credits) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtualItem) (f : Prod (Credits) (Credits) -> Virt X) => fun (i : I) => rrVal X a f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtualItem) -> (f : Prod (Credits) (Credits) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (itemValue X a (fun (x : Credits) => itemValue X a (fun (y : Credits) => f (pair x y)))) w) (run X (itemValue X a (fun (x : Credits) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtualItem) (f : Prod (Credits) (Credits) -> Virt X) (w : W) => fun (i : I) => run X (rrVal X a f i) w\n");
  failed += !accepts("law L5 rrIGet: the law in Virt and its runner path at a variable world (S6)",
    "def lzLaw : (X : Type 0) -> (a : VirtualItem) -> (k : Text) -> (f : Prod (Option Value) (Option Value) -> Virt X) -> Path (Virt X) (itemGet X a k (fun (x : Option Value) => itemGet X a k (fun (y : Option Value) => f (pair x y)))) (itemGet X a k (fun (x : Option Value) => f (pair x x))) :=\n"
    "  fun (X : Type 0) (a : VirtualItem) (k : Text) (f : Prod (Option Value) (Option Value) -> Virt X) => fun (i : I) => rrIGet X a k f i\n"
    "def lzRun : (X : Type 0) -> (a : VirtualItem) -> (k : Text) -> (f : Prod (Option Value) (Option Value) -> Virt X) -> (w : W) -> Path (Prod W X) (run X (itemGet X a k (fun (x : Option Value) => itemGet X a k (fun (y : Option Value) => f (pair x y)))) w) (run X (itemGet X a k (fun (x : Option Value) => f (pair x x))) w) :=\n"
    "  fun (X : Type 0) (a : VirtualItem) (k : Text) (f : Prod (Option Value) (Option Value) -> Virt X) (w : W) => fun (i : I) => run X (rrIGet X a k f i) w\n");
  failed += !accepts("law L1 at a closed world, setSecurity (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL1c : (l : SecurityLevel) -> (m : SecurityLevel) -> Path (Prod W Unit) (run Unit (setSecurity Unit (cardRef Empty 1) l (setSecurity Unit (cardRef Empty 1) m (pure unit))) pzW) (run Unit (setSecurity Unit (cardRef Empty 1) m (pure unit)) pzW) := fun (l : SecurityLevel) (m : SecurityLevel) => refl\n");
  failed += !accepts("law L2 at a closed world, setRarity (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL2c : (r : Rarity) -> (s : Rarity) -> Path (Prod W Unit) (run Unit (setRarity Unit (itemRef Empty 2) r (setRarity Unit (itemRef Empty 2) s (pure unit))) pzW) (run Unit (setRarity Unit (itemRef Empty 2) s (pure unit)) pzW) := fun (r : Rarity) (s : Rarity) => refl\n");
  failed += !accepts("law L3 at a closed world, avatarSet (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL3cLit : (v : Value) -> (u : Value) -> Path (Prod W Unit) (run Unit (avatarSet Unit (avatarRef Empty 0) \"g\" v (avatarSet Unit (avatarRef Empty 0) \"g\" u (pure unit))) pzW) (run Unit (avatarSet Unit (avatarRef Empty 0) \"g\" u (pure unit)) pzW) := fun (v : Value) (u : Value) => refl\n");
  failed += !accepts("law L3 at a closed world, itemSet (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL3cItem : (v : Value) -> (u : Value) -> Path (Prod W Unit) (run Unit (itemSet Unit (itemRef Empty 2) \"g\" v (itemSet Unit (itemRef Empty 2) \"g\" u (pure unit))) pzW) (run Unit (itemSet Unit (itemRef Empty 2) \"g\" u (pure unit)) pzW) := fun (v : Value) (u : Value) => refl\n");
  failed += !accepts("law L4 at a closed world, avatarSet (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL4cLit : (v : Value) -> (f : Option Value -> Virt Unit) -> Path (Prod W Unit) (run Unit (avatarSet Unit (avatarRef Empty 0) \"g\" v (avatarGet Unit (avatarRef Empty 0) \"g\" f)) pzW) (run Unit (avatarSet Unit (avatarRef Empty 0) \"g\" v (f (some v))) pzW) := fun (v : Value) (f : Option Value -> Virt Unit) => refl\n");
  failed += !accepts("law L4 at a closed world, itemSet (S6)",
    "def pzW : W := first (run Unit (avatar Unit \"Ada\" explorer (fun (a : Avatar) => createVirtucard Unit a 5 (fun (c : VirtuCard) => item Unit \"Lamp\" \"tools\" 3 (fun (z : VirtualItem) => pure unit)))) nil)\n"
    "def pzL4cItem : (v : Value) -> (f : Option Value -> Virt Unit) -> Path (Prod W Unit) (run Unit (itemSet Unit (itemRef Empty 2) \"g\" v (itemGet Unit (itemRef Empty 2) \"g\" f)) pzW) (run Unit (itemSet Unit (itemRef Empty 2) \"g\" v (f (some v))) pzW) := fun (v : Value) (f : Option Value -> Virt Unit) => refl\n");
  failed += !accepts("law 2-dim: the fold path case at an inner point, variable t (S6 W5)",
    "cell SecC (X : Type 0) := setS : (setSA : Nat) -> (setST : X) -> X\n"
    "  | ssC : (ssCA : Nat) -> (ssCB : Nat) -> (ssCT : X) -> (i : I) -> X [ ~i -> setS X ssCA (setS X ssCB ssCT), i -> setS X ssCB ssCT ]\n"
    "def runS : SecC Nat -> Nat -> Prod Nat Nat := fun (t : SecC Nat) => fold\n"
    "  (fun (a : Nat) (r : Nat -> Prod Nat Nat) => fun (s : Nat) => r a)\n"
    "  (fun (a : Nat) (b : Nat) (r : Nat -> Prod Nat Nat) => refl)\n"
    "  (fun (v : Nat) => fun (s : Nat) => pair s v)\n"
    "  t\n"
    "def lawS : (a : Nat) -> (b : Nat) -> (t : SecC Nat) -> Path (SecC Nat) (setS Nat a (setS Nat b t)) (setS Nat b t) :=\n"
    "  fun (a : Nat) (b : Nat) (t : SecC Nat) => fun (i : I) => ssC Nat a b t i\n"
    "def lawRunS : (a : Nat) -> (b : Nat) -> (t : SecC Nat) -> (s : Nat) -> Path (Prod Nat Nat) (runS (setS Nat a (setS Nat b t)) s) (runS (setS Nat b t) s) :=\n"
    "  fun (a : Nat) (b : Nat) (t : SecC Nat) (s : Nat) => fun (i : I) => runS (ssC Nat a b t i) s\n"
    "def mzK5 : (t : SecC Nat) -> Path (Path (Prod Nat Nat) (runS (setS Nat 1 (setS Nat 2 t)) 0) (runS (setS Nat 2 t) 0)) (fun (i : I) => runS (ssC Nat 1 2 t i) 0) (fun (i : I) => runS (setS Nat 2 t) 0) :=\n"
    "  fun (t : SecC Nat) => refl\n");
  failed += !accepts("law 2-dim: the fold path case at an inner point, closed t (S6 W5)",
    "cell SecC (X : Type 0) := setS : (setSA : Nat) -> (setST : X) -> X\n"
    "  | ssC : (ssCA : Nat) -> (ssCB : Nat) -> (ssCT : X) -> (i : I) -> X [ ~i -> setS X ssCA (setS X ssCB ssCT), i -> setS X ssCB ssCT ]\n"
    "def runS : SecC Nat -> Nat -> Prod Nat Nat := fun (t : SecC Nat) => fold\n"
    "  (fun (a : Nat) (r : Nat -> Prod Nat Nat) => fun (s : Nat) => r a)\n"
    "  (fun (a : Nat) (b : Nat) (r : Nat -> Prod Nat Nat) => refl)\n"
    "  (fun (v : Nat) => fun (s : Nat) => pair s v)\n"
    "  t\n"
    "def lawS : (a : Nat) -> (b : Nat) -> (t : SecC Nat) -> Path (SecC Nat) (setS Nat a (setS Nat b t)) (setS Nat b t) :=\n"
    "  fun (a : Nat) (b : Nat) (t : SecC Nat) => fun (i : I) => ssC Nat a b t i\n"
    "def lawRunS : (a : Nat) -> (b : Nat) -> (t : SecC Nat) -> (s : Nat) -> Path (Prod Nat Nat) (runS (setS Nat a (setS Nat b t)) s) (runS (setS Nat b t) s) :=\n"
    "  fun (a : Nat) (b : Nat) (t : SecC Nat) (s : Nat) => fun (i : I) => runS (ssC Nat a b t i) s\n"
    "def mzK5d : Path (Path (Prod Nat Nat) (pair 2 5) (pair 2 5)) (fun (i : I) => runS (ssC Nat 1 2 (pure 5) i) 0) (fun (i : I) => pair 2 5) := refl\n");
  failed += !accepts("law L5 2-dim: run of rrBal at an inner point (S6)",
    "def lz2 : (c : VirtuCard) -> (f : Prod Credits Credits -> Virt Unit) -> (w : W) -> Path (Path (Prod W Unit) (run Unit (cardBalance Unit c (fun (x : Credits) => cardBalance Unit c (fun (y : Credits) => f (pair x y)))) w) (run Unit (cardBalance Unit c (fun (x : Credits) => f (pair x x))) w)) (fun (i : I) => run Unit (rrBal Unit c f i) w) (fun (i : I) => run Unit (cardBalance Unit c (fun (x : Credits) => f (pair x x))) w) :=\n"
    "  fun (c : VirtuCard) (f : Prod Credits Credits -> Virt Unit) (w : W) => refl\n");
  printf("path regressions: %d checked, %d failed\n", cases_run, failed);
  return failed != 0;
}
