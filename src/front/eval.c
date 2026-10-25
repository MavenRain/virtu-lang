/* The evaluator, the conversion test, quote and the printer (see
   front/core.h). SHARED by the tcc kits. */
#include "front/core.h"
#include "front/big.h"

#include <string.h>

#define PRINT_DEPTH_MAX 200u
#define PRINT_NAMES_MAX EVAL_DEPTH_LIMIT
#define VARIES 0xffu

static const char *const OP_NAMES[] = {
  [OP_NAT] = "Nat", [OP_UNIT] = "Unit", [OP_PROD] = "Prod",
  [OP_SUM] = "Sum", [OP_OPTION] = "Option", [OP_LIST] = "List",
  [OP_FAMILY] = "?", [OP_UNIT_VAL] = "unit",
  [OP_PAIR] = "pair", [OP_INL] = "inl", [OP_INR] = "inr",
  [OP_NONE] = "none", [OP_SOME] = "some", [OP_NIL] = "nil", [OP_CONS] = "cons",
  [OP_PACK] = "pack", [OP_CTOR] = "?", [OP_FIRST] = "first",
  [OP_SECOND] = "second", [OP_EITHER] = "either", [OP_OPTION_ELIM] = "option",
  [OP_PURE] = "pure", [OP_MAP] = "map", [OP_BIND] = "bind", [OP_FOLD_NAT] = "fold",
  [OP_FOLD_LIST] = "fold", [OP_FOLD_FAMILY] = "fold",
  [OP_COUNFOLD] = "unfold", [OP_OUT] = "out",
  [OP_TEXT] = "Text", [OP_TEXT_EQ] = "textEq", [OP_CREDITS_CHECK] = "creditsCheck", [OP_REF_UNKNOWN] = "refUnknown",
  [OP_FILTER] = "filter", [OP_WITNESS] = "witness", [OP_PAYLOAD] = "payload",
  [OP_NAT_ADD] = "natAdd", [OP_NAT_SUB] = "natSub",
  [OP_NAT_MUL] = "natMul", [OP_SUCC] = "succ", [OP_NAT_EQ] = "natEq",
  [OP_NAT_LE] = "natLe",
  [OP_PROJ] = "?",
  [OP_INTERVAL] = "I", [OP_TRANSP] = "transp",
  [OP_SYSTEM] = "system", [OP_HCOMP] = "hcomp", [OP_FIN] = "Fin",
  [OP_FIN_ADD] = "finAdd", [OP_FIN_NEG] = "finNeg", [OP_FIN_MUL] = "finMul",
  [OP_FIN_VAL] = "finVal", [OP_FIN_EQ] = "finEq", [OP_FIN_CASE] = "finCase",
  [OP_DEC_EQ_FIN] = "decEqFin", [OP_DEC_EQ_NAT] = "decEqNat",
  [OP_GLUE] = "Glue", [OP_GLUE_ELEM] = "glue", [OP_UNGLUE] = "unglue",
  [OP_EMPTY] = "Empty", [OP_ELIM] = "elim", [OP_ABSURD] = "absurd"
};

/* The number of arguments of each operation. VARIES: it comes from the
   family tables. */
static const unsigned char OP_ARITY[] = {
  [OP_NAT] = 0, [OP_UNIT] = 0, [OP_PROD] = 2, [OP_SUM] = 2,
  [OP_OPTION] = 1, [OP_LIST] = 1, [OP_FAMILY] = VARIES,
  [OP_UNIT_VAL] = 0, [OP_PAIR] = 2,
  [OP_INL] = 1, [OP_INR] = 1, [OP_NONE] = 0, [OP_SOME] = 1, [OP_NIL] = 0,
  [OP_CONS] = 2, [OP_PACK] = 2, [OP_CTOR] = VARIES, [OP_FIRST] = 1,
  [OP_SECOND] = 1, [OP_EITHER] = 3, [OP_OPTION_ELIM] = 3, [OP_PURE] = 1,
  [OP_MAP] = 2, [OP_BIND] = 2, [OP_FOLD_NAT] = 3, [OP_FOLD_LIST] = 3,
  [OP_FOLD_FAMILY] = VARIES, [OP_COUNFOLD] = 2, [OP_OUT] = 1, [OP_FILTER] = 2, [OP_WITNESS] = 1,
  [OP_TEXT] = 0, [OP_TEXT_EQ] = 2, [OP_CREDITS_CHECK] = 1, [OP_REF_UNKNOWN] = 2,
  [OP_PAYLOAD] = 1,
  [OP_NAT_ADD] = 2, [OP_NAT_SUB] = 2, [OP_NAT_MUL] = 2, [OP_SUCC] = 1,
  [OP_NAT_EQ] = 2, [OP_NAT_LE] = 2, [OP_PROJ] = 1,
  [OP_INTERVAL] = 0, [OP_TRANSP] = 3,
  [OP_SYSTEM] = VARIES, [OP_HCOMP] = 4, [OP_FIN] = 1, [OP_FIN_ADD] = 3,
  [OP_FIN_NEG] = 2, [OP_FIN_MUL] = 3, [OP_FIN_VAL] = 2, [OP_FIN_EQ] = 3,
  [OP_FIN_CASE] = VARIES, [OP_DEC_EQ_FIN] = 3, [OP_DEC_EQ_NAT] = 2,
  [OP_GLUE] = 3, [OP_GLUE_ELEM] = 3,
  [OP_UNGLUE] = 4, [OP_EMPTY] = 0, [OP_ELIM] = VARIES, [OP_ABSURD] = 2
};

typedef enum {
  SCRUT_VALUE,
  SCRUT_TRAP,
  SCRUT_NEUTRAL
} Scrut;

typedef struct {
  const Value **items;
  uint32_t count;
  uint32_t cap;
} ValBuf;

/* A list value: its cons cells, then the end (nil, a trap or a neutral). */
typedef struct {
  const Value *const *cells;
  uint32_t count;
  const Value *tail;
} Spine;

static const Value *oom(Machine *m) {
  diag_fail(m->diag, "OOM", m->def, "the arena limit is reached");
  return NULL;
}

static const Value *internal(Machine *m, const char *what) {
  diag_fail(m->diag, "INTERNAL", m->def, "the evaluator found %s", what);
  return NULL;
}

static int spend(Machine *m, uint64_t steps) {
  if (m->fuel >= steps) {
    m->fuel -= steps;
    return 1;
  }
  m->fuel = 0;
  return diag_fail(m->diag, "EVAL_FUEL", m->def, "the evaluation takes more than %u steps", EVAL_FUEL_STEPS);
}

static int enter(Machine *m) {
  if (m->depth < EVAL_DEPTH_LIMIT) {
    m->depth++;
    return 1;
  }
  return diag_fail(m->diag, "EVAL_DEPTH", m->def, "the evaluation nests deeper than %u levels", EVAL_DEPTH_LIMIT);
}

static Value *new_value(Machine *m, ValKind kind) {
  Value *v = arena_alloc(m->arena, sizeof *v);
  if (v == NULL) {
    oom(m);
    return NULL;
  }
  v->kind = kind;
  v->tag = CLOS_CORE;
  return v;
}

const Value *val_nat(Machine *m, uint64_t n) {
  Value *v = new_value(m, VAL_NAT);
  if (v != NULL)
    v->nat = n;
  return v;
}

const Value *val_big(Machine *m, const uint32_t *limbs, uint64_t count) {
  Value *v = new_value(m, VAL_BIG);
  if (v != NULL) {
    v->limbs = limbs;
    v->nat = count;
  }
  return v;
}

const Value *val_text(Machine *m, const char *bytes, uint64_t len) {
  Value *v = new_value(m, VAL_TEXT);
  if (v != NULL) {
    v->name = bytes;
    v->nat = len;
  }
  return v;
}

static const Value *val_trap(Machine *m) {
  return new_value(m, VAL_TRAP);
}

/* 1 when V is a Nat value: a machine word or a big Nat (S3 chunk B). */
static int is_nat(const Value *v) {
  return v->kind == VAL_NAT || v->kind == VAL_BIG;
}

/* The Big of a Nat value. 0 on exhaustion. */
static int big_of(Machine *m, const Value *v, Big *out) {
  if (v->kind == VAL_BIG) {
    out->limbs = v->limbs;
    out->count = (uint32_t)v->nat;
    return 1;
  }
  return big_from_u64(m->arena, v->nat, out);
}

/* The Nat value of B in normal form (Q3 (a)): a VAL_NAT below 2^64, else a
   VAL_BIG. */
static const Value *nat_of_big(Machine *m, Big b) {
  uint64_t n = 0;
  if (big_fits_u64(b, &n))
    return val_nat(m, n);
  return val_big(m, b.limbs, b.count);
}

typedef int (*BigOp)(Arena *arena, Big x, Big y, Big *out);

/* F at the Nat values X and Y over big limbs, demoted to the normal form. */
static const Value *big_arith(Machine *m, const Value *x, const Value *y, BigOp f) {
  Big bx;
  Big by;
  Big r;
  if (!big_of(m, x, &bx) || !big_of(m, y, &by) || !f(m->arena, bx, by, &r))
    return oom(m);
  return nat_of_big(m, r);
}

const Value *val_var(Machine *m, uint32_t level) {
  Value *v = new_value(m, VAL_VAR);
  if (v != NULL)
    v->nat = level;
  return v;
}

const Value *val_univ(Machine *m, uint64_t level) {
  Value *v = new_value(m, VAL_UNIV);
  if (v != NULL)
    v->nat = level;
  return v;
}

/* The interval is the free De Morgan algebra (R4 (a)). A value is a join
   of conjunctions of literals, in a canonical order (length, then the
   literals), with absorption. ~~i = i, and ~ swaps meet and join. There is
   no rule i /\ ~i = i0: the interval is not Boolean. */

/* 1 when each literal of A is in B. */
static int conj_subset(const IConj *a, const IConj *b) {
  uint32_t i = 0;
  uint32_t j = 0;
  while (i < a->count && j < b->count && a->lits[i] >= b->lits[j]) {
    i += a->lits[i] == b->lits[j];
    j++;
  }
  return i == a->count;
}

static int conj_cmp(const IConj *a, const IConj *b) {
  uint32_t i;
  if (a->count != b->count)
    return a->count < b->count ? -1 : 1;
  for (i = 0; i < a->count; i++)
    if (a->lits[i] != b->lits[i])
      return a->lits[i] < b->lits[i] ? -1 : 1;
  return 0;
}

static int conj_union(Machine *m, const IConj *a, const IConj *b, IConj *out) {
  uint32_t *lits = arena_alloc(m->arena, ((size_t)a->count + b->count + 1u) * sizeof *lits);
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t k = 0;
  if (lits == NULL) {
    oom(m);
    return 0;
  }
  while (i < a->count || j < b->count) {
    int take_a = j == b->count || (i < a->count && a->lits[i] <= b->lits[j]);
    int take_b = i == a->count || (j < b->count && b->lits[j] <= a->lits[i]);
    lits[k++] = take_a ? a->lits[i] : b->lits[j];
    i += take_a;
    j += take_b;
  }
  out->lits = lits;
  out->count = k;
  return 1;
}

/* The normal form of the join of the N conjunctions CS: absorption, then
   the canonical order. */
static const Value *ival_make(Machine *m, const IConj *cs, uint32_t n) {
  IConj *out = arena_alloc(m->arena, ((size_t)n + 1u) * sizeof *out);
  Value *v;
  uint32_t i;
  uint32_t j;
  uint32_t k = 0;
  int drop;
  if (out == NULL)
    return oom(m);
  for (i = 0; i < n; i++) {
    drop = 0;
    for (j = 0; j < n && !drop; j++)
      drop = j != i && conj_subset(&cs[j], &cs[i]) && (cs[j].count < cs[i].count || j < i);
    for (j = k; j > 0 && !drop && conj_cmp(&out[j - 1u], &cs[i]) > 0; j--)
      out[j] = out[j - 1u];
    out[j] = drop ? out[j] : cs[i];
    k += !drop;
  }
  v = new_value(m, VAL_IVAL);
  if (v == NULL)
    return NULL;
  v->conj = out;
  v->conj_count = k;
  return v;
}

static const Value *ival_lit(Machine *m, uint32_t lit) {
  uint32_t *lits = arena_alloc(m->arena, sizeof *lits);
  IConj c;
  if (lits == NULL)
    return oom(m);
  lits[0] = lit;
  c.lits = lits;
  c.count = 1;
  return ival_make(m, &c, 1);
}

const Value *val_ivar(Machine *m, uint32_t level) {
  return ival_lit(m, level * 2u);
}

const Value *val_iend(Machine *m, int side) {
  IConj c;
  c.lits = NULL;
  c.count = 0;
  return ival_make(m, &c, side != 0 ? 1u : 0u);
}

static const Value *ival_join(Machine *m, const Value *a, const Value *b) {
  IConj *cs;
  uint32_t i;
  if (a == NULL || b == NULL)
    return NULL;
  cs = arena_alloc(m->arena, ((size_t)a->conj_count + b->conj_count + 1u) * sizeof *cs);
  if (cs == NULL)
    return oom(m);
  for (i = 0; i < a->conj_count; i++)
    cs[i] = a->conj[i];
  for (i = 0; i < b->conj_count; i++)
    cs[a->conj_count + i] = b->conj[i];
  return ival_make(m, cs, a->conj_count + b->conj_count);
}

static const Value *ival_meet(Machine *m, const Value *a, const Value *b) {
  IConj *cs;
  uint32_t i;
  uint32_t j;
  if (a == NULL || b == NULL)
    return NULL;
  cs = arena_alloc(m->arena, ((size_t)a->conj_count * b->conj_count + 1u) * sizeof *cs);
  if (cs == NULL)
    return oom(m);
  for (i = 0; i < a->conj_count; i++)
    for (j = 0; j < b->conj_count; j++)
      if (!conj_union(m, &a->conj[i], &b->conj[j], &cs[i * b->conj_count + j]))
        return NULL;
  return ival_make(m, cs, a->conj_count * b->conj_count);
}

/* De Morgan: ~(c1 \/ c2) = ~c1 /\ ~c2 and ~(l1 /\ l2) = ~l1 \/ ~l2. */
static const Value *ival_neg(Machine *m, const Value *a) {
  const Value *out = val_iend(m, 1);
  const Value *flipped;
  uint32_t i;
  uint32_t j;
  if (a == NULL)
    return NULL;
  for (i = 0; i < a->conj_count && out != NULL; i++) {
    flipped = val_iend(m, 0);
    for (j = 0; j < a->conj[i].count; j++)
      flipped = ival_join(m, flipped, ival_lit(m, a->conj[i].lits[j] ^ 1u));
    out = ival_meet(m, out, flipped);
  }
  return out;
}

static int ival_equal(const Value *a, const Value *b) {
  uint32_t i;
  if (a->conj_count != b->conj_count)
    return 0;
  for (i = 0; i < a->conj_count; i++)
    if (conj_cmp(&a->conj[i], &b->conj[i]) != 0)
      return 0;
  return 1;
}

int ival_is_end(const Value *v, int side) {
  if (v == NULL || v->kind != VAL_IVAL)
    return 0;
  return side != 0 ? v->conj_count == 1 && v->conj[0].count == 0 : v->conj_count == 0;
}

/* A literal and its negation are adjacent in a sorted conjunction, and the
   literal is the even one. */
static int conj_empty_face(const IConj *c) {
  uint32_t j;
  for (j = 1; j < c->count; j++)
    if ((c->lits[j - 1u] ^ 1u) == c->lits[j])
      return 1;
  return 0;
}

const Value *face_norm(Machine *m, const Value *phi) {
  IConj *cs;
  uint32_t i;
  uint32_t k = 0;
  if (phi == NULL)
    return NULL;
  cs = arena_alloc(m->arena, ((size_t)phi->conj_count + 1u) * sizeof *cs);
  if (cs == NULL)
    return oom(m);
  for (i = 0; i < phi->conj_count; i++)
    if (!conj_empty_face(&phi->conj[i]))
      cs[k++] = phi->conj[i];
  return ival_make(m, cs, k);
}

const Value *face_join(Machine *m, const Value *a, const Value *b) {
  return face_norm(m, ival_join(m, a, b));
}

const Value *face_meet(Machine *m, const Value *a, const Value *b) {
  return face_norm(m, ival_meet(m, a, b));
}

static int face_implies(Machine *m, const Value *a, const Value *b) {
  const Value *face = face_norm(m, a);
  const Value *meet = face == NULL ? NULL : face_meet(m, face, b);
  return face != NULL && meet != NULL && ival_equal(face, meet);
}

const Value *ival_face(Machine *m, const Value *r, uint32_t level, int side) {
  /* The literal that the map makes true, and the one it makes false. */
  uint32_t keep = level * 2u + (side != 0 ? 0u : 1u);
  uint32_t kill = keep ^ 1u;
  IConj *cs;
  uint32_t *lits;
  uint32_t i;
  uint32_t j;
  uint32_t n;
  uint32_t k = 0;
  int dead;
  if (r == NULL)
    return NULL;
  cs = arena_alloc(m->arena, ((size_t)r->conj_count + 1u) * sizeof *cs);
  if (cs == NULL)
    return oom(m);
  for (i = 0; i < r->conj_count; i++) {
    lits = arena_alloc(m->arena, ((size_t)r->conj[i].count + 1u) * sizeof *lits);
    if (lits == NULL)
      return oom(m);
    dead = 0;
    n = 0;
    for (j = 0; j < r->conj[i].count; j++) {
      dead |= r->conj[i].lits[j] == kill;
      lits[n] = r->conj[i].lits[j];
      n += r->conj[i].lits[j] != keep && r->conj[i].lits[j] != kill;
    }
    cs[k].lits = lits;
    cs[k].count = n;
    k += !dead;
  }
  return ival_make(m, cs, k);
}

int env_face(Machine *m, const Env *env, const IConj *c, const Env **out) {
  const Value **vals;
  const Value *v;
  const Env *e;
  uint32_t n = 0;
  uint32_t i;
  uint32_t j;
  for (e = env; e != NULL; e = e->next)
    n++;
  vals = arena_alloc(m->arena, ((size_t)n + 1u) * sizeof *vals);
  if (vals == NULL) {
    oom(m);
    return 0;
  }
  for (e = env, i = 0; e != NULL; e = e->next, i++) {
    v = e->value;
    for (j = 0; j < c->count && v != NULL && v->kind == VAL_IVAL; j++)
      v = ival_face(m, v, c->lits[j] >> 1u, (c->lits[j] & 1u) == 0u);
    if (v == NULL)
      return 0;
    vals[i] = v;
  }
  *out = NULL;
  for (i = n; i > 0; i--) {
    *out = env_push(m, *out, vals[i - 1u]);
    if (*out == NULL)
      return 0;
  }
  return 1;
}

const Value *val_op(Machine *m, ValKind kind, Op op, uint32_t inst, uint32_t field, const Value *const *args, uint32_t argc) {
  const Value **copy;
  Value *v;
  uint32_t i;
  for (i = 0; i < argc; i++)
    if (args[i] == NULL)
      return NULL;
  copy = arena_alloc(m->arena, ((size_t)argc + 1u) * sizeof *copy);
  if (copy == NULL)
    return oom(m);
  v = new_value(m, kind);
  if (v == NULL)
    return NULL;
  for (i = 0; i < argc; i++)
    copy[i] = args[i];
  v->op = op;
  v->inst = inst;
  v->field = field;
  v->args = copy;
  v->argc = argc;
  return v;
}

static const Value *make(Machine *m, ValKind kind, Op op, uint32_t inst, uint32_t argc, const Value *a, const Value *b, const Value *c) {
  const Value *args[3];
  args[0] = a;
  args[1] = b;
  args[2] = c;
  return val_op(m, kind, op, inst, 0, args, argc);
}

const Value *val_make(Machine *m, Op op, uint32_t inst, uint32_t argc, const Value *a, const Value *b, const Value *c) {
  return make(m, VAL_OP, op, inst, argc, a, b, c);
}

static const Value *constant(Machine *m, Op op) {
  return make(m, VAL_OP, op, 0, 0, NULL, NULL, NULL);
}

/* A Bool (Fin 2) value: 1 for yes, 0 for no. */
static const Value *bool_val(Machine *m, int yes) {
  return val_nat(m, yes ? 1u : 0u);
}

const Value *val_arrow(Machine *m, const Value *dom, const Value *cod) {
  Value *v;
  if (dom == NULL || cod == NULL)
    return NULL;
  v = new_value(m, VAL_PI);
  if (v == NULL)
    return NULL;
  v->dom = dom;
  v->arg = cod;
  return v;
}

const Value *val_closure(Machine *m, ValKind kind, const char *name, const Value *dom, const Env *env, const Core *body) {
  Value *v;
  if (body == NULL || (dom == NULL && kind != VAL_LAM && kind != VAL_PLAM))
    return NULL;
  v = new_value(m, kind);
  if (v == NULL)
    return NULL;
  v->name = name;
  v->dom = dom;
  v->env = env;
  v->body = body;
  return v;
}

int val_is(const Value *v, Op op) {
  return v != NULL && v->kind == VAL_OP && v->op == op;
}

const Env *env_push(Machine *m, const Env *env, const Value *value) {
  Env *e;
  if (value == NULL)
    return NULL;
  e = arena_alloc(m->arena, sizeof *e);
  if (e == NULL) {
    oom(m);
    return NULL;
  }
  e->value = value;
  e->next = env;
  return e;
}

static const Value *env_lookup(Machine *m, const Env *env, uint32_t index) {
  uint32_t i;
  for (i = 0; i < index && env != NULL; i++)
    env = env->next;
  if (env == NULL)
    return internal(m, "a variable outside its scope");
  return env->value;
}

const Value *def_value(Machine *m, uint32_t index) {
  DefInfo *d;
  if (index >= m->def_count)
    return internal(m, "an unknown definition");
  d = &m->defs[index];
  if (d->value == NULL)
    d->value = eval_core(m, NULL, d->body);
  return d->value;
}

static Scrut scrut_of(const Value *v) {
  switch (v->kind) {
  case VAL_TRAP:
    return SCRUT_TRAP;
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PAPP:
    return SCRUT_NEUTRAL;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_UNIV:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_OP:
  case VAL_PLAM:
  case VAL_PATHP:
  case VAL_IVAL:
    return SCRUT_VALUE;
  }
  return SCRUT_VALUE;
}

/* 1 when the scrutinee S stops the operation: a trap gives the trap, a
   neutral S gives the stuck operation. */
static int blocked(Machine *m, const Value *s, Op op, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n, const Value **out) {
  switch (scrut_of(s)) {
  case SCRUT_TRAP:
    *out = s;
    return 1;
  case SCRUT_NEUTRAL:
    *out = val_op(m, VAL_STUCK, op, inst, field, a, n);
    return 1;
  case SCRUT_VALUE:
    return 0;
  }
  return 0;
}

/* S, when S is a stuck refUnknown (S6R K3), else NULL. */
static const Value *ref_unknown_of(const Value *s) {
  return s != NULL && s->kind == VAL_STUCK && s->op == OP_REF_UNKNOWN && s->argc == 2u ? s : NULL;
}

/* refUnknown T id for the stuck refUnknown S (S6R K3): an eliminator at S
   gives the refusal at its result type T (C28). */
static const Value *ref_unknown_at(Machine *m, const Value *type, const Value *s) {
  return type == NULL ? NULL : make(m, VAL_STUCK, OP_REF_UNKNOWN, 0, 2, type, s->args[1], NULL);
}

static void buf_init(ValBuf *buf) {
  buf->items = NULL;
  buf->count = 0;
  buf->cap = 0;
}

static int buf_push(Machine *m, ValBuf *buf, const Value *v) {
  const Value **grown;
  uint32_t cap;
  uint32_t i;
  if (v == NULL)
    return 0;
  if (buf->count < buf->cap) {
    buf->items[buf->count++] = v;
    return 1;
  }
  cap = buf->cap == 0 ? 16u : buf->cap * 2u;
  grown = arena_alloc(m->arena, (size_t)cap * sizeof *grown);
  if (grown == NULL) {
    oom(m);
    return 0;
  }
  for (i = 0; i < buf->count; i++)
    grown[i] = buf->items[i];
  buf->items = grown;
  buf->cap = cap;
  buf->items[buf->count++] = v;
  return 1;
}

static int spine_of(Machine *m, const Value *xs, Spine *out) {
  ValBuf cells;
  buf_init(&cells);
  while (val_is(xs, OP_CONS)) {
    if (!buf_push(m, &cells, xs) || !spend(m, 1))
      return 0;
    xs = xs->args[1];
  }
  out->cells = cells.items;
  out->count = cells.count;
  out->tail = xs;
  return xs != NULL;
}

static const Value *build_list(Machine *m, const Value *const *items, uint32_t count, const Value *tail) {
  const Value *acc = tail;
  uint32_t i;
  for (i = count; i > 0 && acc != NULL; i--)
    acc = make(m, VAL_OP, OP_CONS, 0, 2, items[i - 1u], acc, NULL);
  return acc;
}

/* The end of a list result: nil stays nil, a trap stays a trap, a neutral
   end gives OP FIRST END. */
static const Value *list_tail(Machine *m, const Value *tail, Op op, uint32_t inst, const Value *first) {
  switch (scrut_of(tail)) {
  case SCRUT_TRAP:
    return tail;
  case SCRUT_NEUTRAL:
    return make(m, VAL_STUCK, op, inst, 2, first, tail, NULL);
  case SCRUT_VALUE:
    break;
  }
  return val_is(tail, OP_NIL) ? tail : internal(m, "a list that does not end in nil");
}

const Value *apply_value(Machine *m, const Value *fn, const Value *arg) {
  Value *v;
  if (fn == NULL || arg == NULL)
    return NULL;
  /* S6R K3: refUnknown (Pi A B) id at x is refUnknown (B x) id. */
  if (ref_unknown_of(fn) != NULL && fn->args[0]->kind == VAL_PI)
    return ref_unknown_at(m, closure_apply(m, fn->args[0], arg), fn);
  switch (fn->kind) {
  case VAL_LAM:
    return closure_apply(m, fn, arg);
  case VAL_TRAP:
    return fn;
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PAPP:
    v = new_value(m, VAL_APP);
    if (v == NULL)
      return NULL;
    v->dom = fn;
    v->arg = arg;
    return v;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_UNIV:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_OP:
  case VAL_PLAM:
  case VAL_PATHP:
  case VAL_IVAL:
    break;
  }
  return internal(m, "an application of a value that is not a function");
}

static const Value *kan_apply(Machine *m, const Value *k, const Value *r);

const Value *closure_apply(Machine *m, const Value *binder, const Value *arg) {
  const Env *env;
  if (binder == NULL || arg == NULL)
    return NULL;
  if (binder->tag != CLOS_CORE)
    return kan_apply(m, binder, arg);
  if (binder->body == NULL)
    return binder->arg;
  env = env_push(m, binder->env, arg);
  if (env == NULL)
    return NULL;
  return eval_core(m, env, binder->body);
}

const Value *path_apply(Machine *m, const Value *p, const Value *r, const Value *a, const Value *b) {
  Value *v;
  const Value **ends;
  if (p == NULL || r == NULL)
    return NULL;
  if (p->kind == VAL_PLAM)
    return closure_apply(m, p, r);
  if (p->kind == VAL_TRAP)
    return p;
  if (scrut_of(p) != SCRUT_NEUTRAL)
    return internal(m, "a path application of a value that is not a path");
  /* The end point rule: a neutral path at i0 or i1 is its end. */
  if (ival_is_end(r, 0) && a != NULL)
    return a;
  if (ival_is_end(r, 1) && b != NULL)
    return b;
  ends = arena_alloc(m->arena, 2u * sizeof *ends);
  v = new_value(m, VAL_PAPP);
  if (ends == NULL || v == NULL)
    return oom(m);
  ends[0] = a;
  ends[1] = b;
  v->dom = p;
  v->arg = r;
  v->args = ends;
  v->argc = 2;
  return v;
}

static const Value *reduce_part(Machine *m, Op op, const Value *const *a, uint32_t n, Op ctor, uint32_t index) {
  const Value *out;
  if (blocked(m, a[0], op, 0, 0, a, n, &out))
    return out;
  if (!val_is(a[0], ctor))
    return internal(m, "a projection of a value of the wrong type");
  return a[0]->args[index];
}

static const Value *reduce_either(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[2], OP_EITHER, 0, 0, a, n, &out))
    return out;
  if (val_is(a[2], OP_INL))
    return apply_value(m, a[0], a[2]->args[0]);
  if (val_is(a[2], OP_INR))
    return apply_value(m, a[1], a[2]->args[0]);
  return internal(m, "an either of a value that is not a Sum");
}

static const Value *reduce_option(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[2], OP_OPTION_ELIM, 0, 0, a, n, &out))
    return out;
  if (val_is(a[2], OP_NONE))
    return a[0];
  if (val_is(a[2], OP_SOME))
    return apply_value(m, a[1], a[2]->args[0]);
  return internal(m, "an option of a value that is not an Option");
}

static const Value *reduce_pure(Machine *m, uint32_t inst, const Value *x) {
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return make(m, VAL_OP, OP_SOME, 0, 1, x, NULL, NULL);
  case CARRIER_LIST:
    return make(m, VAL_OP, OP_CONS, 0, 2, x, constant(m, OP_NIL), NULL);
  case CARRIER_SUM:
    return make(m, VAL_OP, OP_INR, 0, 1, x, NULL, NULL);
  case CARRIER_CELL:
    break;
  }
  return internal(m, "an unknown carrier");
}

static const Value *map_list(Machine *m, const Value *f, const Value *xs) {
  Spine s;
  ValBuf out;
  uint32_t i;
  if (!spine_of(m, xs, &s))
    return NULL;
  buf_init(&out);
  for (i = 0; i < s.count; i++)
    if (!buf_push(m, &out, apply_value(m, f, s.cells[i]->args[0])))
      return NULL;
  return build_list(m, out.items, out.count, list_tail(m, s.tail, OP_MAP, CARRIER_LIST, f));
}

static const Value *reduce_map(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *x = a[1];
  const Value *out;
  if (blocked(m, x, OP_MAP, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return val_is(x, OP_SOME) ? make(m, VAL_OP, OP_SOME, 0, 1, apply_value(m, a[0], x->args[0]), NULL, NULL) : x;
  case CARRIER_SUM:
    return val_is(x, OP_INR) ? make(m, VAL_OP, OP_INR, 0, 1, apply_value(m, a[0], x->args[0]), NULL, NULL) : x;
  case CARRIER_LIST:
    return map_list(m, a[0], x);
  case CARRIER_CELL:
    break;
  }
  return internal(m, "an unknown carrier");
}

static int buf_append(Machine *m, ValBuf *buf, const Spine *s) {
  uint32_t i;
  for (i = 0; i < s->count; i++)
    if (!buf_push(m, buf, s->cells[i]->args[0]))
      return 0;
  return 1;
}

/* A list bind concatenates the lists F X. A neutral end in any of them
   leaves the whole bind stuck. */
static const Value *bind_list(Machine *m, const Value *const *a, uint32_t n) {
  Spine s;
  Spine t;
  ValBuf out;
  uint32_t i;
  if (!spine_of(m, a[0], &s))
    return NULL;
  if (scrut_of(s.tail) == SCRUT_NEUTRAL)
    return val_op(m, VAL_STUCK, OP_BIND, CARRIER_LIST, 0, a, n);
  buf_init(&out);
  for (i = 0; i < s.count; i++) {
    if (!spine_of(m, apply_value(m, a[1], s.cells[i]->args[0]), &t) || !buf_append(m, &out, &t))
      return NULL;
    if (scrut_of(t.tail) == SCRUT_NEUTRAL)
      return val_op(m, VAL_STUCK, OP_BIND, CARRIER_LIST, 0, a, n);
    if (scrut_of(t.tail) == SCRUT_TRAP)
      return build_list(m, out.items, out.count, t.tail);
  }
  return build_list(m, out.items, out.count, list_tail(m, s.tail, OP_BIND, CARRIER_LIST, a[1]));
}

static const Value *reduce_bind(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *x = a[0];
  const Value *out;
  if (blocked(m, x, OP_BIND, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return val_is(x, OP_SOME) ? apply_value(m, a[1], x->args[0]) : x;
  case CARRIER_SUM:
    return val_is(x, OP_INR) ? apply_value(m, a[1], x->args[0]) : x;
  case CARRIER_LIST:
    return bind_list(m, a, n);
  case CARRIER_CELL:
    break;
  }
  return internal(m, "an unknown carrier");
}

/* fold step z n: step applied n times to z. */
static const Value *reduce_fold_nat(Machine *m, const Value *const *a, uint32_t n) {
  const Value *acc = a[1];
  const Value *out;
  uint64_t k;
  uint64_t count;
  if (blocked(m, a[2], OP_FOLD_NAT, 0, 0, a, n, &out))
    return out;
  if (!is_nat(a[2]))
    return internal(m, "a fold of a value that is not a Nat");
  /* A big bound exceeds the machine's fuel, so this loop fails before
     reaching UINT64_MAX. It still evaluates steps in the usual order. */
  count = a[2]->kind == VAL_BIG ? UINT64_MAX : a[2]->nat;
  for (k = 0; k < count && acc != NULL; k++)
    acc = spend(m, 1) ? apply_value(m, a[0], acc) : NULL;
  return acc;
}

/* fold f z xs: the right fold. The end of the list is folded first. RT is
   the result type of the fold in ENV, or NULL (S6R K3): a refUnknown end
   gives refUnknown RT id. */
static const Value *reduce_fold_list(Machine *m, const Value *const *a, const Env *env, const Core *rt) {
  const Value *acc = NULL;
  Spine s;
  uint32_t i;
  if (!spine_of(m, a[2], &s))
    return NULL;
  switch (scrut_of(s.tail)) {
  case SCRUT_TRAP:
    acc = s.tail;
    break;
  case SCRUT_NEUTRAL: {
    const Value *res = rt == NULL ? NULL : eval_core(m, env, rt);
    if (rt != NULL && res == NULL)
      return NULL;
    acc = res != NULL && ref_unknown_of(s.tail) != NULL ? ref_unknown_at(m, res, s.tail) : make(m, VAL_STUCK, OP_FOLD_LIST, 0, 3, a[0], a[1], s.tail);
    /* Keep the type on the tail, including when a step wraps it. */
    if (acc != NULL && acc->kind == VAL_STUCK && acc->op == OP_FOLD_LIST)
      ((Value *)acc)->dom = res;
    break;
  }
  case SCRUT_VALUE:
    acc = val_is(s.tail, OP_NIL) ? a[1] : internal(m, "a list that does not end in nil");
    break;
  }
  for (i = s.count; i > 0 && acc != NULL; i--)
    acc = apply_value(m, apply_value(m, a[0], s.cells[i - 1u]->args[0]), acc);
  return acc;
}

/* The number of constructors of the block of F (S6), or of F. */
static uint32_t block_ctors(const Machine *m, const FamilyInfo *f) {
  const FamilyInfo *last = &m->families[f->block_first + f->block_count - 1u];
  return f->block_count > 1u ? last->first_ctor + last->ctor_count - m->families[f->block_first].first_ctor : f->ctor_count;
}

/* The result type of a fold at FAMILY: CASES[NCASES], or for a block of n
   sorts (S6) the target of FAMILY in the n targets before it. */
static const Value *fold_res(const Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases) {
  const FamilyInfo *f = &m->families[family];
  return f->block_count > 1u ? cases[ncases - f->block_count + (family - f->block_first)] : cases[ncases];
}

/* The sort of the recursive field FIELD of a constructor of FAMILY: the
   head of its type, a sort of the block (S6). */
static uint32_t field_sort(const FieldInfo *field, uint32_t family) {
  const Core *t = field->path || field->continuation ? field->type->b : field->type;
  return t != NULL && t->kind == CORE_OP && t->op == OP_FAMILY ? t->inst : family;
}

/* CASES holds NCASES cases, then the result type RES at CASES[NCASES]. A
   block fold (S6) has the targets of its sorts at the end of the cases,
   and RES is the target of FAMILY. */
static const Value *stuck_fold(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x) {
  const Value **args = arena_alloc(m->arena, ((size_t)ncases + 2u) * sizeof *args);
  uint32_t i;
  if (args == NULL)
    return oom(m);
  for (i = 0; i < ncases; i++)
    args[i] = cases[i];
  args[ncases] = fold_res(m, family, cases, ncases);
  args[ncases + 1u] = x;
  return val_op(m, VAL_STUCK, OP_FOLD_FAMILY, family, 0, args, ncases + 2u);
}

/* A neutral elim over the domain family FAMILY (S7 B1, O22). A holds the
   motive, the N - 2 cases and the scrutinee at A[N - 1]. */
static const Value *stuck_elim(Machine *m, uint32_t family, const Value *const *a, uint32_t n) {
  return val_op(m, VAL_STUCK, OP_ELIM, n - 2u, family + 1u, a, n);
}

static const Value *fold_path(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, const Value *p);
static const Value *fold_hcomp(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x);
static const Value *fold_path_field(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, uint32_t field);
static const Value *fold_square(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, const Value *s);
static const Value *elim_family(Machine *m, uint32_t family, const Value *const *a, uint32_t n);
static const Value *kan_closure(Machine *m, ValKind kind, ClosTag tag, uint32_t field, uint32_t argc, const Value *x, const Value *y, const Value *z);
static const Value *kan_closure4(Machine *m, ValKind kind, ClosTag tag, uint32_t field, uint32_t argc, const Value *x, const Value *y, const Value *z, const Value *w);

/* fold case... x over a domain family: the case of the constructor of X,
   applied to its fields. A recursive field and a path field (S4 item 14)
   arrive folded. CASES holds
   NCASES cases, then the result type RES. A formal hcomp goes to
   fold_hcomp. */
static const Value *fold_family(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x) {
  const FamilyInfo *fam;
  const CtorInfo *ctor;
  const Value *acc;
  const Value *field;
  uint32_t base;
  uint32_t i;
  if (x == NULL)
    return NULL;
  switch (scrut_of(x)) {
  case SCRUT_TRAP:
    return x;
  case SCRUT_NEUTRAL:
    return ref_unknown_of(x) != NULL ? ref_unknown_at(m, fold_res(m, family, cases, ncases), x) : stuck_fold(m, family, cases, ncases, x);
  case SCRUT_VALUE:
    break;
  }
  if (val_is(x, OP_HCOMP) && x->argc == 4u)
    return fold_hcomp(m, family, cases, ncases, x);
  if (!val_is(x, OP_CTOR) || x->inst >= m->ctor_count || family >= m->family_count)
    return internal(m, "a fold of a value that is not in the family");
  fam = &m->families[family];
  ctor = &m->ctors[x->inst];
  /* The constructor slots of a block are contiguous (S6). */
  base = m->families[fam->block_first].first_ctor;
  if (ctor->family != family || x->inst < base || x->inst - base >= ncases || x->argc != fam->param_count + ctor->field_count + ctor->interval_count)
    return internal(m, "a fold of a constructor of another family");
  if (ctor->interval_count > 2u)
    return internal(m, "a fold of a constructor with more than 2 interval fields");
  if (!enter(m))
    return NULL;
  acc = cases[x->inst - base];
  for (i = 0; i < ctor->field_count && acc != NULL; i++) {
    field = x->args[fam->param_count + i];
    /* A continuation field k arrives as fun b => fold (k b) (S2 K3). */
    acc = apply_value(m, acc, ctor->fields[i].recursive ? fold_family(m, field_sort(&ctor->fields[i], family), cases, ncases, field) : ctor->fields[i].path ? fold_path_field(m, family, cases, ncases, x, i) : ctor->fields[i].continuation ? kan_closure(m, VAL_LAM, CLOS_FOLD_CONT, field_sort(&ctor->fields[i], family), 2u, field, stuck_fold(m, family, cases, ncases, x), NULL) : field);
  }
  if (ctor->interval_count == 1u && acc != NULL)
    acc = fold_path(m, family, cases, ncases, x, acc);
  if (ctor->interval_count == 2u && acc != NULL)
    acc = fold_square(m, family, cases, ncases, x, acc);
  m->depth--;
  return acc;
}

/* (case args') @ r for the path constructor value X at the interval R (S4
   R4 (a)). P is the case at the folded fields. The ends are the folds of
   X at i0 and i1, for a neutral P. */
static const Value *fold_path(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, const Value *p) {
  const Value *ends[2];
  const Value **a;
  uint32_t i;
  int side;
  for (side = 0; side < 2; side++) {
    a = arena_alloc(m->arena, (size_t)x->argc * sizeof *a);
    if (a == NULL)
      return oom(m);
    for (i = 0; i + 1u < x->argc; i++)
      a[i] = x->args[i];
    a[x->argc - 1u] = val_iend(m, side);
    ends[side] = fold_family(m, family, cases, ncases, val_ctor(m, x->inst, a, x->argc));
  }
  return path_apply(m, p, x->args[x->argc - 1u], ends[0], ends[1]);
}

/* The constructor value X with its 2 interval fields set to R1 and R2. At
   an end it reduces to its boundary. */
static const Value *ctor_at2(Machine *m, const Value *x, const Value *r1, const Value *r2) {
  const Value **a = arena_alloc(m->arena, (size_t)x->argc * sizeof *a);
  uint32_t i;
  if (a == NULL || r1 == NULL || r2 == NULL)
    return a == NULL ? oom(m) : NULL;
  for (i = 0; i + 2u < x->argc; i++)
    a[i] = x->args[i];
  a[x->argc - 2u] = r1;
  a[x->argc - 1u] = r2;
  return val_ctor(m, x->inst, a, x->argc);
}

/* (case args') @ r1 @ r2 for the constructor value X with 2 interval
   fields (S4 item 15). S is the case at the folded fields, a square. For
   a neutral S, the outer ends are the folds of X at i0 and i1 in the
   first interval field, as paths in the second (CLOS_FOLD_FACE); the
   inner ends are the folds of X at r1 and i0, i1. */
static const Value *fold_square(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, const Value *s) {
  const Value *stuck = stuck_fold(m, family, cases, ncases, x);
  const Value *r1 = x->args[x->argc - 2u];
  const Value *outer[2];
  const Value *inner[2];
  int side;
  for (side = 0; side < 2; side++) {
    outer[side] = kan_closure4(m, VAL_PLAM, CLOS_FOLD_FACE, family, 3u, x, val_iend(m, side), stuck, NULL);
    inner[side] = fold_family(m, family, cases, ncases, ctor_at2(m, x, r1, val_iend(m, side)));
    if (outer[side] == NULL || inner[side] == NULL)
      return NULL;
  }
  return path_apply(m, path_apply(m, s, r1, outer[0], outer[1]), x->args[x->argc - 1u], inner[0], inner[1]);
}

/* The elim A[0 .. N - 1] at X in place of the scrutinee A[N - 1] (S7 B1). */
static const Value *elim_at(Machine *m, uint32_t family, const Value *const *a, uint32_t n, const Value *x) {
  const Value **b = arena_alloc(m->arena, (size_t)n * sizeof *b);
  uint32_t i;
  if (b == NULL)
    return oom(m);
  for (i = 0; i + 1u < n; i++)
    b[i] = a[i];
  b[n - 1u] = x;
  return elim_family(m, family, b, n);
}

/* (case fields) @ r for the path constructor value x = A[N - 1] at r. P is
   the case at the fields. The ends are the elims of x at i0 and i1, for a
   neutral P (as fold_path). */
static const Value *elim_path(Machine *m, uint32_t family, const Value *const *a, uint32_t n, const Value *p) {
  const Value *x = a[n - 1u];
  const Value *ends[2];
  const Value **b;
  uint32_t i;
  int side;
  for (side = 0; side < 2; side++) {
    b = arena_alloc(m->arena, (size_t)x->argc * sizeof *b);
    if (b == NULL)
      return oom(m);
    for (i = 0; i + 1u < x->argc; i++)
      b[i] = x->args[i];
    b[x->argc - 1u] = val_iend(m, side);
    ends[side] = elim_at(m, family, a, n, val_ctor(m, x->inst, b, x->argc));
  }
  return path_apply(m, p, x->args[x->argc - 1u], ends[0], ends[1]);
}

/* (case fields) @ r1 @ r2 for the constructor value x = A[N - 1] with 2
   interval fields. S is the case at the fields, a square. The faces are
   as in fold_square, over a stuck elim of x (CLOS_FOLD_FACE). */
static const Value *elim_square(Machine *m, uint32_t family, const Value *const *a, uint32_t n, const Value *s) {
  const Value *x = a[n - 1u];
  const Value *stuck = stuck_elim(m, family, a, n);
  const Value *r1 = x->args[x->argc - 2u];
  const Value *outer[2];
  const Value *inner[2];
  int side;
  for (side = 0; side < 2; side++) {
    outer[side] = kan_closure4(m, VAL_PLAM, CLOS_FOLD_FACE, family, 3u, x, val_iend(m, side), stuck, NULL);
    inner[side] = elim_at(m, family, a, n, ctor_at2(m, x, r1, val_iend(m, side)));
    if (outer[side] == NULL || inner[side] == NULL)
      return NULL;
  }
  return path_apply(m, path_apply(m, s, r1, outer[0], outer[1]), x->args[x->argc - 1u], inner[0], inner[1]);
}

/* elim P c1 .. cn x over the domain family FAMILY (S7 B1, O21). The case
   of the constructor of x takes each field, then for a recursive field its
   elim, and for a path field the elim along it (CLOS_FOLD_PATH). A path
   constructor and a squash go to elim_path and elim_square. A neutral x
   and a formal hcomp stay neutral (O22). */
static const Value *elim_family(Machine *m, uint32_t family, const Value *const *a, uint32_t n) {
  const Value *x = a[n - 1u];
  const FamilyInfo *fam;
  const CtorInfo *ctor;
  const FieldInfo *f;
  const Value *acc;
  const Value *out;
  uint32_t p;
  uint32_t i;
  if (x == NULL)
    return NULL;
  if (ref_unknown_of(x) != NULL)
    return ref_unknown_at(m, apply_value(m, a[0], x), x);
  if (blocked(m, x, OP_ELIM, n - 2u, family + 1u, a, n, &out))
    return out;
  if (val_is(x, OP_HCOMP))
    return stuck_elim(m, family, a, n);
  if (family >= m->family_count || !val_is(x, OP_CTOR) || x->inst >= m->ctor_count)
    return internal(m, "an elim of a value that is not in the family");
  fam = &m->families[family];
  ctor = &m->ctors[x->inst];
  p = fam->param_count;
  if (ctor->family != family || x->inst < fam->first_ctor || x->inst - fam->first_ctor >= n - 2u || x->argc != p + ctor->field_count + ctor->interval_count)
    return internal(m, "an elim of a constructor of another family");
  if (ctor->interval_count > 2u)
    return internal(m, "an elim of a constructor with more than 2 interval fields");
  if (!enter(m))
    return NULL;
  acc = a[1u + (x->inst - fam->first_ctor)];
  for (i = 0; i < ctor->field_count && acc != NULL; i++) {
    f = &ctor->fields[i];
    acc = apply_value(m, acc, x->args[p + i]);
    if (f->recursive && acc != NULL)
      acc = apply_value(m, acc, elim_at(m, family, a, n, x->args[p + i]));
    if (f->continuation && acc != NULL)
      acc = apply_value(m, acc, kan_closure(m, VAL_LAM, CLOS_FOLD_CONT, family, 2u, x->args[p + i], stuck_elim(m, family, a, n), NULL));
    if (f->path && acc != NULL)
      acc = apply_value(m, acc, kan_closure4(m, VAL_PLAM, CLOS_FOLD_PATH, family, 4u, x->args[p + i], x->args[p + f->ends[0]], x->args[p + f->ends[1]], stuck_elim(m, family, a, n)));
  }
  if (ctor->interval_count == 1u && acc != NULL)
    acc = elim_path(m, family, a, n, acc);
  if (ctor->interval_count == 2u && acc != NULL)
    acc = elim_square(m, family, a, n, acc);
  m->depth--;
  return acc;
}

/* elim P c1 .. cn x with n = INST + 2 (USER ruling S4) over the domain
   family FIELD - 1 (S7 B1). virtu-lang has no Sum or Unit elim, so FIELD
   is never 0: an elim without a family is INTERNAL. */
static const Value *reduce_elim(Machine *m, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  if (field == 0u || n != inst + 2u)
    return internal(m, "an elim without a domain family");
  return elim_family(m, field - 1u, a, n);
}

static const Value *filter_option(Machine *m, const Value *const *a, uint32_t n) {
  const Value *test;
  const Value *out;
  if (!val_is(a[1], OP_SOME))
    return a[1];
  test = apply_value(m, a[0], a[1]->args[0]);
  if (test == NULL)
    return NULL;
  if (blocked(m, test, OP_FILTER, CARRIER_OPTION, 0, a, n, &out))
    return out;
  return test->kind == VAL_NAT && test->nat == 1u ? a[1] : constant(m, OP_NONE);
}

static const Value *filter_list(Machine *m, const Value *const *a) {
  const Value *test;
  Spine s;
  ValBuf kept;
  uint32_t i;
  if (!spine_of(m, a[1], &s))
    return NULL;
  buf_init(&kept);
  for (i = 0; i < s.count; i++) {
    test = apply_value(m, a[0], s.cells[i]->args[0]);
    if (test == NULL)
      return NULL;
    switch (scrut_of(test)) {
    case SCRUT_TRAP:
      return build_list(m, kept.items, kept.count, test);
    case SCRUT_NEUTRAL:
      return build_list(m, kept.items, kept.count, make(m, VAL_STUCK, OP_FILTER, CARRIER_LIST, 2, a[0], s.cells[i], NULL));
    case SCRUT_VALUE:
      break;
    }
    if (test->kind == VAL_NAT && test->nat == 1u && !buf_push(m, &kept, s.cells[i]->args[0]))
      return NULL;
  }
  return build_list(m, kept.items, kept.count, list_tail(m, s.tail, OP_FILTER, CARRIER_LIST, a[0]));
}

static const Value *reduce_filter(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[1], OP_FILTER, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return filter_option(m, a, n);
  case CARRIER_LIST:
    return filter_list(m, a);
  case CARRIER_SUM:
  case CARRIER_CELL:
    break;
  }
  return internal(m, "a filter with no instance");
}

/* 1 when S stops a Nat operation: blocked(), and a glue element too. A
   glue element is a point of a Glue type off its faces (a `ua` line at a
   free i), so no Nat operation can read it: the operation is stuck (S6
   R13 (a)). */
static int nat_blocked(Machine *m, const Value *s, Op op, const Value *const *a, uint32_t n, const Value **out) {
  if (val_is(s, OP_GLUE_ELEM)) {
    *out = val_op(m, VAL_STUCK, op, 0, 0, a, n);
    return 1;
  }
  return blocked(m, s, op, 0, 0, a, n, out);
}

/* 1 when both arguments are numbers. Else *OUT is the trap or the stuck
   operation. */
static int nat_args(Machine *m, Op op, const Value *const *a, uint32_t n, uint64_t *x, uint64_t *y, const Value **out) {
  *out = a[0]->kind == VAL_TRAP ? a[0] : a[1];
  if (a[0]->kind == VAL_TRAP || a[1]->kind == VAL_TRAP)
    return 0;
  if (nat_blocked(m, a[0], op, a, n, out) || nat_blocked(m, a[1], op, a, n, out))
    return 0;
  if (!is_nat(a[0]) || !is_nat(a[1])) {
    *out = internal(m, "a Nat operation on a value that is not a Nat");
    return 0;
  }
  *x = a[0]->nat;
  *y = a[1]->nat;
  return 1;
}

static const Value *nat_add(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_ADD, a, n, &x, &y, &out))
    return out;
  if (a[0]->kind == VAL_BIG || a[1]->kind == VAL_BIG || x + y < x)
    return big_arith(m, a[0], a[1], big_add);
  return val_nat(m, x + y);
}

static const Value *nat_sub(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_SUB, a, n, &x, &y, &out))
    return out;
  if (a[0]->kind == VAL_BIG || a[1]->kind == VAL_BIG)
    return big_arith(m, a[0], a[1], big_sub);
  return val_nat(m, x < y ? 0 : x - y);
}

static const Value *nat_mul(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_MUL, a, n, &x, &y, &out))
    return out;
  if (a[0]->kind == VAL_BIG || a[1]->kind == VAL_BIG || (y != 0 && x > UINT64_MAX / y))
    return big_arith(m, a[0], a[1], big_mul);
  return val_nat(m, x * y);
}

/* succ a: a + 1. */
static const Value *nat_succ(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (a[0]->kind == VAL_TRAP)
    return a[0];
  if (nat_blocked(m, a[0], OP_SUCC, a, n, &out))
    return out;
  if (!is_nat(a[0]))
    return internal(m, "a Nat operation on a value that is not a Nat");
  if (a[0]->kind == VAL_BIG || a[0]->nat == UINT64_MAX) {
    const Value *one = val_nat(m, 1u);
    return one == NULL ? NULL : big_arith(m, a[0], one, big_add);
  }
  return val_nat(m, a[0]->nat + 1u);
}

/* finAdd n a b: (a + b) mod n, with no overflow. A[0] is n. */
static const Value *fin_add(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_FIN_ADD, a, n, &k, &x, &out))
    return out;
  if (a[2]->kind == VAL_TRAP)
    return a[2];
  if (nat_blocked(m, a[2], OP_FIN_ADD, a, n, &out))
    return out;
  if (a[2]->kind != VAL_NAT)
    return internal(m, "a Fin operation on a value that is not a number");
  y = a[2]->nat;
  if (k == 0)
    return internal(m, "a Fin 0 operation");
  x %= k;
  y %= k;
  return val_nat(m, x >= k - y ? x - (k - y) : x + y);
}

/* finNeg n a: (n - a) mod n. A[0] is n. */
static const Value *fin_neg(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  if (!nat_args(m, OP_FIN_NEG, a, n, &k, &x, &out))
    return out;
  if (k == 0)
    return internal(m, "a Fin 0 operation");
  x %= k;
  return val_nat(m, x == 0 ? 0 : k - x);
}

/* (X + Y) mod K for X, Y < K, with no overflow. */
static uint64_t add_mod(uint64_t x, uint64_t y, uint64_t k) {
  return x >= k - y ? x - (k - y) : x + y;
}

/* (X * Y) mod K for X, Y < K, with no overflow: double and add. */
static uint64_t mul_mod(uint64_t x, uint64_t y, uint64_t k) {
  uint64_t r = 0;
  for (; y != 0u; y >>= 1) {
    r = (y & 1u) != 0u ? add_mod(r, x, k) : r;
    x = add_mod(x, x, k);
  }
  return r;
}

/* The arguments n a b of a Fin operation as numbers in *K, *X and *Y, with
   n not 0. Else *OUT is the trap, the stuck operation or the error. */
static int fin_args3(Machine *m, Op op, const Value *const *a, uint32_t n, uint64_t *k, uint64_t *x, uint64_t *y, const Value **out) {
  if (!nat_args(m, op, a, n, k, x, out))
    return 0;
  *out = a[2];
  if (a[2]->kind == VAL_TRAP || nat_blocked(m, a[2], op, a, n, out))
    return 0;
  if (a[2]->kind != VAL_NAT) {
    *out = internal(m, "a Fin operation on a value that is not a number");
    return 0;
  }
  if (*k == 0) {
    *out = internal(m, "a Fin 0 operation");
    return 0;
  }
  *y = a[2]->nat;
  return 1;
}

/* finMul n a b: (a * b) mod n, with no overflow. A[0] is n. */
static const Value *fin_mul(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!fin_args3(m, OP_FIN_MUL, a, n, &k, &x, &y, &out))
    return out;
  return val_nat(m, mul_mod(x % k, y % k, k));
}

/* finVal n a: the number a as a Nat. A[0] is n. */
static const Value *fin_val(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  if (!nat_args(m, OP_FIN_VAL, a, n, &k, &x, &out))
    return out;
  if (k == 0)
    return internal(m, "a Fin 0 operation");
  return val_nat(m, x % k);
}

/* finEq: 1 in Fin 2 when X = Y, else 0. */
static const Value *eq_bit(Machine *m, uint64_t x, uint64_t y) {
  return val_nat(m, x == y ? 1u : 0u);
}

/* finEq n a b: 1 in Fin 2 when a = b, else 0. A[0] is n. */
static const Value *fin_eq(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!fin_args3(m, OP_FIN_EQ, a, n, &k, &x, &y, &out))
    return out;
  return eq_bit(m, x % k, y % k);
}

/* S5b item 7: decEqFin and decEqNat build a core term and evaluate it. The
   two builders below are the quote helpers at the end of this file. */
static Core *new_core(Machine *m, CoreKind kind);
static const Core *core_leaf(Machine *m, CoreKind kind, uint64_t nat);

/* A core name with a copy of its N arguments; NULL when one is NULL. */
static const Core *core_opn(Machine *m, Op op, const Core *const *args, uint32_t n) {
  const Core **copy = arena_alloc(m->arena, ((size_t)n + 1u) * sizeof *copy);
  Core *c;
  uint32_t i;
  if (copy == NULL) {
    oom(m);
    return NULL;
  }
  for (i = 0; i < n; i++) {
    if (args[i] == NULL)
      return NULL;
    copy[i] = args[i];
  }
  c = new_core(m, CORE_OP);
  if (c == NULL)
    return NULL;
  c->op = op;
  c->args = copy;
  c->argc = n;
  return c;
}

/* fun NAME => B: a LAM or a PLAM. */
static const Core *core_binder(Machine *m, CoreKind kind, const char *name, const Core *b) {
  Core *c = b == NULL ? NULL : new_core(m, kind);
  if (c == NULL)
    return NULL;
  c->name = name;
  c->b = b;
  return c;
}

/* The variable at de Bruijn index INDEX. */
static const Core *core_var(Machine *m, uint32_t index) {
  Core *c = new_core(m, CORE_VAR);
  if (c != NULL)
    c->index = index;
  return c;
}

/* Refute `Path T X Y` at distinct literals by transporting unit along a
   line from Unit to Empty. finEq and natEq both return Fin 2 (Bool), so
   the line is a finCase on the test. Under the two binders
   p is index 1 and i is index 0. The ends of p are the literals. */
static const Core *refute_core(Machine *m, int fin, uint64_t k, const Core *x, const Core *y) {
  const Core *args[3];
  const Core **ends = arena_alloc(m->arena, 2u * sizeof *ends);
  Core *at = ends == NULL ? NULL : new_core(m, CORE_PAPP);
  Core *cases;
  if (ends == NULL) {
    oom(m);
    return NULL;
  }
  if (at == NULL)
    return NULL;
  ends[0] = x;
  ends[1] = y;
  at->a = core_var(m, 1);
  at->b = core_var(m, 0);
  at->args = ends;
  at->argc = 2;
  if (ends[0] == NULL || ends[1] == NULL || at->a == NULL || at->b == NULL)
    return NULL;
  args[0] = core_leaf(m, CORE_NAT, k);
  args[1] = x;
  args[2] = at;
  args[0] = fin ? core_opn(m, OP_FIN_EQ, args, 3) : core_opn(m, OP_NAT_EQ, args + 1, 2);
  args[1] = core_opn(m, OP_EMPTY, NULL, 0);
  args[2] = core_opn(m, OP_UNIT, NULL, 0);
  cases = (Core *)core_opn(m, OP_FIN_CASE, args, 3);
  if (cases == NULL)
    return NULL;
  cases->a = core_leaf(m, CORE_UNIV, 0);
  if (cases->a == NULL)
    return NULL;
  args[0] = core_binder(m, CORE_PLAM, "i", cases);
  args[1] = core_leaf(m, CORE_IZERO, 0);
  args[2] = core_opn(m, OP_UNIT_VAL, NULL, 0);
  return core_binder(m, CORE_LAM, "p", core_opn(m, OP_TRANSP, args, 3));
}

/* decEqFin n a b and decEqNat a b (S5b item 7). At literals, the value of
   `inl refl` when a = b, else of the refutation of refute_core: each is a
   core term with the literals in place, evaluated in the empty
   environment, so the result is an ordinary closure that prints and checks
   again. A neutral argument leaves the operation stuck (fin_args3 and
   nat_args). A[0] is n for decEqFin. */
static const Value *dec_eq(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t k = 0;
  uint64_t x = 0;
  uint64_t y = 0;
  int fin = op == OP_DEC_EQ_FIN;
  int ok = fin ? fin_args3(m, op, a, n, &k, &x, &y, &out) : nat_args(m, op, a, n, &x, &y, &out);
  const Core *body;
  const Core *cx;
  const Core *cy;
  int equal;
  if (!ok)
    return out;
  x = fin ? x % k : x;
  y = fin ? y % k : y;
  if (!fin && (a[0]->kind == VAL_BIG || a[1]->kind == VAL_BIG)) {
    Big bx;
    Big by;
    if (!big_of(m, a[0], &bx) || !big_of(m, a[1], &by))
      return oom(m);
    equal = big_cmp(bx, by) == 0;
  } else {
    equal = x == y;
  }
  cx = fin ? core_leaf(m, CORE_NAT, x) : quote_value(m, 0, a[0]);
  cy = fin ? core_leaf(m, CORE_NAT, y) : quote_value(m, 0, a[1]);
  body = equal ? core_binder(m, CORE_PLAM, "_", cx) : refute_core(m, fin, k, cx, cy);
  return eval_core(m, NULL, core_opn(m, equal ? OP_INL : OP_INR, &body, 1));
}

/* The finAll case term: k, body0, ..., body(n-1). Body j at the literal j;
   a neutral k keeps the full argument list. At a literal, eval_op evals
   only the body of that literal, so a literal seldom gets here. */
static const Value *fin_case(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (nat_blocked(m, a[0], OP_FIN_CASE, a, n, &out))
    return out;
  if (a[0]->kind != VAL_NAT || a[0]->nat >= n - 1u)
    return internal(m, "a Fin case on a value out of range");
  return a[a[0]->nat + 1u];
}

static const Value *nat_test(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, op, a, n, &x, &y, &out))
    return out;
  if (a[0]->kind == VAL_BIG || a[1]->kind == VAL_BIG) {
    Big bx;
    Big by;
    int order;
    if (!big_of(m, a[0], &bx) || !big_of(m, a[1], &by))
      return oom(m);
    order = big_cmp(bx, by);
    return bool_val(m, op == OP_NAT_EQ ? order == 0 : order <= 0);
  }
  return bool_val(m, op == OP_NAT_EQ ? x == y : x <= y);
}

static const Value *reduce_proj(Machine *m, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  const Value *x = a[0];
  const Value *out;
  uint32_t at;
  if (blocked(m, x, OP_PROJ, inst, field, a, n, &out))
    return out;
  /* A formal hcomp has no fields: the projection is stuck (S4 R3 (a)). */
  if (val_is(x, OP_HCOMP))
    return val_op(m, VAL_STUCK, OP_PROJ, inst, field, a, n);
  if (!val_is(x, OP_CTOR) || x->inst != inst || inst >= m->ctor_count || m->ctors[inst].family >= m->family_count)
    return internal(m, "a projection of a value of the wrong family");
  at = m->families[m->ctors[inst].family].param_count + field;
  return at < x->argc ? x->args[at] : internal(m, "a projection of a missing field");
}

static uint32_t varied_arity(const Machine *m, Op op, uint32_t inst) {
  const CtorInfo *c = inst < m->ctor_count ? &m->ctors[inst] : NULL;
  const FamilyInfo *f = inst < m->family_count ? &m->families[inst] : NULL;
  if (op == OP_CTOR && c != NULL && c->family < m->family_count)
    return m->families[c->family].param_count + c->field_count + c->interval_count;
  if (op == OP_FAMILY && f != NULL)
    return f->param_count + f->index_count;
  if (op == OP_FOLD_FAMILY && f != NULL)
    return block_ctors(m, f) + (f->block_count > 1u ? f->block_count : 0u) + 2u;
  if (op == OP_ELIM)
    return inst + 2u;
  return UINT32_MAX;
}

static int arity_ok(const Machine *m, Op op, uint32_t inst, uint32_t n) {
  if ((size_t)op >= sizeof OP_ARITY)
    return 0;
  if (op == OP_SYSTEM)
    return n % 2u == 0u;
  if (op == OP_FIN_CASE)
    return n >= 1u;
  if (OP_ARITY[op] != VARIES)
    return n == (uint32_t)OP_ARITY[op];
  return n == varied_arity(m, op, inst);
}

/* The Kan operations (S2, R11 (a)). A line L is a path value in U, and
   L(r) is L at the interval value r. A Kan rule gives a value at once; a
   rule that needs a function of an interval value gives a closure tag. */

#define KAN_FRESH_BASE 0x20000000u
#define KAN_FRESH_MASK 0x0fffffffu

static const Value *line_at(Machine *m, const Value *line, const Value *r) {
  return path_apply(m, line, r, NULL, NULL);
}

/* L at a fresh interval variable, never at i0: a neutral line can be a
   type former at i0. Eval has no level, so the Machine gives a level above
   each context level. A Kan rule reads only the head kind. */
static const Value *fresh_ivar(Machine *m) {
  uint32_t level = KAN_FRESH_BASE + (m->fresh & KAN_FRESH_MASK);
  m->fresh++;
  return val_ivar(m, level);
}

static const Value *line_head(Machine *m, const Value *line) {
  return line_at(m, line, fresh_ivar(m));
}

/* T when it is a KIND value (the former of a line at each point), else an
   internal error. */
static const Value *line_former(Machine *m, const Value *t, ValKind kind) {
  if (t == NULL || t->kind == kind)
    return t;
  return internal(m, "a line that changes its type former");
}

static const Value *kan_closure4(Machine *m, ValKind kind, ClosTag tag, uint32_t field, uint32_t argc, const Value *x, const Value *y, const Value *z, const Value *w) {
  const Value **args;
  Value *v;
  if (x == NULL || (argc > 1u && y == NULL) || (argc > 2u && z == NULL) || (argc > 3u && w == NULL))
    return NULL;
  args = arena_alloc(m->arena, 4u * sizeof *args);
  if (args == NULL)
    return oom(m);
  v = new_value(m, kind);
  if (v == NULL)
    return NULL;
  args[0] = x;
  args[1] = y;
  args[2] = z;
  args[3] = w;
  v->name = kind == VAL_PLAM ? "i" : "x";
  v->tag = tag;
  v->field = field;
  v->args = args;
  v->argc = argc;
  return v;
}

static const Value *kan_closure(Machine *m, ValKind kind, ClosTag tag, uint32_t field, uint32_t argc, const Value *x, const Value *y, const Value *z) {
  return kan_closure4(m, kind, tag, field, argc, x, y, z, NULL);
}

/* unfold g v, for the unfold U = unfold g s (S4 Q4): the same step g, and
   the types of U for source printing. */
static const Value *counfold_at(Machine *m, const Value *u, const Value *v) {
  const Value *args[2];
  Value *out;
  args[0] = u->args[0];
  args[1] = v;
  out = (Value *)val_op(m, VAL_OP, OP_COUNFOLD, u->inst, 0, args, 2);
  if (out != NULL) {
    out->dom = u->dom;
    out->arg = u->arg;
  }
  return out;
}

/* The point field I of a layer constructor after out of the unfold U (S4
   Q2), with V the field value and T its type, X at index I. An X field
   becomes unfold g V. A Pi chain field with the codomain X becomes
   fun r1 ... rn => unfold g (V r1 ... rn). Another field stays. */
static const Value *out_field(Machine *m, const Value *u, const Core *t, uint32_t i, const Value *v) {
  uint32_t depth = 0;
  for (; t->kind == CORE_PI; t = t->b)
    depth++;
  if (t->kind != CORE_VAR || t->index != i + depth)
    return v;
  return depth == 0u ? counfold_at(m, u, v) : kan_closure(m, VAL_LAM, CLOS_OUT_PI, depth, 2, u, v, NULL);
}

static const Value *transp(Machine *m, const Value *line, const Value *phi, const Value *base);
static const Value *hcomp(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base);
static const Value *hcomp_stuck(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base);
static const Value *comp(Machine *m, const Value *line, const Value *phi, const Value *sys, const Value *base);
static const Value *hfill(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base, const Value *r);
static const Value *sys_map(Machine *m, const Value *sys, ClosTag tag, uint32_t field, uint32_t extra, const Value *x, const Value *y, const Value *z);
static const Value *hcomp_pathp_at(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base, const Value *j);
static const Value *transp_pathp_at(Machine *m, const Value *line, const Value *phi, const Value *base, const Value *r);
static const Value *glue_at(Machine *m, const Value *line, const Value *r);
static const Value *glue_branch_at(Machine *m, const Value *line, const Value *keyf, uint32_t level, const Value *s);
static const Value *transp_glue(Machine *m, const Value *line, const Value *psi, const Value *u0);
static const Value *unglue_value(Machine *m, const Value *ty, const Value *g);
static const Value *glue_restrict(Machine *m, const Value *x, const Value *key);
static const Value *hcomp_glue(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base);
static const Value *hcomp_univ(Machine *m, const Value *phi, const Value *sys, const Value *base);
static const Value *domain_def(Machine *m, const char *name);

/* The part FIELD (0: witness, 1: payload) of the Sigma value U. */
static const Value *sigma_part(Machine *m, const Value *u, uint32_t field) {
  const Value *one[1];
  one[0] = u;
  return u == NULL ? NULL : reduce_part(m, field == 0u ? OP_WITNESS : OP_PAYLOAD, one, 1, OP_PACK, field);
}

static const Value *transp_stuck(Machine *m, const Value *line, const Value *phi, const Value *base) {
  return make(m, VAL_STUCK, OP_TRANSP, 0, 3, line, phi, base);
}

static const Value *not_typed(Machine *m) {
  return internal(m, "a transport of a value of the wrong type");
}

/* transp along the type argument FIELD of the data line L. */
static const Value *transp_part(Machine *m, const Value *line, uint32_t field, const Value *phi, const Value *x) {
  return transp(m, kan_closure(m, VAL_PLAM, CLOS_LINE_PART, field, 1, line, NULL, NULL), phi, x);
}

/* x'(r) = transp (fun j => A (r \/ ~j)) (phi \/ r) x: X in A(i1), moved
   back to A(r). */
static const Value *pi_back(Machine *m, const Value *line, const Value *phi, const Value *x, const Value *r) {
  return transp(m, kan_closure(m, VAL_PLAM, CLOS_PI_DOM, 0, 2, line, r, NULL), ival_join(m, phi, r), x);
}

/* a'(r) = transp (fun j => A (r /\ j)) (phi \/ ~r) u: U in A(i0), moved
   up to A(r). */
static const Value *sigma_up(Machine *m, const Value *line, const Value *phi, const Value *u, const Value *r) {
  return transp(m, kan_closure(m, VAL_PLAM, CLOS_SIGMA_DOM, 0, 2, line, r, NULL), ival_join(m, phi, ival_neg(m, r)), u);
}

/* x(r) = transp (fun j => A (r /\ j)) (~r) x: X in the domain A(i0) of a
   line of Pi types, moved up to A(r) (S6b rule 3). */
static const Value *pi_up(Machine *m, const Value *line, const Value *x, const Value *r) {
  return transp(m, kan_closure(m, VAL_PLAM, CLOS_PI_DOM_UP, 0, 2, line, r, NULL), ival_neg(m, r), x);
}

/* The type of the point field FIELD of the constructor INST at the
   parameters of the family value T and the values EARLIER of the fields
   before it (S4 items 11 and 13). The field type is over the parameters
   and the earlier fields: the field before it is index 0. */
static const Value *field_type_at(Machine *m, uint32_t inst, uint32_t field, const Value *t, const Value *const *earlier) {
  const Env *env = NULL;
  uint32_t i;
  if (t == NULL)
    return NULL;
  if (!val_is(t, OP_FAMILY) || inst >= m->ctor_count || t->inst != m->ctors[inst].family || field >= m->ctors[inst].field_count)
    return internal(m, "a line that changes its type former");
  for (i = 0; i < t->argc + field; i++) {
    env = env_push(m, env, i < t->argc ? t->args[i] : earlier[i - t->argc]);
    if (env == NULL)
      return NULL;
  }
  return eval_core(m, env, m->ctors[inst].fields[field].type);
}

/* The line of the point field FIELD of the constructor value V under
   transp L phi: L itself for a recursive field. */
static const Value *field_line(Machine *m, const Value *line, const Value *phi, const Value *v, uint32_t field) {
  return m->ctors[v->inst].fields[field].recursive ? line : kan_closure(m, VAL_PLAM, CLOS_FAMILY_FIELD, field, 3, line, v, phi);
}

/* The type at R of the point field FIELD of the constructor value V under
   transp L phi (S4 item 13): the field type at the parameters of L(r) and
   at the fills at R of the earlier fields. The fill of field k is
   transp (fun j => line_k (r /\ j)) (phi \/ ~r) (field k of V), as
   transp_sigma moves the first part. A field type that uses no earlier
   field gets the fields of V, which it does not read. */
static const Value *family_field_at(Machine *m, const Value *line, const Value *phi, const Value *v, uint32_t field, const Value *r) {
  const Value *t = line_former(m, line_at(m, line, r), VAL_OP);
  const Value **fills;
  uint32_t p;
  uint32_t k;
  if (t == NULL)
    return NULL;
  if (!val_is(t, OP_FAMILY) || v->inst >= m->ctor_count || field >= m->ctors[v->inst].field_count || v->argc < t->argc + field)
    return internal(m, "a line that changes its type former");
  p = t->argc;
  if (!m->ctors[v->inst].fields[field].dependent)
    return field_type_at(m, v->inst, field, t, v->args + p);
  fills = arena_alloc(m->arena, ((size_t)field + 1u) * sizeof *fills);
  if (fills == NULL)
    return oom(m);
  for (k = 0; k < field; k++) {
    fills[k] = transp(m, kan_closure(m, VAL_PLAM, CLOS_LINE_MEET, 0, 2, field_line(m, line, phi, v, k), r, NULL), ival_join(m, phi, ival_neg(m, r)), v->args[p + k]);
    if (fills[k] == NULL)
      return NULL;
  }
  return field_type_at(m, v->inst, field, t, fills);
}

static const Value *kan_apply(Machine *m, const Value *k, const Value *r) {
  const Value *const *a = k->args;
  const Value *t;
  switch (k->tag) {
  case CLOS_LINE_PART:
    t = line_former(m, line_at(m, a[0], r), VAL_OP);
    if (t != NULL && k->field >= t->argc)
      return internal(m, "a line that changes its type former");
    return t == NULL ? NULL : t->args[k->field];
  case CLOS_PI_DOM:
    t = line_former(m, line_at(m, a[0], ival_join(m, a[1], ival_neg(m, r))), VAL_PI);
    return t == NULL ? NULL : t->dom;
  case CLOS_SIGMA_DOM:
    t = line_former(m, line_at(m, a[0], ival_meet(m, a[1], r)), VAL_SIGMA);
    return t == NULL ? NULL : t->dom;
  case CLOS_PI_COD:
    return closure_apply(m, line_former(m, line_at(m, a[0], r), VAL_PI), pi_back(m, a[0], a[1], a[2], r));
  case CLOS_SIGMA_COD:
    return closure_apply(m, line_former(m, line_at(m, a[0], r), VAL_SIGMA), sigma_up(m, a[0], a[1], a[2], r));
  case CLOS_TRANSP_PI:
    return transp(m, kan_closure(m, VAL_PLAM, CLOS_PI_COD, 0, 3, a[0], a[1], r), a[1], apply_value(m, a[2], pi_back(m, a[0], a[1], r, val_iend(m, 0))));
  case CLOS_CONST:
    return a[0];
  case CLOS_OUT_PI:
    t = apply_value(m, a[1], r);
    if (t == NULL)
      return NULL;
    return k->field > 1u ? kan_closure(m, VAL_LAM, CLOS_OUT_PI, k->field - 1u, 2, a[0], t, NULL) : counfold_at(m, a[0], t);
  case CLOS_SYS_APP:
    return apply_value(m, closure_apply(m, a[0], r), a[1]);
  case CLOS_SYS_PAPP:
    return path_apply(m, closure_apply(m, a[0], r), a[1], a[2], a[3]);
  case CLOS_SYS_SIGMA:
    return sigma_part(m, closure_apply(m, a[0], r), k->field);
  case CLOS_SYS_ARG:
    t = closure_apply(m, a[0], r);
    if (t != NULL && (t->kind != VAL_OP || k->field >= t->argc))
      return internal(m, "a system branch that changes its constructor");
    return t == NULL ? NULL : t->args[k->field];
  case CLOS_SYS_MEET:
    return closure_apply(m, a[0], ival_meet(m, a[1], r));
  case CLOS_COMP_BRANCH:
    return transp(m, kan_closure(m, VAL_PLAM, CLOS_LINE_JOIN, 0, 2, a[1], r, NULL), r, closure_apply(m, a[0], r));
  case CLOS_LINE_JOIN:
    return line_at(m, a[0], ival_join(m, a[1], r));
  case CLOS_HCOMP_PI:
    t = closure_apply(m, a[0], r);
    if (t == NULL)
      return NULL;
    if (t->kind == VAL_UNIV)
      return apply_value(m, hcomp_stuck(m, a[0], a[1], a[2], a[3]), r);
    return hcomp(m, t, a[1], sys_map(m, a[2], CLOS_SYS_APP, 0, 1, r, NULL, NULL), apply_value(m, a[3], r));
  case CLOS_HCOMP_PATHP:
    return hcomp_pathp_at(m, a[0], a[1], a[2], a[3], r);
  case CLOS_SIGMA_FILL_COD:
    return closure_apply(m, a[0], hfill(m, a[0]->dom, a[1], a[2], a[3], r));
  case CLOS_TRANSP_PATHP:
    return transp_pathp_at(m, a[0], a[1], a[2], r);
  case CLOS_PATHP_LINE:
    return closure_apply(m, line_former(m, line_at(m, a[0], r), VAL_PATHP), a[1]);
  case CLOS_PATHP_END:
    t = line_former(m, line_at(m, a[0], r), VAL_PATHP);
    return t == NULL ? NULL : t->args[k->field];
  case CLOS_GLUE_BASE:
    t = glue_at(m, a[0], r);
    return t == NULL ? NULL : t->args[0];
  case CLOS_GLUE_T:
    return sigma_part(m, glue_branch_at(m, a[0], a[1], k->field, ival_meet(m, a[2], r)), 0);
  case CLOS_GLUE_FILL:
    t = sigma_part(m, sigma_part(m, glue_branch_at(m, a[0], a[1], k->field, r), 1), 0);
    return apply_value(m, t, transp(m, kan_closure(m, VAL_PLAM, CLOS_GLUE_T, k->field, 3, a[0], a[1], r), ival_join(m, a[2], ival_neg(m, r)), a[3]));
  case CLOS_CONTR_AT:
    return path_apply(m, apply_value(m, a[0], a[1]), r, a[2], a[1]);
  case CLOS_PATH_REV:
    return path_apply(m, a[0], ival_neg(m, r), a[1], a[2]);
  case CLOS_SYS_UNGLUE:
    return unglue_value(m, a[1], closure_apply(m, a[0], r));
  case CLOS_GLUE_HFILL:
    return apply_value(m, sigma_part(m, sigma_part(m, a[0], 1), 0), hfill(m, sigma_part(m, a[0], 0), a[1], a[2], a[3], r));
  case CLOS_SYS_RESTRICT:
    return glue_restrict(m, closure_apply(m, a[0], r), a[1]);
  case CLOS_EQUIV_LINE:
    return apply_value(m, apply_value(m, domain_def(m, "Equiv"), a[1]), closure_apply(m, a[0], ival_neg(m, r)));
  case CLOS_SYS_FOLD:
    if (a[1]->argc < 2u)
      return internal(m, "a fold branch without its cases");
    return fold_family(m, k->field, a[1]->args, a[1]->argc - 2u, closure_apply(m, a[0], r));
  case CLOS_FAMILY_FIELD:
    return family_field_at(m, a[0], a[2], a[1], k->field, r);
  case CLOS_SYS_TRANSP:
    return transp(m, a[1], a[2], closure_apply(m, a[0], r));
  case CLOS_LINE_MEET:
    return line_at(m, a[0], ival_meet(m, a[1], r));
  case CLOS_FOLD_PATH:
    if (a[3]->argc < 2u)
      return internal(m, "a folded path field without its cases");
    /* A stuck elim (S7 B1) holds the motive, the cases and x. */
    if (a[3]->op == OP_ELIM)
      return elim_at(m, k->field, a[3]->args, a[3]->argc, path_apply(m, a[0], r, a[1], a[2]));
    return fold_family(m, k->field, a[3]->args, a[3]->argc - 2u, path_apply(m, a[0], r, a[1], a[2]));
  case CLOS_FOLD_FACE:
    if (a[2]->argc < 2u)
      return internal(m, "a folded face without its cases");
    if (a[2]->op == OP_ELIM)
      return elim_at(m, k->field, a[2]->args, a[2]->argc, ctor_at2(m, a[0], a[1], r));
    return fold_family(m, k->field, a[2]->args, a[2]->argc - 2u, ctor_at2(m, a[0], a[1], r));
  case CLOS_FOLD_CONT:
    if (a[1]->argc < 2u)
      return internal(m, "a folded continuation field without its cases");
    if (a[1]->op == OP_ELIM)
      return elim_at(m, k->field, a[1]->args, a[1]->argc, apply_value(m, a[0], r));
    return fold_family(m, k->field, a[1]->args, a[1]->argc - 2u, apply_value(m, a[0], r));
  case CLOS_PATH_SIGMA_COD:
    return closure_apply(m, line_former(m, line_at(m, a[0], r), VAL_SIGMA), sigma_part(m, line_at(m, a[1], r), 0));
  case CLOS_PI_DOM_UP:
    t = line_former(m, line_at(m, a[0], ival_meet(m, a[1], r)), VAL_PI);
    return t == NULL ? NULL : t->dom;
  case CLOS_PATH_PI_COD:
    return closure_apply(m, line_former(m, line_at(m, a[0], r), VAL_PI), pi_up(m, a[0], a[1], r));
  case CLOS_PATH_PI_POINT:
    return apply_value(m, line_at(m, a[1], r), pi_up(m, a[0], a[2], r));
  case CLOS_CORE:
    break;
  }
  return internal(m, "a closure with an unknown tag");
}

static const Value *transp_list(Machine *m, const Value *line, const Value *phi, const Value *base) {
  Spine sp;
  const Value **items;
  const Value *tail;
  uint32_t i;
  if (!spine_of(m, base, &sp))
    return NULL;
  items = arena_alloc(m->arena, ((size_t)sp.count + 1u) * sizeof *items);
  if (items == NULL)
    return oom(m);
  for (i = 0; i < sp.count; i++) {
    items[i] = transp_part(m, line, 0, phi, sp.cells[i]->args[0]);
    if (items[i] == NULL)
      return NULL;
  }
  tail = scrut_of(sp.tail) == SCRUT_NEUTRAL ? transp_stuck(m, line, phi, sp.tail) : sp.tail;
  return build_list(m, items, sp.count, tail);
}

/* A boundary face may use the constructor's interval fields, but not a
   family parameter that can change along transport. */
static int ctor_interval_face(const Core *t, uint32_t dimensions) {
  if (t->kind == CORE_VAR)
    return t->index < dimensions;
  if (t->kind == CORE_IZERO || t->kind == CORE_IONE)
    return 1;
  if (t->kind == CORE_INEG)
    return ctor_interval_face(t->a, dimensions);
  if (t->kind == CORE_IMEET || t->kind == CORE_IJOIN)
    return ctor_interval_face(t->a, dimensions) && ctor_interval_face(t->b, dimensions);
  return 0;
}

/* Rebuilding a path constructor commutes with its boundary when each
   branch is a recursive field: both sides transport that field along L.
   General boundary terms need a boundary correction. Until that rule is
   implemented, retain the transport rather than change its endpoints. */
static int field_transport_boundary(const CtorInfo *ctor) {
  const Core *sys = ctor->boundary;
  uint32_t k;
  if (sys == NULL)
    return 1;
  if (sys->kind != CORE_OP || sys->op != OP_SYSTEM)
    return 0;
  for (k = 0; k + 1u < sys->argc; k += 2u) {
    const Core *branch = sys->args[k + 1u];
    uint32_t index;
    if (!ctor_interval_face(sys->args[k], ctor->interval_count)
        || branch->kind != CORE_PLAM || branch->b->kind != CORE_VAR
        || branch->b->index <= ctor->interval_count)
      return 0;
    /* A branch is under one extra, unused path binder. */
    index = branch->b->index - ctor->interval_count - 1u;
    if (index >= ctor->field_count || !ctor->fields[ctor->field_count - 1u - index].recursive)
      return 0;
  }
  return 1;
}

/* S4 item 11 (R7 (a)): L(i) = F (p i), a family with parameters. A
   constructor goes into its point fields, each along its field type at the
   parameters of L (a recursive field along L itself); the interval fields
   do not change, and the parameters are those of L(i1). A formal hcomp
   goes into its tubes and its base (CCHM):
   transp L phi (hcomp F(p i0) psi u u0)
     = hcomp F(p i1) psi (fun j => [ k -> transp L phi (u j) ]) (transp L phi u0). */
static const Value *transp_family(Machine *m, const Value *line, const Value *phi, const Value *base) {
  const Value *end = line_former(m, line_at(m, line, val_iend(m, 1)), VAL_OP);
  const CtorInfo *ctor;
  const Value **args;
  const Value *sys;
  uint32_t q;
  uint32_t i;
  if (end == NULL)
    return NULL;
  if (val_is(base, OP_HCOMP) && base->argc == 4u) {
    sys = sys_map(m, base->args[2], CLOS_SYS_TRANSP, 0, 2, line, phi, NULL);
    return sys == NULL ? NULL : hcomp(m, end, base->args[1], sys, transp(m, line, phi, base->args[3]));
  }
  if (!val_is(base, OP_CTOR) || base->inst >= m->ctor_count || !val_is(end, OP_FAMILY))
    return not_typed(m);
  ctor = &m->ctors[base->inst];
  q = end->argc + ctor->field_count;
  if (end->inst != ctor->family || base->argc != q + ctor->interval_count)
    return not_typed(m);
  if (!field_transport_boundary(ctor))
    return transp_stuck(m, line, phi, base);
  args = arena_alloc(m->arena, ((size_t)base->argc + 1u) * sizeof *args);
  if (args == NULL)
    return oom(m);
  for (i = 0; i < end->argc; i++)
    args[i] = end->args[i];
  for (i = end->argc; i < q; i++) {
    args[i] = transp(m, field_line(m, line, phi, base, i - end->argc), phi, base->args[i]);
    if (args[i] == NULL)
      return NULL;
  }
  for (i = q; i < base->argc; i++)
    args[i] = base->args[i];
  return val_ctor(m, base->inst, args, base->argc);
}

/* Rules 3, 4 and 8 (R4 (a)): L(i) is the data type FORMER at a fresh i. A
   closed type gives the base; a constructor base goes into its arguments;
   a neutral base and a domain family are stuck. */
/* The line L is the type HEAD (L at a fresh variable) at each point: HEAD
   converts with L(i0). The levels of the test are above the fresh levels. */
static int line_constant(Machine *m, const Value *line, const Value *head) {
  const Value *end = line_at(m, line, val_iend(m, 0));
  return end != NULL && conv_values(m, KAN_FRESH_BASE + KAN_FRESH_MASK + 1u, head, end);
}

static const Value *transp_data(Machine *m, const Value *head, const Value *line, const Value *phi, const Value *base) {
  int neutral = scrut_of(base) == SCRUT_NEUTRAL;
  switch (head->op) {
  case OP_TEXT:
  case OP_NAT:
  case OP_FIN:
  case OP_EMPTY:
  case OP_UNIT:
    return base;
  case OP_PROD:
    if (neutral)
      break;
    if (!val_is(base, OP_PAIR))
      return not_typed(m);
    return make(m, VAL_OP, OP_PAIR, 0, 2, transp_part(m, line, 0, phi, base->args[0]), transp_part(m, line, 1, phi, base->args[1]), NULL);
  case OP_SUM:
    if (neutral)
      break;
    if (!val_is(base, OP_INL) && !val_is(base, OP_INR))
      return not_typed(m);
    return make(m, VAL_OP, base->op, 0, 1, transp_part(m, line, val_is(base, OP_INL) ? 0u : 1u, phi, base->args[0]), NULL, NULL);
  case OP_OPTION:
    if (neutral)
      break;
    if (val_is(base, OP_NONE))
      return base;
    if (!val_is(base, OP_SOME))
      return not_typed(m);
    return make(m, VAL_OP, OP_SOME, 0, 1, transp_part(m, line, 0, phi, base->args[0]), NULL, NULL);
  case OP_LIST:
    if (neutral)
      break;
    return transp_list(m, line, phi, base);
  case OP_FAMILY:
    /* S4 chunk B: at a cofamily, the regularity rule on a constant line.
       On a line that changes, the transport stays neutral. */
    if (head->inst < m->family_count && m->families[head->inst].cofamily) {
      if (line_constant(m, line, head))
        return base;
      break;
    }
    /* Indexed fibres and sibling sorts need their own field lines. Keep
       the transport neutral until those reductions are implemented. */
    if (head->inst < m->family_count && (m->families[head->inst].index_count > 0u || m->families[head->inst].block_count > 1u))
      break;
    /* S4 item 6 (R7 (a)): a family with no parameters is the same type at
       each point of the line by its form (not by regularity), so the base
       stays, also a neutral base or a formal hcomp. */
    if (head->inst < m->family_count && m->families[head->inst].param_count == 0u)
      return base;
    if (neutral)
      break;
    return transp_family(m, line, phi, base);
  case OP_GLUE:
    return transp_glue(m, line, phi, base);
  case OP_INTERVAL:
  case OP_UNIT_VAL:
  case OP_COUNFOLD:
  case OP_OUT:
  case OP_PAIR:
  case OP_INL:
  case OP_INR:
  case OP_NONE:
  case OP_SOME:
  case OP_NIL:
  case OP_CONS:
  case OP_PACK:
  case OP_CTOR:
  case OP_FIRST:
  case OP_SECOND:
  case OP_EITHER:
  case OP_OPTION_ELIM:
  case OP_PURE:
  case OP_MAP:
  case OP_BIND:
  case OP_FOLD_NAT:
  case OP_FOLD_LIST:
  case OP_FOLD_FAMILY:
  case OP_ELIM:
  case OP_FILTER:
  case OP_WITNESS:
  case OP_PAYLOAD:
  case OP_TEXT_EQ:
  case OP_CREDITS_CHECK:
  case OP_REF_UNKNOWN:
  case OP_NAT_ADD:
  case OP_NAT_SUB:
  case OP_NAT_MUL:
  case OP_SUCC:
  case OP_NAT_EQ:
  case OP_NAT_LE:
  case OP_ABSURD:
  case OP_PROJ:
  case OP_TRANSP:
  case OP_SYSTEM:
  case OP_HCOMP:
  case OP_FIN_ADD:
  case OP_FIN_NEG:
  case OP_FIN_MUL:
  case OP_FIN_VAL:
  case OP_FIN_EQ:
  case OP_FIN_CASE:
  case OP_DEC_EQ_FIN:
  case OP_DEC_EQ_NAT:
  case OP_GLUE_ELEM:
  case OP_UNGLUE:
    return internal(m, "a transport along a line that is not a type");
  }
  return transp_stuck(m, line, phi, base);
}

/* Rule 6: L(i) = Sigma (x : A i) (B i x). The base goes through its
   projections, so a neutral base moves too. */
static const Value *transp_sigma(Machine *m, const Value *line, const Value *phi, const Value *base) {
  const Value *one[1];
  const Value *u;
  const Value *v;
  one[0] = base;
  u = reduce_part(m, OP_WITNESS, one, 1, OP_PACK, 0);
  v = reduce_part(m, OP_PAYLOAD, one, 1, OP_PACK, 1);
  return make(m, VAL_OP, OP_PACK, 0, 2, sigma_up(m, line, phi, u, val_iend(m, 1)), transp(m, kan_closure(m, VAL_PLAM, CLOS_SIGMA_COD, 0, 3, line, phi, u), phi, v), NULL);
}

/* transp L phi a, the rules of the S2 kickoff section 1.2 in order. */
static const Value *transp(Machine *m, const Value *line, const Value *phi, const Value *base) {
  const Value *face;
  const Value *head;
  if (line == NULL || phi == NULL || base == NULL || !spend(m, 1))
    return NULL;
  face = face_norm(m, phi);
  if (face == NULL)
    return NULL;
  /* Rule 1: phi is true. */
  if (ival_is_end(face, 1) || base->kind == VAL_TRAP)
    return base;
  head = line_head(m, line);
  if (head == NULL)
    return NULL;
  switch (head->kind) {
  case VAL_UNIV: /* rule 2 */
    return base;
  case VAL_OP: /* rules 3, 4, 8 */
    return transp_data(m, head, line, face, base);
  case VAL_PI: /* rule 5 */
    return kan_closure(m, VAL_LAM, CLOS_TRANSP_PI, 0, 3, line, face, base);
  case VAL_SIGMA: /* rule 6 */
    return transp_sigma(m, line, face, base);
  case VAL_TRAP:
    return head;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_LAM:
  case VAL_PLAM:
  case VAL_IVAL:
    return internal(m, "a transport along a line that is not a type");
  case VAL_PATHP: /* rule 7 */
    return kan_closure(m, VAL_PLAM, CLOS_TRANSP_PATHP, 0, 3, line, face, base);
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PAPP:
    break;
  }
  /* Rule 9: a neutral line. */
  return transp_stuck(m, line, face, base);
}

int path_sigma_part(Machine *m, const Value *line, const Value *p, const Value *a, const Value *b, uint32_t field, const Value **out) {
  out[0] = field == 0u ? kan_closure(m, VAL_PLAM, CLOS_SIGMA_DOM, 0, 2, line, val_iend(m, 1), NULL)
    : kan_closure(m, VAL_PLAM, CLOS_PATH_SIGMA_COD, 0, 2, line, p, NULL);
  out[1] = kan_closure(m, VAL_PLAM, CLOS_SYS_SIGMA, field, 1, p, NULL, NULL);
  out[2] = sigma_part(m, a, field);
  out[3] = sigma_part(m, b, field);
  return out[0] != NULL && out[1] != NULL && out[2] != NULL && out[3] != NULL && !m->diag->set;
}

/* The pointwise path at X of a path P of functions (a PLAM) over the
   line LINE of Pi types from A to B: X is a point of the domain at i0.
   OUT: the line j |-> B(j) (x(j)), the path j |-> (p @ j) (x(j)), and its
   ends A x and B (x(i1)), with x(j) = pi_up (S6b rule 3). */
int path_pi_point(Machine *m, const Value *line, const Value *p, const Value *a, const Value *b, const Value *x, const Value **out) {
  out[0] = kan_closure(m, VAL_PLAM, CLOS_PATH_PI_COD, 0, 2, line, x, NULL);
  out[1] = kan_closure(m, VAL_PLAM, CLOS_PATH_PI_POINT, 0, 3, line, p, x);
  out[2] = apply_value(m, a, x);
  out[3] = apply_value(m, b, pi_up(m, line, x, val_iend(m, 1)));
  return out[0] != NULL && out[1] != NULL && out[2] != NULL && out[3] != NULL && !m->diag->set;
}

const Value *val_transp(Machine *m, const Value *line, const Value *phi, const Value *base) {
  return transp(m, line, phi, base);
}

/* hcomp, comp and hfill (S2b, S2 kickoff section 1.3). A system is a
   VAL_OP OP_SYSTEM with the arguments key0, branch0, key1, branch1, ...;
   each branch is a PLAM over the tube variable. */

/* Normalize face keys and discard unreachable branches before applying
   constructor projections or comparing generated systems. */
static const Value *system_value(Machine *m, const Value *const *args, uint32_t argc) {
  const Value **live;
  const Value *key;
  uint32_t k;
  uint32_t n = 0;
  live = arena_alloc(m->arena, ((size_t)argc + 1u) * sizeof *live);
  if (live == NULL)
    return oom(m);
  for (k = 0; k + 1u < argc; k += 2u) {
    key = face_norm(m, args[k]);
    if (key == NULL)
      return NULL;
    if (!ival_is_end(key, 0)) {
      live[n++] = key;
      live[n++] = args[k + 1u];
    }
  }
  return val_op(m, VAL_OP, OP_SYSTEM, 0, 0, live, n);
}

/* SYS with each branch b replaced by the closure TAG of b and the EXTRA
   values X, Y, Z. The keys do not change. */
static const Value *sys_map(Machine *m, const Value *sys, ClosTag tag, uint32_t field, uint32_t extra, const Value *x, const Value *y, const Value *z) {
  const Value **args;
  uint32_t k;
  if (sys == NULL)
    return NULL;
  args = arena_alloc(m->arena, ((size_t)sys->argc + 1u) * sizeof *args);
  if (args == NULL)
    return oom(m);
  for (k = 0; k + 1u < sys->argc; k += 2u) {
    args[k] = sys->args[k];
    args[k + 1u] = kan_closure4(m, VAL_PLAM, tag, field, extra + 1u, sys->args[k + 1u], x, y, z);
  }
  return system_value(m, args, sys->argc);
}

/* SYS with one more branch KEY -> BRANCH. */
static const Value *sys_add(Machine *m, const Value *sys, const Value *key, const Value *branch) {
  const Value **args;
  uint32_t k;
  if (sys == NULL)
    return NULL;
  args = arena_alloc(m->arena, ((size_t)sys->argc + 2u) * sizeof *args);
  if (args == NULL)
    return oom(m);
  for (k = 0; k < sys->argc; k++)
    args[k] = sys->args[k];
  args[sys->argc] = key;
  args[sys->argc + 1u] = branch;
  return system_value(m, args, sys->argc + 2u);
}

static const Value *constant_line(Machine *m, const Value *v) {
  return kan_closure(m, VAL_PLAM, CLOS_CONST, 0, 1, v, NULL, NULL);
}

static const Value *hcomp_stuck(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value *args[4];
  args[0] = ty;
  args[1] = phi;
  args[2] = sys;
  args[3] = base;
  return val_op(m, VAL_STUCK, OP_HCOMP, 0, 0, args, 4);
}

/* 1 when TY is a domain family with a path constructor (a HIT). */
static int hit_type(const Machine *m, const Value *ty) {
  const FamilyInfo *f;
  uint32_t k;
  if (!val_is(ty, OP_FAMILY) || ty->inst >= m->family_count)
    return 0;
  f = &m->families[ty->inst];
  for (k = 0; k < f->ctor_count; k++)
    if (f->first_ctor + k < m->ctor_count && m->ctors[f->first_ctor + k].interval_count > 0u)
      return 1;
  return 0;
}

/* hcomp at a HIT with no true key (S4 R3 (a)): a canonical value, never
   VAL_STUCK (conv_loop rejects VAL_OP against VAL_STUCK). There is no
   regularity, so this holds also at i0 and with a neutral base. */
static const Value *hcomp_formal(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value *args[4];
  args[0] = ty;
  args[1] = phi;
  args[2] = sys;
  args[3] = base;
  return val_op(m, VAL_OP, OP_HCOMP, 0, 0, args, 4);
}

/* fun (k : I) => fold (p @ k) for the path field FIELD of the constructor
   value X (S4 item 14). The ends of p are the fields ENDS of X. The
   closure reads the cases from a stuck fold of X. */
static const Value *fold_path_field(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x, uint32_t field) {
  const FieldInfo *f = &m->ctors[x->inst].fields[field];
  uint32_t p = m->families[family].param_count;
  return kan_closure4(m, VAL_PLAM, CLOS_FOLD_PATH, field_sort(f, family), 4u, x->args[p + field], x->args[p + f->ends[0]], x->args[p + f->ends[1]], stuck_fold(m, family, cases, ncases, x));
}

/* fold (hcomp F phi u u0) = hcomp RES phi (fun i => [ k -> fold (u i) ])
   (fold u0). CASES holds NCASES cases, then RES. The branches read the
   cases from a stuck fold of X (CLOS_SYS_FOLD). */
static const Value *fold_hcomp(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x) {
  const Value *pack = stuck_fold(m, family, cases, ncases, x);
  const Value *sys = pack == NULL ? NULL : sys_map(m, x->args[2], CLOS_SYS_FOLD, family, 1, pack, NULL, NULL);
  const Value *base = sys == NULL ? NULL : fold_family(m, family, cases, ncases, x->args[3]);
  return base == NULL ? NULL : hcomp(m, fold_res(m, family, cases, ncases), x->args[1], sys, base);
}

/* The index of the branch of SYS under the first true (i1) key, or
   sys->argc when no key is true. hcomp and Glue use this search. */
static uint32_t true_branch(Machine *m, const Value *sys) {
  uint32_t k;
  for (k = 0; k + 1u < sys->argc; k += 2u)
    if (ival_is_end(face_norm(m, sys->args[k]), 1))
      return k + 1u;
  return sys->argc;
}

/* phi is true, so the join of the keys is true and one key is i1: the
   result is its branch at i1. */
static const Value *hcomp_true(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  uint32_t k = true_branch(m, sys);
  if (k < sys->argc)
    return closure_apply(m, sys->args[k], val_iend(m, 1));
  return hit_type(m, ty) ? hcomp_formal(m, ty, phi, sys, base) : hcomp_stuck(m, ty, phi, sys, base);
}

/* The type of the argument FIELD of the constructor value V in the data
   type TY, or NULL. */
static const Value *data_field_type(const Value *ty, const Value *v, uint32_t field) {
  if (val_is(v, OP_PAIR) && val_is(ty, OP_PROD) && field < 2u)
    return ty->args[field];
  if ((val_is(v, OP_INL) && val_is(ty, OP_SUM)) || (val_is(v, OP_SOME) && val_is(ty, OP_OPTION)) || (val_is(v, OP_CONS) && val_is(ty, OP_LIST) && field == 0u))
    return ty->args[0];
  if (val_is(v, OP_INR) && val_is(ty, OP_SUM))
    return ty->args[1];
  return val_is(v, OP_CONS) && val_is(ty, OP_LIST) ? ty : NULL;
}

/* The type of the argument K of the constructor value V in the uniform
   family TY (S2 K7), or NULL. A point field gets its type at the
   parameters of TY and the fields of V. A field type that uses an earlier
   field needs comp, not hcomp, so it gives NULL (stuck). */
static const Value *family_field_type(Machine *m, const Value *ty, const Value *v, uint32_t k) {
  const FamilyInfo *f = &m->families[ty->inst];
  const CtorInfo *ctor = v->inst < m->ctor_count ? &m->ctors[v->inst] : NULL;
  uint32_t p = f->param_count;
  if (ctor == NULL || !val_is(v, OP_CTOR) || ctor->family != ty->inst || f->index_count != 0u || ty->argc != p ||
      k < p || k - p >= ctor->field_count || ctor->fields[k - p].dependent)
    return NULL;
  return field_type_at(m, v->inst, k - p, ty, v->args + p);
}

/* 1 when the branch B at a fresh i has the head of V: the same literal or
   the same constructor (the K10 head test). */
static int same_head_at(Machine *m, const Value *b, const Value *v) {
  const Value *x = closure_apply(m, b, fresh_ivar(m));
  if (x == NULL)
    return 0;
  if (v->kind == VAL_NAT || v->kind == VAL_BIG || v->kind == VAL_TEXT)
    return x->kind == v->kind && conv_values(m, 0, x, v);
  return x->kind == VAL_OP && x->op == v->op && x->inst == v->inst && x->argc == v->argc;
}

/* A data type (R4 (a)): each live branch at a fresh i has the head of u0.
   A literal or a nullary constructor gives u0; a constructor with
   arguments goes into them. A neutral u0 and a head mismatch are stuck.
   A uniform domain family that is not a HIT goes into the point fields of
   a constructor value, and a parameter keeps its value (S2 K7). */
static const Value *hcomp_data(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value **args;
  const Value *field;
  uint32_t k;
  if (hit_type(m, ty))
    return hcomp_formal(m, ty, phi, sys, base);
  if (scrut_of(base) == SCRUT_NEUTRAL ||
      (base->kind != VAL_NAT && base->kind != VAL_BIG && base->kind != VAL_TEXT && base->kind != VAL_OP))
    return hcomp_stuck(m, ty, phi, sys, base);
  for (k = 0; k + 1u < sys->argc; k += 2u)
    if (!ival_is_end(face_norm(m, sys->args[k]), 0) && !same_head_at(m, sys->args[k + 1u], base))
      return hcomp_stuck(m, ty, phi, sys, base);
  if (base->argc == 0u)
    return base;
  args = arena_alloc(m->arena, ((size_t)base->argc + 1u) * sizeof *args);
  if (args == NULL)
    return oom(m);
  for (k = 0; k < base->argc; k++) {
    if (val_is(ty, OP_FAMILY) && k < m->families[ty->inst].param_count) {
      args[k] = ty->args[k];
    } else {
      field = val_is(ty, OP_FAMILY) ? family_field_type(m, ty, base, k) : data_field_type(ty, base, k);
      if (field == NULL || field->kind == VAL_UNIV)
        return hcomp_stuck(m, ty, phi, sys, base);
      args[k] = hcomp(m, field, phi, sys_map(m, sys, CLOS_SYS_ARG, k, 0, NULL, NULL, NULL), base->args[k]);
      if (args[k] == NULL)
        return NULL;
    }
  }
  return val_op(m, VAL_OP, base->op, base->inst, base->field, args, base->argc);
}

/* Sigma: the first part is an hcomp in A; the second part is a comp along
   i |-> B (the hfill of the first part at i). */
static const Value *hcomp_sigma(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value *w;
  const Value *cod;
  const Value *u = sigma_part(m, base, 0);
  const Value *v = sigma_part(m, base, 1);
  const Value *first = sys_map(m, sys, CLOS_SYS_SIGMA, 0, 0, NULL, NULL, NULL);
  const Value *second = sys_map(m, sys, CLOS_SYS_SIGMA, 1, 0, NULL, NULL, NULL);
  if (ty->dom->kind == VAL_UNIV)
    return hcomp_stuck(m, ty, phi, sys, base);
  w = hcomp(m, ty->dom, phi, first, u);
  cod = w == NULL ? NULL : closure_apply(m, ty, w);
  if (cod == NULL)
    return NULL;
  if (cod->kind == VAL_UNIV)
    return hcomp_stuck(m, ty, phi, sys, base);
  return make(m, VAL_OP, OP_PACK, 0, 2, w, comp(m, kan_closure4(m, VAL_PLAM, CLOS_SIGMA_FILL_COD, 0, 4, ty, phi, first, u), phi, second, v), NULL);
}

/* PathP at j: hcomp (A j) (phi \/ ~j \/ j) [sys @ j, ~j -> a, j -> b]
   (u0 @ j), with a and b the ends of the path type. */
static const Value *hcomp_pathp_at(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base, const Value *j) {
  const Value *a = ty->args[0];
  const Value *b = ty->args[1];
  const Value *at = closure_apply(m, ty, j);
  const Value *nj = ival_neg(m, j);
  const Value *s;
  if (at == NULL)
    return NULL;
  if (at->kind == VAL_UNIV)
    return path_apply(m, hcomp_stuck(m, ty, phi, sys, base), j, a, b);
  s = sys_add(m, sys_add(m, sys_map(m, sys, CLOS_SYS_PAPP, 0, 3, j, a, b), nj, constant_line(m, a)), j, constant_line(m, b));
  return hcomp(m, at, ival_join(m, phi, ival_join(m, nj, j)), s, path_apply(m, base, j, a, b));
}

/* Rule 7 of transp at r: comp (fun i => L(i) at r) (phi \/ ~r \/ r)
   [phi -> p0 @ r, ~r -> the i0 end of L(i), r -> the i1 end of L(i)]
   (p0 @ r). */
static const Value *transp_pathp_at(Machine *m, const Value *line, const Value *phi, const Value *base, const Value *r) {
  const Value *t = line_former(m, line_at(m, line, val_iend(m, 0)), VAL_PATHP);
  const Value *end = line_former(m, line_at(m, line, val_iend(m, 1)), VAL_PATHP);
  const Value *at = end == NULL ? NULL : closure_apply(m, end, r);
  const Value *nr = ival_neg(m, r);
  const Value *p = t == NULL ? NULL : path_apply(m, base, r, t->args[0], t->args[1]);
  const Value *empty = val_op(m, VAL_OP, OP_SYSTEM, 0, 0, NULL, 0);
  const Value *s;
  if (at == NULL)
    return NULL;
  if (at->kind == VAL_UNIV)
    return path_apply(m, transp_stuck(m, line, phi, base), r, end->args[0], end->args[1]);
  s = sys_add(m, sys_add(m, sys_add(m, empty, phi, constant_line(m, p)), nr, kan_closure(m, VAL_PLAM, CLOS_PATHP_END, 0, 1, line, NULL, NULL)), r, kan_closure(m, VAL_PLAM, CLOS_PATHP_END, 1, 1, line, NULL, NULL));
  return comp(m, kan_closure(m, VAL_PLAM, CLOS_PATHP_LINE, 0, 2, line, r, NULL), ival_join(m, phi, ival_join(m, nr, r)), s, p);
}

/* hfill A phi sys u0 r = hcomp A (phi \/ ~r) [j |-> sys (r /\ j),
   ~r -> u0] u0. */
static const Value *hfill(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base, const Value *r) {
  const Value *nr = ival_neg(m, r);
  return hcomp(m, ty, ival_join(m, phi, nr), sys_add(m, sys_map(m, sys, CLOS_SYS_MEET, 0, 1, r, NULL, NULL), nr, constant_line(m, base)), base);
}

/* comp L phi sys u0 = hcomp L(i1) phi [i |-> transp (fun j => L (i \/ j))
   i (sys i)] (transp L i0 u0). */
static const Value *comp(Machine *m, const Value *line, const Value *phi, const Value *sys, const Value *base) {
  return hcomp(m, line_at(m, line, val_iend(m, 1)), phi, sys_map(m, sys, CLOS_COMP_BRANCH, 0, 1, line, NULL, NULL), transp(m, line, val_iend(m, 0), base));
}

/* hcomp A phi sys u0, the rules of the S2 kickoff section 1.3 in order. */
static const Value *hcomp(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value *face;
  if (ty == NULL || phi == NULL || sys == NULL || base == NULL || !spend(m, 1))
    return NULL;
  face = face_norm(m, phi);
  if (face == NULL)
    return NULL;
  if (base->kind == VAL_TRAP)
    return base;
  /* In U a true key goes to hcomp_univ, so that its Glue makes the record
     that transp at Glue reads at an end. */
  if (ival_is_end(face, 1) && ty->kind != VAL_UNIV)
    return hcomp_true(m, ty, face, sys, base);
  switch (ty->kind) {
  case VAL_PI:
    return kan_closure4(m, VAL_LAM, CLOS_HCOMP_PI, 0, 4, ty, face, sys, base);
  case VAL_SIGMA:
    return hcomp_sigma(m, ty, face, sys, base);
  case VAL_PATHP:
    return kan_closure4(m, VAL_PLAM, CLOS_HCOMP_PATHP, 0, 4, ty, face, sys, base);
  case VAL_OP:
    if (val_is(ty, OP_GLUE) && ty->argc == 3u)
      return hcomp_glue(m, ty, face, sys, base);
    return hcomp_data(m, ty, face, sys, base);
  case VAL_UNIV:
    return hcomp_univ(m, face, sys, base);
  case VAL_TRAP:
    return ty;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_LAM:
  case VAL_PLAM:
  case VAL_IVAL:
    return internal(m, "an hcomp in a value that is not a type");
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PAPP:
    break;
  }
  return hcomp_stuck(m, ty, face, sys, base);
}

/* Glue (S3b, S3 kickoff section 1.3). SYS is an OP_SYSTEM value as hcomp
   stores it. A Glue branch does not use its tube variable, so the branch
   at i1 is the branch at any point. */

/* The Glue operation OP over the N arguments A, with the face at index PHI
   normalized, as hcomp stores its face. */
static const Value *glue_op(Machine *m, ValKind kind, Op op, const Value *const *a, uint32_t n, uint32_t phi) {
  const Value *args[4];
  uint32_t i;
  for (i = 0; i < n && i < 4u; i++)
    args[i] = i == phi ? face_norm(m, a[i]) : a[i];
  return val_op(m, kind, op, 0, 0, args, i);
}

/* The branch of SYS under its first true key, at i1, in *OUT. 0 when no
   key is true. */
static int glue_true(Machine *m, const Value *sys, const Value **out) {
  uint32_t k = true_branch(m, sys);
  if (k >= sys->argc)
    return 0;
  *out = line_at(m, sys->args[k], val_iend(m, 1));
  return 1;
}

/* Glue A phi sys: a true key gives T, the witness of its branch pack T e.
   Else Glue is a type. */
static const Value *reduce_glue(Machine *m, const Value *const *a, uint32_t n) {
  const Value *branch = NULL;
  const Value *t;
  const Value *raw;
  Value *out;
  GlueRec *rec;
  if (!glue_true(m, a[2], &branch))
    return glue_op(m, VAL_OP, OP_GLUE, a, n, 1);
  /* Record after the branch eval, so a nested Glue does not win. The
     result is a fresh copy of T: glue_at finds a record by pointer, and a
     cached T can be the result of two Glue values. */
  t = sigma_part(m, branch, 0);
  raw = glue_op(m, VAL_OP, OP_GLUE, a, n, 1);
  if (t == NULL)
    return NULL;
  out = arena_alloc(m->arena, sizeof *out);
  rec = arena_alloc(m->arena, sizeof *rec);
  if (out == NULL || rec == NULL)
    return oom(m);
  *out = *t;
  rec->out = out;
  rec->raw = raw;
  rec->next = m->glue_recs;
  m->glue_recs = rec;
  return out;
}

/* glue sys a (args phi, sys, a): a true key gives t, its branch. Else glue
   is canonical. */
static const Value *reduce_glue_elem(Machine *m, const Value *const *a, uint32_t n) {
  const Value *branch = NULL;
  return glue_true(m, a[1], &branch) ? branch : glue_op(m, VAL_OP, OP_GLUE_ELEM, a, n, 0);
}

/* unglue A phi sys g. A trap g gives the trap. A true key gives witness e
   g, with e the payload of its branch pack T e. Test this first: its
   branch type may itself be Glue, with a glue constructor for g. Else
   unglue (glue sys a) is a, and a neutral g leaves unglue stuck. */
static const Value *reduce_unglue(Machine *m, const Value *const *a, uint32_t n) {
  const Value *branch = NULL;
  if (a[3]->kind == VAL_TRAP)
    return a[3];
  if (glue_true(m, a[2], &branch))
    return apply_value(m, sigma_part(m, sigma_part(m, branch, 1), 0), a[3]);
  if (val_is(a[3], OP_GLUE_ELEM) && a[3]->argc == 3u)
    return a[3]->args[2];
  return glue_op(m, VAL_STUCK, OP_UNGLUE, a, n, 1);
}

/* unglue G at the Glue type TY (args A, phi, sys), or NULL for a NULL G. */
static const Value *unglue_value(Machine *m, const Value *ty, const Value *g) {
  const Value *ua[4];
  if (g == NULL)
    return NULL;
  ua[0] = ty->args[0];
  ua[1] = ty->args[1];
  ua[2] = ty->args[2];
  ua[3] = g;
  return reduce_unglue(m, ua, 4);
}

/* X restricted to the Glue key KEY. For a glue element X (args phi, sys,
   a), this is the branch at i1 of a system key implied by KEY.
   Else X. There is no value-level face substitution: under KEY, u0 and
   the tubes of hcomp at Glue are glue elements, not T elements. */
static const Value *glue_restrict(Machine *m, const Value *x, const Value *key) {
  const Value *sys;
  uint32_t k;
  if (x == NULL || !val_is(x, OP_GLUE_ELEM) || x->argc != 3u)
    return x;
  sys = x->args[1];
  for (k = 0; k + 1u < sys->argc; k += 2u)
    if (face_implies(m, key, sys->args[k]))
      return line_at(m, sys->args[k + 1u], val_iend(m, 1));
  return x;
}

/* hcomp at Glue A phi sys (S3c item 11, CCHM): glue [phi_k -> hcomp T_k
   psi u u0] a1, with a1 = hcomp A (psi \/ phi) [psi -> unglue (u j), phi_k
   -> witness e_k (hfill T_k psi u u0 j)] (unglue u0). One branch for each
   key phi_k of the Glue system, as transp_glue. u and u0 are restricted
   to phi_k (glue_restrict) in the T_k parts. */
static const Value *hcomp_glue(Machine *m, const Value *ty, const Value *phi, const Value *sys, const Value *base) {
  const Value *gs = ty->args[2];
  const Value *sys_a = sys_map(m, sys, CLOS_SYS_UNGLUE, 0, 1, ty, NULL, NULL);
  const Value **sg;
  const Value *pack;
  const Value *sys_k;
  const Value *base_k;
  const Value *ga[3];
  uint32_t k;
  sg = arena_alloc(m->arena, ((size_t)gs->argc + 1u) * sizeof *sg);
  if (sg == NULL)
    return oom(m);
  for (k = 0; k + 1u < gs->argc; k += 2u) {
    pack = line_at(m, gs->args[k + 1u], val_iend(m, 1));
    sys_k = sys_map(m, sys, CLOS_SYS_RESTRICT, 0, 1, gs->args[k], NULL, NULL);
    base_k = glue_restrict(m, base, gs->args[k]);
    if (sys_k == NULL)
      return NULL;
    sys_a = sys_add(m, sys_a, gs->args[k], kan_closure4(m, VAL_PLAM, CLOS_GLUE_HFILL, 0, 4, pack, phi, sys_k, base_k));
    sg[k] = gs->args[k];
    sg[k + 1u] = constant_line(m, hcomp(m, sigma_part(m, pack, 0), phi, sys_k, base_k));
    if (sg[k + 1u] == NULL || sys_a == NULL)
      return NULL;
  }
  ga[0] = ty->args[1];
  ga[1] = system_value(m, sg, gs->argc);
  ga[2] = hcomp(m, ty->args[0], face_join(m, phi, ty->args[1]), sys_a, unglue_value(m, ty, base));
  if (ga[1] == NULL || ga[2] == NULL)
    return NULL;
  return reduce_glue_elem(m, ga, 3);
}

/* transp at Glue (S3b item 8, CCHM as cubicaltt compGlue). */

/* forall i. PHI, with i the variable at LEVEL: the conjunctions of the
   face PHI that have no literal of LEVEL. */
static const Value *ival_forall(Machine *m, const Value *phi, uint32_t level) {
  IConj *cs;
  uint32_t i;
  uint32_t j;
  uint32_t k = 0;
  int has;
  if (phi == NULL)
    return NULL;
  cs = arena_alloc(m->arena, ((size_t)phi->conj_count + 1u) * sizeof *cs);
  if (cs == NULL)
    return oom(m);
  for (i = 0; i < phi->conj_count; i++) {
    has = 0;
    for (j = 0; j < phi->conj[i].count; j++)
      has = has || (phi->conj[i].lits[j] >> 1u) == level;
    if (!has)
      cs[k++] = phi->conj[i];
  }
  return face_norm(m, ival_make(m, cs, k));
}

/* The interval term PHI with S in place of the variable at LEVEL. */
static const Value *ival_replace(Machine *m, const Value *phi, uint32_t level, const Value *s) {
  const Value *out = val_iend(m, 0);
  const Value *ns = ival_neg(m, s);
  const Value *t;
  uint32_t i;
  uint32_t j;
  uint32_t x;
  if (phi == NULL || ns == NULL)
    return NULL;
  for (i = 0; i < phi->conj_count && out != NULL; i++) {
    t = val_iend(m, 1);
    for (j = 0; j < phi->conj[i].count; j++) {
      x = phi->conj[i].lits[j];
      t = ival_meet(m, t, (x >> 1u) != level ? ival_lit(m, x) : (x & 1u) != 0 ? ns : s);
    }
    out = ival_join(m, out, t);
  }
  return out;
}

/* Substitution in a face also removes inconsistent conjunctions. */
static const Value *ival_subst(Machine *m, const Value *phi, uint32_t level, const Value *s) {
  return face_norm(m, ival_replace(m, phi, level, s));
}

static const Value *val_isubst(Machine *m, const Value *v, uint32_t level, const Value *s);

/* ENV with val_isubst on each value, in *OUT. Returns 0 on an error. */
static int env_isubst(Machine *m, const Env *env, uint32_t level, const Value *s, const Env **out) {
  const Value **vals;
  const Env *e;
  uint32_t n = 0;
  uint32_t i;
  for (e = env; e != NULL; e = e->next)
    n++;
  vals = arena_alloc(m->arena, ((size_t)n + 1u) * sizeof *vals);
  if (vals == NULL) {
    oom(m);
    return 0;
  }
  for (e = env, i = 0; e != NULL; e = e->next, i++) {
    vals[i] = val_isubst(m, e->value, level, s);
    if (vals[i] == NULL)
      return 0;
  }
  *out = NULL;
  for (i = n; i > 0; i--) {
    *out = env_push(m, *out, vals[i - 1u]);
    if (*out == NULL)
      return 0;
  }
  return 1;
}

/* V with S for the interval variable at LEVEL in each interval value,
   argument and closure env (glue_at, S4a). Nothing reduces: a system only
   drops its false keys. A NULL part (a PAPP end) stays NULL. */
static const Value *val_isubst(Machine *m, const Value *v, uint32_t level, const Value *s) {
  const Value **args;
  const Value *r = v;
  Value *out;
  uint32_t i;
  if (v == NULL || !spend(m, 1) || !enter(m))
    return NULL;
  switch (v->kind) {
  case VAL_IVAL:
    r = ival_replace(m, v, level, s);
    break;
  case VAL_UNIV:
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_VAR:
  case VAL_TRAP:
    break;
  case VAL_OP:
  case VAL_STUCK:
  case VAL_LAM:
  case VAL_PLAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_PATHP:
  case VAL_APP:
  case VAL_PAPP:
    args = arena_alloc(m->arena, ((size_t)v->argc + 1u) * sizeof *args);
    out = arena_alloc(m->arena, sizeof *out);
    if (args == NULL || out == NULL) {
      m->depth--;
      return oom(m);
    }
    *out = *v;
    out->args = args;
    r = out;
    for (i = 0; i < v->argc && r != NULL; i++) {
      args[i] = v->args[i] == NULL ? NULL : val_isubst(m, v->args[i], level, s);
      r = v->args[i] != NULL && args[i] == NULL ? NULL : r;
    }
    out->dom = r == NULL || v->dom == NULL ? v->dom : val_isubst(m, v->dom, level, s);
    out->arg = r == NULL || v->arg == NULL ? v->arg : val_isubst(m, v->arg, level, s);
    r = (v->dom != NULL && out->dom == NULL) || (v->arg != NULL && out->arg == NULL) ? NULL : r;
    if (r != NULL && v->env != NULL && !env_isubst(m, v->env, level, s, &out->env))
      r = NULL;
    if (r != NULL && v->kind == VAL_OP && v->op == OP_SYSTEM)
      r = system_value(m, args, v->argc);
    break;
  }
  m->depth--;
  return r;
}

/* The OP_GLUE value of the Glue line L at R. A true key reduces Glue to T,
   so then the record that reduce_glue makes gives A, phi and sys. With no
   record (a fold of a path constructor at an end gives the end case, not
   the case path at the end), it is the Glue of L at a fresh variable, with
   R for the variable. */
static const Value *glue_at(Machine *m, const Value *line, const Value *r) {
  const GlueRec *h0 = m->glue_recs;
  const GlueRec *rec;
  const Value *t;
  const Value *fv;
  t = line_at(m, line, r);
  if (t == NULL)
    return NULL;
  /* Only the records of this evaluation, newest first. A reduced branch
     type can itself be Glue: the newest record for T is the outermost
     Glue that reduced to T, and it takes priority over the Glue metadata
     of T. A nested glue_at adds records but does not remove them. */
  for (rec = m->glue_recs; rec != NULL && rec != h0; rec = rec->next)
    if (rec->out == t && rec->raw != NULL)
      return rec->raw;
  if (val_is(t, OP_GLUE) && t->argc == 3u)
    return t;
  fv = fresh_ivar(m);
  t = fv == NULL ? NULL : line_at(m, line, fv);
  if (t == NULL)
    return NULL;
  if (!val_is(t, OP_GLUE) || t->argc != 3u)
    return internal(m, "a line that changes its type former");
  return val_isubst(m, t, fv->conj[0].lits[0] >> 1u, r);
}

/* The branch pack T e of the Glue line L at S whose key is KEYF at S. KEYF
   is a key at the fresh LEVEL. The system drops a false key, so a branch
   is found by its key, not by its index. */
static const Value *glue_branch_at(Machine *m, const Value *line, const Value *keyf, uint32_t level, const Value *s) {
  const Value *g = glue_at(m, line, s);
  const Value *want = ival_subst(m, keyf, level, s);
  const Value *sys;
  uint32_t k;
  if (g == NULL || want == NULL)
    return NULL;
  sys = g->args[2];
  for (k = 0; k + 1u < sys->argc; k += 2u)
    if (ival_equal(sys->args[k], want))
      return line_at(m, sys->args[k + 1u], val_iend(m, 1));
  return internal(m, "a Glue branch that changes its face");
}

/* The value of the domain library def NAME (the global lookup of R3 (a)). */
static const Value *domain_def(Machine *m, const char *name) {
  uint32_t i;
  for (i = 0; i < m->def_count; i++)
    if (m->defs[i].name != NULL && strcmp(m->defs[i].name, name) == 0)
      return def_value(m, i);
  return internal(m, "a Kan rule that needs a domain library def that is not there");
}

/* fiber T A f y of the domain library. */
static const Value *fiber_type(Machine *m, const Value *t, const Value *a, const Value *f, const Value *y) {
  return apply_value(m, apply_value(m, apply_value(m, apply_value(m, domain_def(m, "fiber"), t), a), f), y);
}

/* hcomp in the universe (S3c item 12, CCHM): hcomp (Type 0) psi [k -> A j]
   A0 = Glue A0 psi [k -> pack (A i1) e], with e = lineToEquiv (fun j => A
   (~j)) = transp (fun j => Equiv (A i1) (A (~j))) i0 (idEquiv (A i1)). A
   key that is i0 has no branch. */
static const Value *hcomp_univ(Machine *m, const Value *phi, const Value *sys, const Value *base) {
  const Value **sg;
  const Value *t1;
  const Value *e;
  const Value *ga[3];
  uint32_t k;
  uint32_t n = 0;
  sg = arena_alloc(m->arena, ((size_t)sys->argc + 1u) * sizeof *sg);
  if (sg == NULL)
    return oom(m);
  for (k = 0; k + 1u < sys->argc; k += 2u)
    if (!ival_is_end(face_norm(m, sys->args[k]), 0)) {
      t1 = closure_apply(m, sys->args[k + 1u], val_iend(m, 1));
      e = t1 == NULL ? NULL : transp(m, kan_closure(m, VAL_PLAM, CLOS_EQUIV_LINE, 0, 2, sys->args[k + 1u], t1, NULL), val_iend(m, 0), apply_value(m, domain_def(m, "idEquiv"), t1));
      sg[n] = sys->args[k];
      sg[n + 1u] = e == NULL ? NULL : constant_line(m, make(m, VAL_OP, OP_PACK, 0, 2, t1, e, NULL));
      if (sg[n + 1u] == NULL)
        return NULL;
      n += 2u;
    }
  ga[0] = base;
  ga[1] = phi;
  ga[2] = system_value(m, sg, n);
  if (ga[2] == NULL)
    return NULL;
  return reduce_glue(m, ga, 3);
}

/* transp L psi u0 with L(i) = Glue (A i) (phi i) sys(i). Branch k has the
   key key_k and delta_k = forall i. key_k. The result is glue [key_l(i1)
   -> t1_l] a1, with t1_l and w_l from the fiber extension of branch l at
   i1, and a1 = hcomp (A i1) (phi i1 \/ psi) [key_l(i1) -> w_l @ ~j, psi ->
   a1'] a1'. */
static const Value *transp_glue(Machine *m, const Value *line, const Value *psi, const Value *u0) {
  const Value *fv = fresh_ivar(m);
  const Value *gf;
  const Value *g0;
  const Value *g1;
  const Value *gs;
  const Value *s1;
  const Value **dks;
  const Value **sg;
  const Value *ua[4];
  const Value *ga[3];
  const Value *empty = val_op(m, VAL_OP, OP_SYSTEM, 0, 0, NULL, 0);
  const Value *delta = val_iend(m, 0);
  const Value *a0;
  const Value *a1p;
  const Value *a1;
  const Value *sys_a;
  const Value *sys_r;
  const Value *sys_f;
  const Value *b;
  const Value *e;
  const Value *f;
  const Value *cl;
  const Value *center;
  const Value *contr;
  const Value *refl;
  const Value *ext;
  const Value *t1;
  const Value *fill;
  const Value *cover;
  uint32_t level;
  uint32_t k;
  uint32_t l;
  if (fv == NULL || empty == NULL)
    return NULL;
  level = fv->conj[0].lits[0] >> 1u;
  gf = glue_at(m, line, fv);
  g0 = gf == NULL ? NULL : glue_at(m, line, val_iend(m, 0));
  g1 = g0 == NULL ? NULL : glue_at(m, line, val_iend(m, 1));
  if (g1 == NULL)
    return NULL;
  gs = gf->args[2];
  s1 = g1->args[2];
  dks = arena_alloc(m->arena, ((size_t)gs->argc + 1u) * sizeof *dks);
  sg = arena_alloc(m->arena, ((size_t)s1->argc + 1u) * sizeof *sg);
  if (dks == NULL || sg == NULL)
    return oom(m);
  /* a0 = unglue u0 at i0. */
  ua[0] = g0->args[0];
  ua[1] = g0->args[1];
  ua[2] = g0->args[2];
  ua[3] = u0;
  a0 = reduce_unglue(m, ua, 4);
  /* a1' = comp A (psi \/ delta) [psi -> a0, delta_k -> witness (e_k i)
     (fill_k i)] a0. No regularity, so the psi branch stays. */
  sys_a = sys_add(m, empty, psi, constant_line(m, a0));
  for (k = 0; k + 1u < gs->argc; k += 2u) {
    dks[k] = ival_forall(m, gs->args[k], level);
    if (dks[k] == NULL)
      return NULL;
    delta = face_join(m, delta, dks[k]);
    if (!ival_is_end(dks[k], 0))
      sys_a = sys_add(m, sys_a, dks[k], kan_closure4(m, VAL_PLAM, CLOS_GLUE_FILL, level, 4, line, gs->args[k], psi, u0));
  }
  cover = face_join(m, psi, delta);
  a1p = comp(m, kan_closure(m, VAL_PLAM, CLOS_GLUE_BASE, 0, 1, line, NULL, NULL), cover, sys_a, a0);
  if (a1p == NULL || cover == NULL)
    return NULL;
  refl = constant_line(m, a1p);
  /* One fiber extension for each branch l at i1. */
  sys_r = sys_add(m, empty, psi, refl);
  for (l = 0; l + 1u < s1->argc; l += 2u) {
    b = line_at(m, s1->args[l + 1u], val_iend(m, 1));
    e = sigma_part(m, b, 1);
    f = sigma_part(m, e, 0);
    cl = apply_value(m, sigma_part(m, e, 1), a1p);
    center = sigma_part(m, cl, 0);
    contr = sigma_part(m, cl, 1);
    sys_f = sys_add(m, empty, psi, kan_closure(m, VAL_PLAM, CLOS_CONTR_AT, 0, 3, contr, make(m, VAL_OP, OP_PACK, 0, 2, u0, refl, NULL), center));
    for (k = 0; k + 1u < gs->argc; k += 2u)
      if (!ival_is_end(dks[k], 0)) {
        fill = transp(m, kan_closure(m, VAL_PLAM, CLOS_GLUE_T, level, 3, line, gs->args[k], val_iend(m, 1)), psi, u0);
        sys_f = sys_add(m, sys_f, dks[k], kan_closure(m, VAL_PLAM, CLOS_CONTR_AT, 0, 3, contr, make(m, VAL_OP, OP_PACK, 0, 2, fill, refl, NULL), center));
      }
    ext = hcomp(m, fiber_type(m, sigma_part(m, b, 0), g1->args[0], f, a1p), cover, sys_f, center);
    t1 = sigma_part(m, ext, 0);
    sys_r = sys_add(m, sys_r, s1->args[l], kan_closure(m, VAL_PLAM, CLOS_PATH_REV, 0, 3, sigma_part(m, ext, 1), apply_value(m, f, t1), a1p));
    sg[l] = s1->args[l];
    sg[l + 1u] = constant_line(m, t1);
    if (sg[l + 1u] == NULL || sys_r == NULL)
      return NULL;
  }
  a1 = hcomp(m, g1->args[0], face_join(m, g1->args[1], psi), sys_r, a1p);
  ga[0] = g1->args[1];
  ga[1] = system_value(m, sg, s1->argc);
  ga[2] = a1;
  if (a1 == NULL || ga[1] == NULL)
    return NULL;
  return reduce_glue_elem(m, ga, 3);
}

/* A constructor value (S4 R2 (a)). The boundary of a path constructor is
   over the parameters, the point fields and the interval fields, in that
   order: at a true face the value is the branch, else it is the OP_CTOR
   value. `loop i0` is `base`. A point constructor has no boundary. */
static const Value *reduce_ctor(Machine *m, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  const Env *env = NULL;
  const Value *sys;
  uint32_t i;
  uint32_t k;
  if (inst >= m->ctor_count || m->ctors[inst].boundary == NULL)
    return val_op(m, VAL_OP, OP_CTOR, inst, field, a, n);
  for (i = 0; i < n; i++) {
    env = env_push(m, env, a[i]);
    if (env == NULL)
      return NULL;
  }
  sys = eval_core(m, env, m->ctors[inst].boundary);
  if (sys == NULL)
    return NULL;
  k = true_branch(m, sys);
  return k < sys->argc ? closure_apply(m, sys->args[k], val_iend(m, 1)) : val_op(m, VAL_OP, OP_CTOR, inst, field, a, n);
}

const Value *val_ctor(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  return reduce_ctor(m, inst, 0, a, n);
}

/* out (unfold g s) (S4 Q4): 1 fuel, r := g s. The layer r, a constructor
   of F ps S, becomes the same constructor of F ps (C ps), with each field
   from out_field. A neutral argument, a formal hcomp or a neutral r is
   stuck. */
static const Value *reduce_out(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  uint32_t p = m->families[inst].param_count;
  const CtorInfo *ctor;
  const Value **args;
  const Value *r;
  const Value *out;
  uint32_t i;
  if (blocked(m, a[0], OP_OUT, inst, 0, a, n, &out))
    return out;
  if (val_is(a[0], OP_HCOMP))
    return val_op(m, VAL_STUCK, OP_OUT, inst, 0, a, n);
  if (!val_is(a[0], OP_COUNFOLD) || a[0]->argc != 2u)
    return internal(m, "an out of a value that is not an unfold");
  r = spend(m, 1) ? apply_value(m, a[0]->args[0], a[0]->args[1]) : NULL;
  if (r == NULL)
    return NULL;
  if (blocked(m, r, OP_OUT, inst, 0, a, n, &out))
    return out;
  ctor = val_is(r, OP_CTOR) && r->inst < m->ctor_count ? &m->ctors[r->inst] : NULL;
  if (ctor == NULL || ctor->family != m->families[inst].layer || r->argc != p + 1u + ctor->field_count)
    return internal(m, "a layer that is not a constructor of the layer family");
  args = arena_alloc(m->arena, (size_t)r->argc * sizeof *args);
  if (args == NULL)
    return oom(m);
  for (i = 0; i < p; i++)
    args[i] = r->args[i];
  args[p] = val_op(m, VAL_OP, OP_FAMILY, inst, 0, r->args, p);
  for (i = 0; i < ctor->field_count; i++)
    args[p + 1u + i] = out_field(m, a[0], ctor->fields[i].type, i, r->args[p + 1u + i]);
  return val_op(m, VAL_OP, OP_CTOR, r->inst, r->field, args, r->argc);
}

/* textEq a b (S5): some of the unit value when the bytes agree, else none.
   A neutral argument stays stuck. S6R K4: convertible arguments give some
   of the unit value, also when they are neutral. The levels of the test
   are above the fresh levels, as in line_constant (open point 1). */
static const Value *text_eq(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  const Value **some;
  if (a[0]->kind == VAL_TRAP || a[1]->kind == VAL_TRAP)
    return a[0]->kind == VAL_TRAP ? a[0] : a[1];
  if (!conv_values(m, KAN_FRESH_BASE + KAN_FRESH_MASK + 1u, a[0], a[1])) {
    if (blocked(m, a[0], OP_TEXT_EQ, 0, 0, a, n, &out) || blocked(m, a[1], OP_TEXT_EQ, 0, 0, a, n, &out))
      return out;
    if (a[0]->kind != VAL_TEXT || a[1]->kind != VAL_TEXT)
      return internal(m, "a Text operation on a value that is not a Text");
    if (a[0]->nat != a[1]->nat || memcmp(a[0]->name, a[1]->name, (size_t)a[0]->nat) != 0)
      return constant(m, OP_NONE);
  }
  some = arena_alloc(m->arena, sizeof *some);
  if (some == NULL)
    return oom(m);
  some[0] = constant(m, OP_UNIT_VAL);
  return some[0] == NULL ? NULL : val_op(m, VAL_OP, OP_SOME, 0, 0, some, 1);
}

/* creditsCheck n (S5): n when n is below 2^64, else the run-time refusal
   EVAL_CREDITS_OVERFLOW. A neutral argument stays stuck. */
static const Value *credits_check(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  Big b;
  uint64_t x = 0;
  if (a[0]->kind == VAL_TRAP)
    return a[0];
  if (nat_blocked(m, a[0], OP_CREDITS_CHECK, a, n, &out))
    return out;
  if (!is_nat(a[0]))
    return internal(m, "a Nat operation on a value that is not a Nat");
  if (!big_of(m, a[0], &b))
    return oom(m);
  if (big_fits_u64(b, &x))
    return val_nat(m, x);
  diag_fail(m->diag, "EVAL_CREDITS_OVERFLOW", m->def, "a Credits value is above 2^64 - 1");
  return NULL;
}

/* refUnknown A id (S5): the run-time refusal EVAL_REF_UNKNOWN at a literal
   id, as no instance in the world has the id. A neutral id stays stuck. */
static const Value *ref_unknown(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  Big b;
  const char *digits = NULL;
  size_t len = 0;
  if (a[1]->kind == VAL_TRAP)
    return a[1];
  if (nat_blocked(m, a[1], OP_REF_UNKNOWN, a, n, &out))
    return out;
  if (!is_nat(a[1]))
    return internal(m, "a reference id that is not a Nat");
  if (!big_of(m, a[1], &b) || !big_text(m->arena, b, &digits, &len))
    return oom(m);
  diag_fail(m->diag, "EVAL_REF_UNKNOWN", m->def, "the reference id %.*s has no instance in the world", (int)len, digits);
  return NULL;
}

static const Value *reduce_op(Machine *m, Op op, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  if (!arity_ok(m, op, inst, n))
    return internal(m, "an operation with the wrong number of arguments");
  switch (op) {
  case OP_TEXT:
  case OP_NAT:
  case OP_FIN:
  case OP_EMPTY:
  case OP_UNIT:
  case OP_PROD:
  case OP_SUM:
  case OP_OPTION:
  case OP_LIST:
  case OP_INTERVAL:
  case OP_FAMILY:
  case OP_UNIT_VAL:
  case OP_COUNFOLD:
  case OP_PAIR:
  case OP_INL:
  case OP_INR:
  case OP_NONE:
  case OP_SOME:
  case OP_NIL:
  case OP_CONS:
  case OP_PACK:
    return val_op(m, VAL_OP, op, inst, field, a, n);
  case OP_OUT:
    return reduce_out(m, inst, a, n);
  case OP_CTOR:
    return reduce_ctor(m, inst, field, a, n);
  case OP_SYSTEM:
    return system_value(m, a, n);
  case OP_FIRST:
    return reduce_part(m, op, a, n, OP_PAIR, 0);
  case OP_SECOND:
    return reduce_part(m, op, a, n, OP_PAIR, 1);
  case OP_WITNESS:
    return reduce_part(m, op, a, n, OP_PACK, 0);
  case OP_PAYLOAD:
    return reduce_part(m, op, a, n, OP_PACK, 1);
  case OP_EITHER:
    return reduce_either(m, a, n);
  case OP_OPTION_ELIM:
    return reduce_option(m, a, n);
  case OP_ELIM:
    return reduce_elim(m, inst, field, a, n);
  case OP_PURE:
    return reduce_pure(m, inst, a[0]);
  case OP_MAP:
    return reduce_map(m, inst, a, n);
  case OP_BIND:
    return reduce_bind(m, inst, a, n);
  case OP_FOLD_NAT:
    return reduce_fold_nat(m, a, n);
  case OP_FOLD_LIST:
    return reduce_fold_list(m, a, NULL, NULL);
  case OP_FOLD_FAMILY:
    return fold_family(m, inst, a, n - 2u, a[n - 1u]);
  case OP_FILTER:
    return reduce_filter(m, inst, a, n);
  case OP_NAT_ADD:
    return nat_add(m, a, n);
  case OP_NAT_SUB:
    return nat_sub(m, a, n);
  case OP_NAT_MUL:
    return nat_mul(m, a, n);
  case OP_SUCC:
    return nat_succ(m, a, n);
  case OP_FIN_ADD:
    return fin_add(m, a, n);
  case OP_FIN_NEG:
    return fin_neg(m, a, n);
  case OP_FIN_MUL:
    return fin_mul(m, a, n);
  case OP_FIN_VAL:
    return fin_val(m, a, n);
  case OP_FIN_EQ:
    return fin_eq(m, a, n);
  case OP_FIN_CASE:
    return fin_case(m, a, n);
  case OP_DEC_EQ_FIN:
  case OP_DEC_EQ_NAT:
    return dec_eq(m, op, a, n);
  case OP_TEXT_EQ:
    return text_eq(m, a, n);
  case OP_CREDITS_CHECK:
    return credits_check(m, a, n);
  case OP_REF_UNKNOWN:
    return ref_unknown(m, a, n);
  case OP_NAT_EQ:
  case OP_NAT_LE:
    return nat_test(m, op, a, n);
  case OP_ABSURD:
    return val_op(m, VAL_STUCK, OP_ABSURD, 0, 0, a, n);
  case OP_PROJ:
    return reduce_proj(m, inst, field, a, n);
  case OP_TRANSP:
    return transp(m, a[0], a[1], a[2]);
  case OP_HCOMP:
    return hcomp(m, a[0], a[1], a[2], a[3]);
  case OP_GLUE:
    return reduce_glue(m, a, n);
  case OP_GLUE_ELEM:
    return reduce_glue_elem(m, a, n);
  case OP_UNGLUE:
    return reduce_unglue(m, a, n);
  }
  return internal(m, "an unknown operation");
}

static const Value *eval_op(Machine *m, const Env *env, const Core *c) {
  const Value **args = arena_alloc(m->arena, ((size_t)c->argc + 1u) * sizeof *args);
  Value *out;
  uint32_t i;
  if (args == NULL)
    return oom(m);
  for (i = 0; i < c->argc; i++) {
    args[i] = eval_core(m, env, c->args[i]);
    if (args[i] == NULL)
      return NULL;
    /* A finAll case at a literal: eval only the body of that literal. */
    if (i == 0 && c->op == OP_FIN_CASE && args[0]->kind == VAL_NAT)
      return args[0]->nat < c->argc - 1u ? eval_core(m, env, c->args[args[0]->nat + 1u]) : internal(m, "a Fin case on a value out of range");
  }
  /* S6R K3: option and finCase at a refUnknown give refUnknown at the
     result type in the core; the List fold takes it at its end. */
  if (c->a != NULL && (c->op == OP_OPTION_ELIM || c->op == OP_FIN_CASE) && ref_unknown_of(args[c->op == OP_FIN_CASE ? 0u : 2u]) != NULL)
    return ref_unknown_at(m, eval_core(m, env, c->a), args[c->op == OP_FIN_CASE ? 0u : 2u]);
  if (c->op == OP_FOLD_LIST)
    return reduce_fold_list(m, args, env, c->a);
  out = (Value *)reduce_op(m, c->op, c->inst, c->field, args, c->argc);
  if (out != NULL && ((out->kind == VAL_STUCK && (out->op == OP_FIN_CASE || out->op == OP_OPTION_ELIM)) || val_is(out, OP_COUNFOLD)) && c->a != NULL) {
    out->dom = eval_core(m, env, c->a);
    if (out->dom == NULL)
      return NULL;
  }
  if (out != NULL && val_is(out, OP_COUNFOLD) && c->b != NULL) {
    out->arg = eval_core(m, env, c->b);
    if (out->arg == NULL)
      return NULL;
  }
  return out;
}

static const Value *eval_pathp(Machine *m, const Env *env, const Core *c) {
  const Value **ends = arena_alloc(m->arena, 2u * sizeof *ends);
  Value *v;
  if (ends == NULL)
    return oom(m);
  ends[0] = eval_core(m, env, c->args[0]);
  ends[1] = eval_core(m, env, c->args[1]);
  v = (Value *)val_closure(m, VAL_PLAM, c->name, NULL, env, c->b);
  if (ends[0] == NULL || ends[1] == NULL || v == NULL)
    return NULL;
  v->kind = VAL_PATHP;
  v->args = ends;
  v->argc = 2;
  return v;
}

static const Value *eval_inner(Machine *m, const Env *env, const Core *c) {
  switch (c->kind) {
  case CORE_VAR:
    return env_lookup(m, env, c->index);
  case CORE_GLOBAL:
    return def_value(m, c->index);
  case CORE_NAT:
    return val_nat(m, c->nat);
  case CORE_TEXT:
    return val_text(m, c->name, c->nat);
  case CORE_BIG:
    return val_big(m, c->limbs, c->nat);
  case CORE_TRAP:
    return val_trap(m);
  case CORE_UNIV:
    return val_univ(m, c->nat);
  case CORE_LAM:
    return val_closure(m, VAL_LAM, c->name, NULL, env, c->b);
  case CORE_PI:
    return val_closure(m, VAL_PI, c->name, eval_core(m, env, c->a), env, c->b);
  case CORE_SIGMA:
    return val_closure(m, VAL_SIGMA, c->name, eval_core(m, env, c->a), env, c->b);
  case CORE_APP:
    return apply_value(m, eval_core(m, env, c->a), eval_core(m, env, c->b));
  case CORE_OP:
    return eval_op(m, env, c);
  case CORE_PLAM:
    return val_closure(m, VAL_PLAM, c->name, NULL, env, c->b);
  case CORE_PATHP:
    return eval_pathp(m, env, c);
  case CORE_PAPP:
    return path_apply(m, eval_core(m, env, c->a), eval_core(m, env, c->b), eval_core(m, env, c->args[0]), eval_core(m, env, c->args[1]));
  case CORE_IZERO:
    return val_iend(m, 0);
  case CORE_IONE:
    return val_iend(m, 1);
  case CORE_INEG:
    return ival_neg(m, eval_core(m, env, c->a));
  case CORE_IMEET:
    return ival_meet(m, eval_core(m, env, c->a), eval_core(m, env, c->b));
  case CORE_IJOIN:
    return ival_join(m, eval_core(m, env, c->a), eval_core(m, env, c->b));
  }
  return internal(m, "an unknown core term");
}

const Value *eval_core(Machine *m, const Env *env, const Core *c) {
  const Value *v;
  if (c == NULL || !spend(m, 1) || !enter(m))
    return NULL;
  v = eval_inner(m, env, c);
  m->depth--;
  return v;
}

/* Eta: a function is equal to G when the two agree on a fresh variable. */
static int conv_fun(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *x = val_var(m, level);
  return conv_values(m, level + 1u, apply_value(m, a, x), apply_value(m, b, x));
}

static int conv_binder(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *x = val_var(m, level);
  return conv_values(m, level, a->dom, b->dom) && conv_values(m, level + 1u, closure_apply(m, a, x), closure_apply(m, b, x));
}

/* Path eta (R11): a path is equal to Q when the two agree at a fresh
   interval variable. A neutral Q there is a VAL_PAPP with no ends. */
static int conv_path(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *i = val_ivar(m, level);
  const Value *at_a = a->kind == VAL_PLAM ? closure_apply(m, a, i) : path_apply(m, a, i, NULL, NULL);
  const Value *at_b = b->kind == VAL_PLAM ? closure_apply(m, b, i) : path_apply(m, b, i, NULL, NULL);
  return conv_values(m, level + 1u, at_a, at_b);
}

/* Two path types: the lines at a fresh interval variable and both ends. */
static int conv_pathp(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *i = val_ivar(m, level);
  return conv_values(m, level, a->args[0], b->args[0]) && conv_values(m, level, a->args[1], b->args[1])
      && conv_values(m, level + 1u, closure_apply(m, a, i), closure_apply(m, b, i));
}

static int same_head(const Value *a, const Value *b) {
  return a->op == b->op && a->inst == b->inst && a->field == b->field && a->argc == b->argc;
}

/* Product and Sigma eta: compare a pair with the projections of a neutral. */
static int conv_pair(Machine *m, uint32_t level, const Value *pair, const Value *neutral) {
  Op first = pair->op == OP_PAIR ? OP_FIRST : OP_WITNESS;
  Op second = pair->op == OP_PAIR ? OP_SECOND : OP_PAYLOAD;
  return conv_values(m, level, pair->args[0], make(m, VAL_STUCK, first, 0, 1, neutral, NULL, NULL))
      && conv_values(m, level, pair->args[1], make(m, VAL_STUCK, second, 0, 1, neutral, NULL, NULL));
}

/* Equal branches may cover a face in a different order or subdivision. */
static int conv_system(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *cover;
  uint32_t i;
  uint32_t j;
  for (i = 0; i + 1u < a->argc; i += 2u) {
    cover = val_iend(m, 0);
    for (j = 0; j + 1u < b->argc; j += 2u)
      if (conv_values(m, level, a->args[i + 1u], b->args[j + 1u]))
        cover = face_join(m, cover, b->args[j]);
    if (!face_implies(m, a->args[i], cover))
      return 0;
  }
  return 1;
}

/* 1 when FAM is a single-constructor layer of a cofamily. */
static int is_layer(const Machine *m, uint32_t fam) {
  uint32_t i;
  if (m->families[fam].ctor_count != 1u)
    return 0;
  for (i = 0; i < m->family_count; i++)
    if (m->families[i].cofamily && m->families[i].layer == fam)
      return 1;
  return 0;
}

/* Layer eta (S4 K5 (a)): the node A of a layer family against the neutral
   B compares by projections, as conv_pair does for a pair. */
static int conv_layer(Machine *m, uint32_t level, const Value *a, const Value *b) {
  uint32_t p = m->families[m->ctors[a->inst].family].param_count;
  uint32_t n = m->ctors[a->inst].field_count;
  uint32_t i;
  if (a->argc != p + n)
    return 0;
  for (i = 0; i < n; i++)
    if (!conv_values(m, level, a->args[p + i], reduce_proj(m, a->inst, i, &b, 1)))
      return 0;
  return 1;
}

/* A side of the bisimulation rule (S4 K5 (a)): unfold G S, or the neutral
   S read as unfold out S when G is NULL (eta). */
typedef struct {
  const Value *g;
  const Value *s;
  const Value *type;
} Side;

/* The layer of the side with the step G at the seed V: G V, or out V. */
static const Value *side_layer(Machine *m, uint32_t inst, const Value *g, const Value *v) {
  return g != NULL ? apply_value(m, g, v) : reduce_out(m, inst, &v, 1);
}

/* Field I of the layer L, a node of the constructor NODE with P
   parameters, or a neutral. */
static const Value *layer_field(Machine *m, uint32_t node, uint32_t p, const Value *l, uint32_t i) {
  if (l == NULL)
    return NULL;
  return val_is(l, OP_CTOR) && l->inst == node && p + i < l->argc ? l->args[p + i] : reduce_proj(m, node, i, &l, 1);
}

/* The field I of type T with the values A and B. Compare a Y field under
   the binders of its Pi chain; another field converts directly. */
static int bisim_field(Machine *m, uint32_t level, const Core *t, uint32_t i, const Value *a, const Value *b) {
  uint32_t depth = 0;
  uint32_t k;
  for (; t->kind == CORE_PI; t = t->b)
    depth++;
  if (a == NULL || b == NULL)
    return 0;
  if (t->kind != CORE_VAR || t->index != i + depth)
    return conv_values(m, level, a, b);
  for (k = 0; k < depth && a != NULL && b != NULL; k++, level++) {
    a = apply_value(m, a, val_var(m, level));
    b = apply_value(m, b, val_var(m, level));
  }
  return a != NULL && b != NULL && conv_values(m, level, a, b);
}

/* One check at a fresh seed x: both steps have the same seed type and
   agree field by field. Only the identity seed relation is admitted. */
static int bisim_check(Machine *m, uint32_t level, uint32_t inst, const Value *g1, const Value *g2) {
  const FamilyInfo *f = &m->families[inst];
  uint32_t node = m->families[f->layer].first_ctor;
  const CtorInfo *ctor = &m->ctors[node];
  uint32_t p = f->param_count + 1u;
  const Value *x = spend(m, 1) ? val_var(m, level) : NULL;
  const Value *l1 = x == NULL ? NULL : side_layer(m, inst, g1, x);
  const Value *l2 = l1 == NULL ? NULL : side_layer(m, inst, g2, x);
  uint32_t i;
  if (l2 == NULL)
    return 0;
  for (i = 0; i < ctor->field_count; i++)
    if (!bisim_field(m, level + 1u, ctor->fields[i].type, i, layer_field(m, node, p, l1, i), layer_field(m, node, p, l2, i)))
      return 0;
  return 1;
}

/* Unequal seeds stay distinct. Inferring a graph relation from a variable
   seed accepted open refl proofs whose closed instances were refused.
   Check the seed-preserving rule uniformly, with one unit of fuel. */
static int conv_bisim(Machine *m, uint32_t level, uint32_t inst, Side s1, Side s2) {
  if (conv_values(m, level, s1.type, s2.type) && conv_values(m, level, s1.s, s2.s))
    return bisim_check(m, level, inst, s1.g, s2.g);
  return 0;
}

/* Eta is checked uniformly and contracts an unfold even at a closed
   seed. Testing only against a neutral broke substitution of refl. */
static int counfold_eta(Machine *m, uint32_t level, const Value *u) {
  return u->arg != NULL && u->dom != NULL &&
    conv_values(m, level, u->arg, u->dom) &&
    bisim_check(m, level, u->inst, u->args[0], NULL);
}

/* A cofamily without Y fields is a finite record. Its observations can
   be compared at the actual seeds, with no coinductive hypothesis. */
static int finite_layer(const Machine *m, uint32_t inst) {
  const CtorInfo *ctor = &m->ctors[m->families[m->families[inst].layer].first_ctor];
  uint32_t i;
  for (i = 0; i < ctor->field_count; i++) {
    const Core *t = ctor->fields[i].type;
    uint32_t depth = 0;
    for (; t->kind == CORE_PI; t = t->b)
      depth++;
    if (t->kind == CORE_VAR && t->index == i + depth)
      return 0;
  }
  return 1;
}

static int conv_finite(Machine *m, uint32_t level, uint32_t inst, Side s1, Side s2) {
  const FamilyInfo *f = &m->families[inst];
  uint32_t node = m->families[f->layer].first_ctor;
  uint32_t p = f->param_count + 1u;
  const Value *l1 = spend(m, 1) ? side_layer(m, inst, s1.g, s1.s) : NULL;
  const Value *l2 = l1 == NULL ? NULL : side_layer(m, inst, s2.g, s2.s);
  uint32_t i;
  if (l2 == NULL)
    return 0;
  for (i = 0; i < m->ctors[node].field_count; i++)
    if (!conv_values(m, level, layer_field(m, node, p, l1, i), layer_field(m, node, p, l2, i)))
      return 0;
  return 1;
}

/* An unfold A or B against the other side: two equal unfolds convert as
   before. A single-constructor layer admits finite observation equality
   or typed eta and seed-preserving bisimulation. An hcomp at a cofamily
   is stuck (hcomp_stuck), so it
   is a neutral and gets eta. A formal hcomp (only at a HIT) or an other
   value does not convert. */
static int conv_counfold(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *u = val_is(a, OP_COUNFOLD) ? a : b;
  const Value *v = u == a ? b : a;
  Side su = {u->args[0], u->args[1], u->arg};
  Side sv = {NULL, v, u->dom};
  if (val_is(v, OP_COUNFOLD)) {
    if (v->inst != u->inst || u->argc != 2u || v->argc != 2u)
      return 0;
    if (conv_values(m, level, u->args[0], v->args[0]) && conv_values(m, level, u->args[1], v->args[1]))
      return 1;
    sv.g = v->args[0];
    sv.s = v->args[1];
    sv.type = v->arg;
  } else if (scrut_of(v) != SCRUT_NEUTRAL)
    return 0;
  /* Record eta and these observation checks need exactly one constructor.
     A sum layer must preserve its constructor tag. */
  if (m->families[m->families[u->inst].layer].ctor_count != 1u)
    return 0;
  if (finite_layer(m, u->inst))
    return conv_finite(m, level, u->inst, su, sv);
  if (val_is(v, OP_COUNFOLD) && counfold_eta(m, level, v))
    return conv_values(m, level, u, v->args[1]);
  if (counfold_eta(m, level, u))
    return conv_values(m, level, u->args[1], v);
  return conv_bisim(m, level, u->inst, su, sv);
}

/* The last argument of an operation is compared in the loop, so a long
   list does not nest. */
static int conv_loop(Machine *m, uint32_t level, const Value *a, const Value *b) {
  uint32_t i;
  while (a != NULL && b != NULL && spend(m, 1)) {
    if (a->kind == VAL_LAM || b->kind == VAL_LAM)
      return conv_fun(m, level, a, b);
    if (a->kind == VAL_PLAM || b->kind == VAL_PLAM)
      return conv_path(m, level, a, b);
    if ((val_is(a, OP_PAIR) || val_is(a, OP_PACK)) && scrut_of(b) == SCRUT_NEUTRAL)
      return conv_pair(m, level, a, b);
    if ((val_is(b, OP_PAIR) || val_is(b, OP_PACK)) && scrut_of(a) == SCRUT_NEUTRAL)
      return conv_pair(m, level, b, a);
    if (val_is(a, OP_COUNFOLD) || val_is(b, OP_COUNFOLD))
      return conv_counfold(m, level, a, b);
    if (val_is(a, OP_CTOR) && scrut_of(b) == SCRUT_NEUTRAL && is_layer(m, m->ctors[a->inst].family))
      return conv_layer(m, level, a, b);
    if (val_is(b, OP_CTOR) && scrut_of(a) == SCRUT_NEUTRAL && is_layer(m, m->ctors[b->inst].family))
      return conv_layer(m, level, b, a);
    if (a->kind != b->kind)
      return 0;
    switch (a->kind) {
    case VAL_NAT:
    case VAL_UNIV:
    case VAL_VAR:
      return a->nat == b->nat;
    case VAL_TEXT:
      return a->nat == b->nat && memcmp(a->name, b->name, (size_t)a->nat) == 0;
    case VAL_BIG:
      return a->nat == b->nat && memcmp(a->limbs, b->limbs, (size_t)a->nat * sizeof *a->limbs) == 0;
    case VAL_TRAP:
      return 1;
    case VAL_LAM:
    case VAL_PLAM:
      return 0;
    case VAL_PI:
    case VAL_SIGMA:
      return conv_binder(m, level, a, b);
    case VAL_PATHP:
      return conv_pathp(m, level, a, b);
    case VAL_IVAL:
      return ival_equal(a, b);
    case VAL_PAPP:
    case VAL_APP:
      if (!conv_values(m, level, a->dom, b->dom))
        return 0;
      a = a->arg;
      b = b->arg;
      break;
    case VAL_OP:
    case VAL_STUCK:
      if (val_is(a, OP_SYSTEM) && val_is(b, OP_SYSTEM))
        return conv_system(m, level, a, b) && conv_system(m, level, b, a);
      if (!same_head(a, b))
        return 0;
      if (a->argc == 0)
        return 1;
      for (i = 0; i + 1u < a->argc; i++)
        if (!conv_values(m, level, a->args[i], b->args[i]))
          return 0;
      a = a->args[a->argc - 1u];
      b = b->args[b->argc - 1u];
      break;
    }
  }
  return 0;
}

int conv_values(Machine *m, uint32_t level, const Value *a, const Value *b) {
  int same;
  if (a == NULL || b == NULL || !enter(m))
    return 0;
  same = conv_loop(m, level, a, b);
  m->depth--;
  return same;
}

static Core *new_core(Machine *m, CoreKind kind) {
  Core *c = arena_alloc(m->arena, sizeof *c);
  if (c == NULL) {
    oom(m);
    return NULL;
  }
  c->kind = kind;
  return c;
}

static const Core *core_leaf(Machine *m, CoreKind kind, uint64_t nat) {
  Core *c = new_core(m, kind);
  if (c != NULL)
    c->nat = nat;
  return c;
}

static const Core *quote_var(Machine *m, uint32_t level, const Value *v) {
  Core *c;
  if (v->nat >= level) {
    internal(m, "a variable above its level");
    return NULL;
  }
  c = new_core(m, CORE_VAR);
  if (c != NULL)
    c->index = level - 1u - (uint32_t)v->nat;
  return c;
}

static const Core *quote_app(Machine *m, uint32_t level, const Value *v) {
  const Core *fn = quote_value(m, level, v->dom);
  const Core *arg = quote_value(m, level, v->arg);
  Core *c;
  if (fn == NULL || arg == NULL)
    return NULL;
  c = new_core(m, CORE_APP);
  if (c == NULL)
    return NULL;
  c->a = fn;
  c->b = arg;
  return c;
}

static const Core *core_pair(Machine *m, CoreKind kind, const Core *a, const Core *b) {
  Core *c;
  if (a == NULL || (b == NULL && kind != CORE_INEG))
    return NULL;
  c = new_core(m, kind);
  if (c == NULL)
    return NULL;
  c->a = a;
  c->b = b;
  return c;
}

/* The two ends of a PATHP or a PAPP value. */
static const Core *const *quote_ends(Machine *m, uint32_t level, const Value *v) {
  const Core **ends = arena_alloc(m->arena, 2u * sizeof *ends);
  if (ends == NULL) {
    oom(m);
    return NULL;
  }
  if (v->argc != 2 || v->args[0] == NULL || v->args[1] == NULL) {
    internal(m, "a path with no ends");
    return NULL;
  }
  ends[0] = quote_value(m, level, v->args[0]);
  ends[1] = quote_value(m, level, v->args[1]);
  return ends[0] != NULL && ends[1] != NULL ? ends : NULL;
}

static const Core *quote_binder(Machine *m, uint32_t level, const Value *v, CoreKind kind) {
  int has_dom = kind == CORE_PI || kind == CORE_SIGMA;
  int interval = kind == CORE_PLAM || kind == CORE_PATHP;
  const Core *dom = has_dom ? quote_value(m, level, v->dom) : NULL;
  const Core *body = quote_value(m, level + 1u, closure_apply(m, v, interval ? val_ivar(m, level) : val_var(m, level)));
  const Core *const *ends = kind == CORE_PATHP ? quote_ends(m, level, v) : NULL;
  Core *c;
  if (body == NULL || (dom == NULL && has_dom) || (ends == NULL && kind == CORE_PATHP))
    return NULL;
  c = new_core(m, kind);
  if (c == NULL)
    return NULL;
  c->name = v->name;
  c->a = dom;
  c->b = body;
  c->args = ends;
  c->argc = ends != NULL ? 2u : 0u;
  return c;
}

static const Core *quote_lit(Machine *m, uint32_t level, uint32_t lit) {
  Core *c;
  if ((lit >> 1u) >= level) {
    internal(m, "an interval variable above its level");
    return NULL;
  }
  c = new_core(m, CORE_VAR);
  if (c == NULL)
    return NULL;
  c->index = level - 1u - (lit >> 1u);
  return (lit & 1u) != 0u ? core_pair(m, CORE_INEG, c, NULL) : c;
}

static const Core *quote_ival(Machine *m, uint32_t level, const Value *v) {
  const Core *out = core_leaf(m, CORE_IZERO, 0);
  const Core *t;
  const IConj *k;
  uint32_t i;
  uint32_t j;
  for (i = 0; i < v->conj_count; i++) {
    k = &v->conj[i];
    t = k->count == 0 ? core_leaf(m, CORE_IONE, 0) : quote_lit(m, level, k->lits[0]);
    for (j = 1; j < k->count; j++)
      t = core_pair(m, CORE_IMEET, t, quote_lit(m, level, k->lits[j]));
    out = i == 0 ? t : core_pair(m, CORE_IJOIN, out, t);
  }
  return out;
}

static const Core *quote_papp(Machine *m, uint32_t level, const Value *v) {
  const Core *path = quote_value(m, level, v->dom);
  const Core *r = quote_value(m, level, v->arg);
  const Core *const *ends = quote_ends(m, level, v);
  Core *c;
  if (path == NULL || r == NULL || ends == NULL)
    return NULL;
  c = new_core(m, CORE_PAPP);
  if (c == NULL)
    return NULL;
  c->a = path;
  c->b = r;
  c->args = ends;
  c->argc = 2;
  return c;
}

static const Core *quote_op(Machine *m, uint32_t level, const Value *v) {
  const Core **args = arena_alloc(m->arena, ((size_t)v->argc + 1u) * sizeof *args);
  Core *c;
  uint32_t i;
  if (args == NULL) {
    oom(m);
    return NULL;
  }
  for (i = 0; i < v->argc; i++) {
    args[i] = quote_value(m, level, v->args[i]);
    if (args[i] == NULL)
      return NULL;
  }
  c = new_core(m, CORE_OP);
  if (c == NULL)
    return NULL;
  c->op = v->op;
  c->inst = v->inst;
  c->field = v->field;
  c->args = args;
  c->argc = v->argc;
  if ((v->op == OP_FIN_CASE || v->op == OP_OPTION_ELIM || v->op == OP_FOLD_LIST || v->op == OP_COUNFOLD) && v->dom != NULL) {
    c->a = quote_value(m, level, v->dom);
    if (c->a == NULL)
      return NULL;
  }
  if (v->op == OP_COUNFOLD && v->arg != NULL) {
    c->b = quote_value(m, level, v->arg);
    if (c->b == NULL)
      return NULL;
  }
  return c;
}

/* A big Nat quotes to its literal. */
static const Core *quote_literal(Machine *m, CoreKind kind, const Value *v) {
  Core *k = new_core(m, kind);
  if (k == NULL)
    return NULL;
  k->nat = v->nat;
  k->name = v->name;
  k->limbs = v->limbs;
  return k;
}

static const Core *quote_inner(Machine *m, uint32_t level, const Value *v) {
  switch (v->kind) {
  case VAL_NAT:
    return core_leaf(m, CORE_NAT, v->nat);
  case VAL_TEXT:
    return quote_literal(m, CORE_TEXT, v);
  case VAL_BIG:
    return quote_literal(m, CORE_BIG, v);
  case VAL_TRAP:
    return core_leaf(m, CORE_TRAP, 0);
  case VAL_UNIV:
    return core_leaf(m, CORE_UNIV, v->nat);
  case VAL_VAR:
    return quote_var(m, level, v);
  case VAL_APP:
    return quote_app(m, level, v);
  case VAL_LAM:
    return quote_binder(m, level, v, CORE_LAM);
  case VAL_PI:
    return quote_binder(m, level, v, CORE_PI);
  case VAL_SIGMA:
    return quote_binder(m, level, v, CORE_SIGMA);
  case VAL_OP:
  case VAL_STUCK:
    return quote_op(m, level, v);
  case VAL_PLAM:
    return quote_binder(m, level, v, CORE_PLAM);
  case VAL_PATHP:
    return quote_binder(m, level, v, CORE_PATHP);
  case VAL_IVAL:
    return quote_ival(m, level, v);
  case VAL_PAPP:
    return quote_papp(m, level, v);
  }
  internal(m, "an unknown value");
  return NULL;
}

const Core *quote_value(Machine *m, uint32_t level, const Value *v) {
  const Core *c;
  if (v == NULL || !enter(m))
    return NULL;
  c = quote_inner(m, level, v);
  m->depth--;
  return c;
}

const char *op_name(const Machine *m, Op op, uint32_t inst, uint32_t field) {
  if (op == OP_FAMILY)
    return inst < m->family_count ? m->families[inst].name : "?";
  if (op == OP_CTOR)
    return inst < m->ctor_count ? m->ctors[inst].name : "?";
  if (op == OP_PROJ)
    return inst < m->ctor_count && field < m->ctors[inst].field_count ? m->ctors[inst].fields[field].name : "?";
  return (size_t)op < sizeof OP_NAMES / sizeof OP_NAMES[0] ? OP_NAMES[op] : "?";
}

/* The printer keeps len + 4 <= cap, so "..." always fits. */
typedef struct {
  Machine *m;
  char *buf;
  size_t cap;
  size_t len;
  int full;
  const char *const *names;
  uint32_t name_count;
  const char *extra[PRINT_NAMES_MAX];
  uint32_t extra_count;
  unsigned depth;
  unsigned depth_limit;
  int depth_cut;
} Printer;

static void put(Printer *p, const char *s) {
  size_t n = strlen(s);
  if (p->full)
    return;
  if (p->len + n + 4u <= p->cap) {
    memcpy(p->buf + p->len, s, n + 1u);
    p->len += n;
    return;
  }
  memcpy(p->buf + p->len, "...", 4u);
  p->len += 3u;
  p->full = 1;
}

static const char *var_name(const Printer *p, uint64_t level) {
  if (level < p->name_count && p->names[level] != NULL)
    return p->names[level];
  if (level >= p->name_count && level - p->name_count < p->extra_count
      && level - p->name_count < PRINT_NAMES_MAX)
    return p->extra[level - p->name_count];
  return "_";
}

static void print_value(Printer *p, const Value *v, int atom);

static int is_glue(const Value *v) {
  return v != NULL && (v->kind == VAL_OP || v->kind == VAL_STUCK) && (v->op == OP_GLUE || v->op == OP_GLUE_ELEM || v->op == OP_UNGLUE);
}

static int is_chain(const Value *v) {
  return v != NULL && (v->kind == VAL_OP || v->kind == VAL_STUCK) && v->argc > 0 && v->op != OP_SYSTEM && !is_glue(v);
}

/* A printed binder must not capture another variable or a global name. */
static int name_used(const Printer *p, const char *name) {
  uint32_t i;
  uint32_t j;
  if (name[0] == '%')
    return 1; /* checker-generated eta names are not surface identifiers */
  if (strcmp(name, "PathP") == 0)
    return 1; /* printed path types use this builtin outside OP_NAMES */
  for (i = 0; i < p->name_count; i++)
    if (p->names[i] != NULL && strcmp(name, p->names[i]) == 0)
      return 1;
  for (i = 0; i < p->extra_count && i < PRINT_NAMES_MAX; i++)
    if (strcmp(name, p->extra[i]) == 0)
      return 1;
  for (i = 0; i < sizeof OP_NAMES / sizeof OP_NAMES[0]; i++)
    if (strcmp(name, OP_NAMES[i]) == 0)
      return 1;
  for (i = 0; i < p->m->def_count; i++)
    if (strcmp(name, p->m->defs[i].name) == 0)
      return 1;
  for (i = 0; i < p->m->family_count; i++)
    if (strcmp(name, p->m->families[i].name) == 0)
      return 1;
  for (i = 0; i < p->m->ctor_count; i++) {
    const CtorInfo *c = &p->m->ctors[i];
    if (c->name != NULL && strcmp(name, c->name) == 0)
      return 1;
    for (j = 0; j < c->field_count; j++)
      if (strcmp(name, c->fields[j].name) == 0)
        return 1;
  }
  return 0;
}

static void print_binder(Printer *p, const Value *v) {
  uint32_t level = p->name_count + p->extra_count;
  const char *name = v->name != NULL ? v->name : "x";
  char fresh[32];
  uint32_t suffix = 0;
  int interval = v->kind == VAL_PLAM || v->kind == VAL_PATHP;
  const Value *body = closure_apply(p->m, v, interval ? val_ivar(p->m, level) : val_var(p->m, level));
  int binder_dom = v->dom != NULL && (v->dom->kind == VAL_PI || v->dom->kind == VAL_SIGMA || v->dom->kind == VAL_LAM || v->dom->kind == VAL_UNIV);
  while (name_used(p, name)) {
    snprintf(fresh, sizeof fresh, "_x%u", suffix++);
    name = fresh;
  }
  switch (v->kind) {
  case VAL_LAM:
  case VAL_PLAM:
    put(p, "fun ");
    put(p, name);
    put(p, " => ");
    break;
  case VAL_PATHP:
    put(p, "PathP (fun ");
    put(p, name);
    put(p, " => ");
    break;
  case VAL_PI:
    if (v->name == NULL) {
      print_value(p, v->dom, binder_dom);
      put(p, " -> ");
      break;
    }
    put(p, "(");
    put(p, name);
    put(p, " : ");
    print_value(p, v->dom, 0);
    put(p, ") -> ");
    break;
  case VAL_SIGMA:
    put(p, "Sigma (");
    put(p, name);
    put(p, " : ");
    print_value(p, v->dom, 0);
    put(p, ") ");
    break;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_UNIV:
  case VAL_OP:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_IVAL:
  case VAL_PAPP:
    break;
  }
  if (p->extra_count < PRINT_NAMES_MAX)
    p->extra[p->extra_count] = name;
  p->extra_count++;
  print_value(p, body, v->kind == VAL_SIGMA);
  p->extra_count--;
  if (v->kind == VAL_PATHP) {
    put(p, ") ");
    print_value(p, v->args[0], 1);
    put(p, " ");
    print_value(p, v->args[1], 1);
  }
}

/* A join of meets: a meet binds closer than a join, so only an atom
   position needs parentheses. */
static void print_ival(Printer *p, const Value *v, int atom) {
  int parens = atom && (v->conj_count > 1u || (v->conj_count == 1u && v->conj[0].count > 1u));
  uint32_t i;
  uint32_t j;
  uint32_t lit;
  put(p, v->conj_count == 0 ? "i0" : "");
  put(p, parens ? "(" : "");
  for (i = 0; i < v->conj_count; i++) {
    put(p, i == 0 ? "" : " \\/ ");
    put(p, v->conj[i].count == 0 ? "i1" : "");
    for (j = 0; j < v->conj[i].count; j++) {
      lit = v->conj[i].lits[j];
      put(p, j == 0 ? "" : " /\\ ");
      put(p, (lit & 1u) != 0 ? "~" : "");
      put(p, var_name(p, lit >> 1u));
    }
  }
  put(p, parens ? ")" : "");
}

/* A system prints as its tube, `fun i => [ k -> t, ... ]`: each branch is
   applied to ONE fresh interval name, so all bodies share that binder. A
   key is in the outer context. A GLUE tube prints `fun (_ : I) => [ ... ]`:
   the Glue rules refuse other binders, and a Glue branch does not use it. */
static void print_system(Printer *p, const Value *v, int atom, int glue) {
  uint32_t level = p->name_count + p->extra_count;
  const Value *first = v->argc >= 2u ? v->args[1] : NULL;
  const char *name = first != NULL && first->kind == VAL_PLAM && first->name != NULL ? first->name : "i";
  char fresh[32];
  uint32_t suffix = 0;
  uint32_t k;
  name = glue ? "_" : name;
  while (!glue && name_used(p, name)) {
    snprintf(fresh, sizeof fresh, "_x%u", suffix++);
    name = fresh;
  }
  put(p, atom ? "(" : "");
  put(p, "fun ");
  put(p, glue ? "(_ : I)" : name);
  put(p, " => [");
  if (p->extra_count < PRINT_NAMES_MAX)
    p->extra[p->extra_count] = name;
  p->extra_count++;
  for (k = 0; k + 1u < v->argc; k += 2u) {
    put(p, k == 0 ? " " : ", ");
    print_value(p, v->args[k], 0);
    put(p, " -> ");
    print_value(p, line_at(p->m, v->args[k + 1u], val_ivar(p->m, level)), 0);
  }
  p->extra_count--;
  put(p, " ]");
  put(p, atom ? ")" : "");
}

/* Glue A phi sys, glue sys a and unglue g. glue does not print phi, and
   unglue does not print A, phi and sys: the checker reads them from the
   type. */
static void print_glue(Printer *p, const Value *v, int atom) {
  uint32_t first = v->op == OP_UNGLUE ? 3u : v->op == OP_GLUE_ELEM ? 1u : 0u;
  uint32_t i;
  put(p, atom ? "(" : "");
  put(p, op_name(p->m, v->op, v->inst, v->field));
  for (i = first; i < v->argc; i++) {
    put(p, " ");
    if (val_is(v->args[i], OP_SYSTEM))
      print_system(p, v->args[i], 1, 1);
    else
      print_value(p, v->args[i], 1);
  }
  put(p, atom ? ")" : "");
}

/* A case or coiteration in an inference-only position retains its type.
   A typed identity supplies the annotation using ordinary source syntax. */
static int is_typed_value(const Value *v) {
  return v != NULL && v->dom != NULL && ((v->kind == VAL_STUCK && v->op == OP_FIN_CASE) || val_is(v, OP_COUNFOLD));
}

static int is_cell_pure(const Machine *m, const Value *v) {
  const CtorInfo *ctor;
  const FamilyInfo *family;
  if (!val_is(v, OP_CTOR) || v->inst >= m->ctor_count)
    return 0;
  ctor = &m->ctors[v->inst];
  family = &m->families[ctor->family];
  return family->cell && v->inst == family->first_ctor + family->ctor_count - 1u && v->argc == family->param_count + 1u;
}

static void print_typed(Printer *p, const Value *v, const Value *type) {
  char name[32];
  uint32_t suffix = 0;
  do {
    snprintf(name, sizeof name, "_case%u", suffix++);
  } while (name_used(p, name));
  put(p, "((fun (");
  put(p, name);
  put(p, " : ");
  print_value(p, type, 0);
  put(p, ") => ");
  put(p, name);
  put(p, ") (");
  if (is_cell_pure(p->m, v)) {
    put(p, "pure ");
    print_value(p, v->args[v->argc - 1u], 1);
  } else {
    print_value(p, v, 0);
  }
  put(p, "))");
}

/* The seed of a coiteration must infer even after its definition unfolds. */
static void print_counfold(Printer *p, const Value *v, int atom) {
  put(p, atom ? "(" : "");
  put(p, "unfold ");
  print_value(p, v->args[0], 1);
  put(p, " ");
  if (v->arg != NULL)
    print_typed(p, v->args[1], v->arg);
  else
    print_value(p, v->args[1], 1);
  put(p, atom ? ")" : "");
}

/* A Text value as a string literal that the lexer reads back (S5): \" and
   \\ for a quote and a backslash, \xHH for a control byte, every other
   byte as it is. */
static void put_text_literal(Printer *p, const Value *v) {
  char esc[8];
  uint64_t i;
  put(p, "\"");
  for (i = 0; i < v->nat; i++) {
    unsigned char ch = (unsigned char)v->name[i];
    int quote = ch == '"' || ch == '\\';
    if (quote || (ch >= 0x20u && ch != 0x7fu))
      snprintf(esc, sizeof esc, "%s%c", quote ? "\\" : "", ch);
    else
      snprintf(esc, sizeof esc, "\\x%02x", ch);
    put(p, esc);
  }
  put(p, "\"");
}

static void print_other(Printer *p, const Value *v, int atom) {
  char num[32];
  if (v == NULL) {
    put(p, "?");
    return;
  }
  if (v->kind == VAL_OP && v->op == OP_SYSTEM) {
    print_system(p, v, atom, 0);
    return;
  }
  if (is_glue(v)) {
    print_glue(p, v, atom);
    return;
  }
  switch (v->kind) {
  case VAL_NAT:
    snprintf(num, sizeof num, "%llu", (unsigned long long)v->nat);
    put(p, num);
    return;
  case VAL_BIG: {
    Big b;
    const char *text = NULL;
    size_t len = 0;
    b.limbs = v->limbs;
    b.count = (uint32_t)v->nat;
    put(p, big_text(p->m->arena, b, &text, &len) ? text : "?");
    return;
  }
  case VAL_TEXT:
    put_text_literal(p, v);
    return;
  case VAL_TRAP:
    put(p, "trap");
    return;
  case VAL_VAR:
    put(p, var_name(p, v->nat));
    return;
  case VAL_OP:
  case VAL_STUCK:
    put(p, op_name(p->m, v->op, v->inst, v->field));
    return;
  case VAL_IVAL:
    print_ival(p, v, atom);
    return;
  case VAL_UNIV:
  case VAL_APP:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_PLAM:
  case VAL_PATHP:
  case VAL_PAPP:
    break;
  }
  if (atom)
    put(p, "(");
  switch (v->kind) {
  case VAL_UNIV:
    snprintf(num, sizeof num, "Type %llu", (unsigned long long)v->nat);
    put(p, num);
    break;
  case VAL_APP:
    print_value(p, v->dom, v->dom->kind != VAL_APP && v->dom->kind != VAL_VAR);
    put(p, " ");
    print_value(p, v->arg, 1);
    break;
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_PLAM:
  case VAL_PATHP:
    print_binder(p, v);
    break;
  case VAL_PAPP:
    print_value(p, v->dom, v->dom->kind != VAL_PAPP && v->dom->kind != VAL_APP && v->dom->kind != VAL_VAR);
    put(p, " @ ");
    print_value(p, v->arg, 1);
    break;
  case VAL_TEXT:
  case VAL_BIG:
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_VAR:
  case VAL_OP:
  case VAL_STUCK:
  case VAL_IVAL:
    break;
  }
  if (atom)
    put(p, ")");
}

/* An operation prints as its name and its arguments. The last argument is
   printed in the loop, so a long list does not nest. */
static void print_value(Printer *p, const Value *v, int atom) {
  char num[32];
  uint32_t open = 0;
  uint32_t i;
  if (p->depth >= p->depth_limit) {
    put(p, "...");
    p->full = 1;
    p->depth_cut = 1;
    return;
  }
  p->depth++;
  while (is_chain(v) && !p->full && !val_is(v, OP_COUNFOLD) && !is_cell_pure(p->m, v) && !(atom && is_typed_value(v))) {
    uint32_t count = v->argc - 1u;
    if (atom) {
      put(p, "(");
      open++;
    }
    put(p, op_name(p->m, v->op, v->inst, v->field));
    /* A finite case prints as finCase n k c0 .. c(n-1) (S5a): the core has
       no n, so print the case count. */
    if (v->op == OP_FIN_CASE) {
      snprintf(num, sizeof num, " %lu", (unsigned long)(v->argc - 1u));
      put(p, num);
    }
    /* A family fold omits RES. A block fold prints its targets first. */
    if (v->op == OP_FOLD_FAMILY) {
      const FamilyInfo *f = &p->m->families[v->inst];
      count--;
      if (f->block_count > 1u) {
        count -= f->block_count;
        for (i = 0; i < f->block_count; i++) {
          put(p, " ");
          print_value(p, v->args[count + i], 1);
        }
      }
    }
    for (i = 0; i < count; i++) {
      put(p, " ");
      print_value(p, v->args[i], 1);
    }
    put(p, " ");
    v = v->args[v->argc - 1u];
    atom = 1;
  }
  if (is_cell_pure(p->m, v)) {
    const CtorInfo *ctor = &p->m->ctors[v->inst];
    uint32_t n = p->m->families[ctor->family].param_count;
    const Value *type = val_op(p->m, VAL_OP, OP_FAMILY, ctor->family, 0, v->args, n);
    print_typed(p, v, type);
  } else if (atom && is_typed_value(v))
    print_typed(p, v, v->dom);
  else if (val_is(v, OP_COUNFOLD))
    print_counfold(p, v, atom);
  else
    print_other(p, v, atom);
  for (i = 0; i < open; i++)
    put(p, ")");
  p->depth--;
}

int value_print_depth(Machine *m, const char *const *names, uint32_t name_count, const Value *v, char *buf, size_t cap, unsigned depth_limit) {
  Printer p;
  if (cap == 0)
    return 0;
  buf[0] = '\0';
  if (cap < 4u)
    return 0;
  memset(&p, 0, sizeof p);
  p.m = m;
  p.buf = buf;
  p.cap = cap;
  p.names = names;
  p.name_count = name_count;
  p.depth_limit = depth_limit < PRINT_NAMES_MAX ? depth_limit : PRINT_NAMES_MAX;
  print_value(&p, v, 0);
  return p.depth_cut ? -1 : !p.full && !m->diag->set;
}

int value_print(Machine *m, const char *const *names, uint32_t name_count, const Value *v, char *buf, size_t cap) {
  return value_print_depth(m, names, name_count, v, buf, cap, PRINT_DEPTH_MAX) == 1;
}
