/* The JSON target. The tcc-json kit only. See json.h. */
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "front/big.h"

#define JSON_TYPE_MAX 4096u

/* new-lang.sh replaces the placeholder with the language name. */
static const char LANG_NAME[] = "virtu-lang";

typedef struct {
  Machine *m;
  const char *def; /* the current instance, for diagnostics */
  char *data;
  size_t len;
  size_t cap;
  unsigned depth;
} Out;

static int put(Out *o, const char *text, size_t n) {
  if (o->len + n + 1 > o->cap) {
    size_t cap = o->cap == 0 ? 4096 : o->cap;
    char *data;
    while (cap < o->len + n + 1)
      cap *= 2;
    data = arena_alloc(o->m->arena, cap);
    if (data == NULL)
      return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
    if (o->len > 0)
      memcpy(data, o->data, o->len);
    o->data = data;
    o->cap = cap;
  }
  memcpy(o->data + o->len, text, n);
  o->len += n;
  o->data[o->len] = '\0';
  return 1;
}

static int put_text(Out *o, const char *text) {
  return put(o, text, strlen(text));
}

/* A JSON string of the LEN bytes at TEXT. Names and printed types are
   ASCII here; escape all the same. */
static int put_quoted(Out *o, const char *text, size_t len) {
  char esc[8];
  size_t i;
  int ok = put(o, "\"", 1);
  for (i = 0; ok && i < len; i++) {
    unsigned char ch = (unsigned char)text[i];
    int plain = ch >= 0x20 && ch != '"' && ch != '\\';
    if (plain)
      ok = put(o, text + i, 1);
    else {
      snprintf(esc, sizeof esc, "\\u%04x", ch);
      ok = put(o, esc, 6);
    }
  }
  return ok && put(o, "\"", 1);
}

static int put_string(Out *o, const char *text) {
  return put_quoted(o, text, strlen(text));
}

/* A big Nat: a bare JSON number of any length (Q6 (a)). */
static int put_big(Out *o, const Value *v) {
  Big b;
  const char *digits = NULL;
  size_t len = 0;
  b.limbs = v->limbs;
  b.count = (uint32_t)v->nat;
  if (!big_text(o->m->arena, b, &digits, &len))
    return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
  return put(o, digits, len);
}

static int refuse(Out *o, const char *what) {
  return diag_fail(o->m->diag, "JSON_VALUE", o->def, "the instance holds %s, which has no JSON form", what);
}

static int internal(Out *o, const char *what) {
  return diag_fail(o->m->diag, "INTERNAL", o->def, "the JSON printer found %s", what);
}

/* 1 when V is a number or a canonical value. A trap is a compile error. */
static int ready(Out *o, const Value *v) {
  switch (v->kind) {
  case VAL_BIG:
  case VAL_NAT:
  case VAL_OP:
  case VAL_PLAM:
    return 1;
  case VAL_TRAP:
    return diag_fail(o->m->diag, "EVAL_TRAP", o->def, "the instance value is a trap");
  case VAL_UNIV:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_PATHP:
    return refuse(o, "a type");
  case VAL_LAM:
    return refuse(o, "a function");
  case VAL_IVAL:
    return refuse(o, "an interval term");
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PAPP:
    return refuse(o, "a stuck term");
  }
  return internal(o, "an unknown value kind");
}

static int value_json(Out *o, const Value *type, const Value *v);

static int sum_json(Out *o, const Value *type, const Value *v) {
  if (val_is(v, OP_INL))
    return put_text(o, "{\"inl\":") && value_json(o, type->args[0], v->args[0]) && put_text(o, "}");
  if (val_is(v, OP_INR))
    return put_text(o, "{\"inr\":") && value_json(o, type->args[1], v->args[0]) && put_text(o, "}");
  return internal(o, "a Sum value that is not inl or inr");
}

/* none is null, so `some` wraps a value that can be null (ledger rule). A
   path is never null. */
static int option_json(Out *o, const Value *type, const Value *v) {
  const Value *elem = type->args[0];
  int wrap = val_is(elem, OP_OPTION);
  if (val_is(v, OP_NONE))
    return put_text(o, "null");
  if (!val_is(v, OP_SOME))
    return internal(o, "an Option value that is not none or some");
  return (!wrap || put_text(o, "{\"some\":")) && value_json(o, elem, v->args[0]) && (!wrap || put_text(o, "}"));
}

/* Walks the spine, so a long list does not nest the C stack. */
static int list_json(Out *o, const Value *type, const Value *v) {
  const Value *cell = v;
  int first = 1;
  int ok = put_text(o, "[");
  while (ok && val_is(cell, OP_CONS)) {
    ok = (first || put_text(o, ",")) && value_json(o, type->args[0], cell->args[0]) && ready(o, cell->args[1]);
    cell = cell->args[1];
    first = 0;
  }
  if (!ok)
    return 0;
  return val_is(cell, OP_NIL) ? put_text(o, "]") : internal(o, "a List value that is not nil or cons");
}

static int is_enum(const Machine *m, const FamilyInfo *f) {
  uint32_t i;
  for (i = 0; i < f->ctor_count; i++)
    if (m->ctors[f->first_ctor + i].field_count != 0)
      return 0;
  return 1;
}

/* Only constants: the name. One constructor: an object of the fields.
   Otherwise: an object with "tag" and the fields. A field type is in the
   context of the family parameters, as in check.c. */
/* A printed hcomp face or tube as a JSON string. NAMES name its free
   interval variables (NULL for a closed term). */
static int hcomp_term_json(Out *o, const char *const *names, uint32_t name_count, const Value *v) {
  char *text = arena_alloc(o->m->arena, JSON_TYPE_MAX);
  if (text == NULL)
    return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
  if (!value_print(o->m, names, name_count, v, text, JSON_TYPE_MAX))
    return diag_fail(o->m->diag, "JSON_TYPE", o->def, "an hcomp face or tube cannot be printed as source syntax");
  return put_string(o, text);
}

static int family_json(Out *o, const Value *type, const Value *v) {
  const FamilyInfo *f;
  const CtorInfo *ci;
  const Env *env = NULL;
  uint32_t i;
  int ok;
  /* A formal hcomp at a HIT (S4 R3 (a)) is a canonical value. */
  if (val_is(v, OP_HCOMP) && v->argc == 4u)
    return put_text(o, "{\"hcomp\":{\"phi\":") && hcomp_term_json(o, NULL, 0u, v->args[1])
      && put_text(o, ",\"tube\":") && hcomp_term_json(o, NULL, 0u, v->args[2]) && put_text(o, ",\"base\":")
      && value_json(o, type, v->args[3]) && put_text(o, "}}");
  if (type->inst >= o->m->family_count || !val_is(v, OP_CTOR) || v->inst >= o->m->ctor_count)
    return internal(o, "a family value that is not a constructor");
  f = &o->m->families[type->inst];
  ci = &o->m->ctors[v->inst];
  if (ci->family != type->inst || v->argc != f->param_count + ci->field_count || type->argc != f->param_count + f->index_count)
    return internal(o, "a constructor of another family");
  if (is_enum(o->m, f))
    return put_string(o, ci->name);
  for (i = 0; i < f->param_count; i++)
    env = env_push(o->m, env, type->args[i]);
  /* The point of a truncation (PLAN section 3, rule 4) is {"trunc": a}. */
  if ((strcmp(ci->name, "trunc1") == 0 || strcmp(ci->name, "trunc0") == 0) && ci->field_count == 1u) {
    const Value *ft = eval_core(o->m, env, ci->fields[0].type);
    return ft != NULL && put_text(o, "{\"trunc\":") && value_json(o, ft, v->args[f->param_count]) && put_text(o, "}");
  }
  ok = put_text(o, "{");
  if (f->ctor_count != 1)
    ok = ok && put_text(o, "\"tag\":") && put_string(o, ci->name);
  for (i = 0; ok && i < ci->field_count; i++) {
    const FieldInfo *fi = &ci->fields[i];
    const Value *ft = eval_core(o->m, env, fi->type);
    int lead = i == 0 && f->ctor_count == 1;
    ok = ft != NULL && (lead || put_text(o, ",")) && put_string(o, fi->name) && put_text(o, ":")
      && value_json(o, ft, v->args[f->param_count + i]);
    /* A later field type is over this field value (S4 item 13). */
    env = ok ? env_push(o->m, env, v->args[f->param_count + i]) : env;
    ok = ok && env != NULL;
  }
  return ok && put_text(o, "}");
}

static int op_json(Out *o, const Value *type, const Value *v) {
  char num[24];
  switch (type->op) {
  case OP_NAT:
  case OP_FIN:
    if (v->kind == VAL_BIG)
      return put_big(o, v);
    if (v->kind != VAL_NAT)
      return internal(o, "a Nat or Fin value that is not a number");
    snprintf(num, sizeof num, "%llu", (unsigned long long)v->nat);
    return put_text(o, num);
  case OP_UNIT:
    return put_text(o, "{}");
  case OP_EMPTY:
    return internal(o, "a value of the type Empty, which has no value");
  case OP_PROD:
    if (!val_is(v, OP_PAIR))
      return internal(o, "a Prod value that is not a pair");
    return put_text(o, "{\"first\":") && value_json(o, type->args[0], v->args[0]) && put_text(o, ",\"second\":")
      && value_json(o, type->args[1], v->args[1]) && put_text(o, "}");
  case OP_SUM:
    return sum_json(o, type, v);
  case OP_OPTION:
    return option_json(o, type, v);
  case OP_LIST:
    return list_json(o, type, v);
  case OP_FAMILY:
    /* S4 Q7: a cofamily value has no JSON form. */
    if (type->inst < o->m->family_count && o->m->families[type->inst].cofamily)
      return refuse(o, "a coinductive value");
    return family_json(o, type, v);
  case OP_GLUE:
    return refuse(o, "a Glue element");
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
  case OP_INTERVAL:
  case OP_TRANSP:
  case OP_SYSTEM:
  case OP_HCOMP:
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
  case OP_UNFOLD:
  case OP_FILTER:
  case OP_WITNESS:
  case OP_PAYLOAD:
  case OP_NAT_ADD:
  case OP_NAT_SUB:
  case OP_NAT_MUL:
  case OP_SUCC:
  case OP_NAT_EQ:
  case OP_NAT_LE:
  case OP_ABSURD:
  case OP_PROJ:
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
    return internal(o, "a type that is not a type former");
  }
  return internal(o, "an unknown operation");
}

static int printed_json(Out *o, const Value *v);

/* `Fin n` with a literal n. */
static int fin_literal(const Value *t) {
  return val_is(t, OP_FIN) && t->args[0]->kind == VAL_NAT;
}

/* The payload type is the Sigma body at the witness. */
static int pack_json(Out *o, const Value *type, const Value *v) {
  const Value *payload;
  if (!val_is(v, OP_PACK))
    return internal(o, "a Sigma value that is not pack");
  if (!put_text(o, "{\"witness\":") || !value_json(o, type->dom, v->args[0]))
    return 0;
  payload = closure_apply(o->m, type, v->args[0]);
  return payload != NULL && put_text(o, ",\"payload\":") && value_json(o, payload, v->args[1]) && put_text(o, "}");
}

/* The printed normal form of V, as a string. */
static int printed_json(Out *o, const Value *v) {
  char *text = arena_alloc(o->m->arena, JSON_TYPE_MAX);
  if (text == NULL)
    return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
  if (!value_print(o->m, NULL, 0, v, text, JSON_TYPE_MAX))
    return diag_fail(o->m->diag, "JSON_TYPE", o->def, "a path or its point cannot be printed as source syntax");
  return put_string(o, text);
}

/* A point that is a function or a type has no JSON form, so its printed
   normal form is the point (a string). */
static int point_json(Out *o, const Value *type, const Value *v) {
  if (type == NULL || v == NULL)
    return 0;
  if (type->kind != VAL_PI && type->kind != VAL_UNIV)
    return value_json(o, type, v);
  return printed_json(o, v);
}

/* 1 when C mentions the variable at de Bruijn index INDEX. An operation
   binds no variable. */
static int mentions(const Core *c, uint32_t index) {
  if (c == NULL)
    return 0;
  switch (c->kind) {
  case CORE_VAR:
    return c->index == index;
  case CORE_GLOBAL:
  case CORE_NAT:
  case CORE_BIG:
  case CORE_TRAP:
  case CORE_UNIV:
  case CORE_IZERO:
  case CORE_IONE:
    return 0;
  case CORE_LAM:
  case CORE_PLAM:
    return mentions(c->b, index + 1u);
  case CORE_PI:
  case CORE_SIGMA:
    return mentions(c->a, index) || mentions(c->b, index + 1u);
  case CORE_PATHP:
    return mentions(c->b, index + 1u) || mentions(c->args[0], index) || mentions(c->args[1], index);
  case CORE_PAPP:
    return mentions(c->a, index) || mentions(c->b, index) || mentions(c->args[0], index) || mentions(c->args[1], index);
  case CORE_APP:
  case CORE_INEG:
  case CORE_IMEET:
  case CORE_IJOIN:
    return mentions(c->a, index) || mentions(c->b, index);
  case CORE_OP: {
    uint32_t i;
    for (i = 0; i < c->argc; i++)
      if (mentions(c->args[i], index))
        return 1;
    return 0;
  }
  }
  return 1;
}

/* 1 when V is a number or a canonical value, including the functions and
   types that point_json prints as source strings, with no diagnostic. */
static int is_ready(const Value *v) {
  return v->kind == VAL_NAT || v->kind == VAL_OP || v->kind == VAL_PLAM
    || v->kind == VAL_LAM || v->kind == VAL_UNIV || v->kind == VAL_PI
    || v->kind == VAL_SIGMA || v->kind == VAL_PATHP;
}

/* 1 for a literal carrier, 0 for an unsupported type, -1 on an error.
   Check every Sigma fiber before printing a table, including empty
   carriers whose enumeration has no first point. */
static int carrier_literal(Machine *m, const Value *a) {
  uint64_t i;
  if (fin_literal(a))
    return 1;
  if (a->kind != VAL_SIGMA || !fin_literal(a->dom))
    return 0;
  for (i = 0; i < a->dom->args[0]->nat; i++) {
    const Value *b = closure_apply(m, a, val_nat(m, i));
    if (b == NULL || m->diag->set)
      return -1;
    if (!fin_literal(b))
      return 0;
  }
  return 1;
}

/* The point K of a literal carrier A: Fin n, or Sigma (e : Fin a) (Fin b)
   in witness-major order. NULL past the last point
   and at each other carrier. */
static const Value *carrier_point(Machine *m, const Value *a, uint64_t k) {
  uint64_t i;
  if (fin_literal(a))
    return k < a->args[0]->nat ? val_nat(m, k) : NULL;
  if (a->kind != VAL_SIGMA || !fin_literal(a->dom))
    return NULL;
  for (i = 0; i < a->dom->args[0]->nat; i++) {
    const Value *b = closure_apply(m, a, val_nat(m, i));
    const Value *pair[2];
    if (b == NULL || !fin_literal(b))
      return NULL;
    if (k < b->args[0]->nat) {
      pair[0] = val_nat(m, i);
      pair[1] = val_nat(m, k);
      return val_op(m, VAL_OP, OP_PACK, 0, 0, pair, 2);
    }
    k -= b->args[0]->nat;
  }
  return NULL;
}

/* Rule 1 (R9 (a), S6b R12 (a)): a path in a universe (the line LINE, ends
   A and B) whose end A is a literal carrier (carrier_point) is the table
   of transp (fun i => p @ i) i0 x for each point x of A. *DONE is 0 when
   the rule does not apply, or when a table value is not ready: then the
   output goes back to MARK and the next rules apply. */
static int table_json(Out *o, const Value *line, const Value *a, const Value *b, const Value *v, int *done) {
  Machine *m = o->m;
  const Value *from = closure_apply(m, line, val_iend(m, 0));
  const Value *to = closure_apply(m, line, val_iend(m, 1));
  const Value *x;
  size_t mark = o->len;
  uint64_t k;
  int literal;
  *done = 0;
  if (from == NULL || to == NULL)
    return 0;
  if (from->kind != VAL_UNIV)
    return 1;
  literal = carrier_literal(m, a);
  if (literal <= 0)
    return literal == 0 && !m->diag->set;
  if (!put_text(o, "{\"from\":") || !point_json(o, from, a) || !put_text(o, ",\"to\":")
      || !point_json(o, to, b) || !put_text(o, ",\"transport\":["))
    return 0;
  for (k = 0; (x = carrier_point(m, a, k)) != NULL; k++) {
    const Value *vk = val_transp(m, v, val_iend(m, 0), x);
    if (vk == NULL || m->diag->set)
      return 0;
    if (!is_ready(vk)) {
      o->len = mark;
      o->data[mark] = '\0';
      return 1;
    }
    if ((k > 0 && !put_text(o, ",")) || !point_json(o, b, vk))
      return 0;
  }
  *done = 1;
  return !m->diag->set && put_text(o, "]}");
}

/* 1 when TY is a family with a path constructor (a HIT). */
static int hit_family(const Machine *m, const Value *ty) {
  const FamilyInfo *f;
  uint32_t k;
  if (ty == NULL || !val_is(ty, OP_FAMILY) || ty->inst >= m->family_count)
    return 0;
  f = &m->families[ty->inst];
  for (k = 0; k < f->ctor_count; k++)
    if (f->first_ctor + k < m->ctor_count && m->ctors[f->first_ctor + k].interval_count > 0u)
      return 1;
  return 0;
}

/* 1 when the value X at the path variable (level 0) mentions it, -1 on an
   error. */
static int mentions_ivar(Machine *m, const Value *x) {
  const Core *c = quote_value(m, 1u, x);
  if (c == NULL)
    return -1;
  return mentions(c, 0u);
}

/* Rule 4 (S4 R9 (a)): X is the body of a path at the HIT type TY, at the
   path variable (level 0). A path constructor with one interval field whose
   argument is the path variable gives {"path": NAME, "args": [field
   trees]}; a formal hcomp gives {"hcomp": {"phi": printed, "tube": printed,
   "base": tree}};
   a point constructor that does not mention the variable gives its
   family_json object. *DONE is 0 for each other body: then the caller
   resets the output and rule 6 applies. */
static int hit_tree(Out *o, const Value *ty, const Value *x, int *done) {
  static const char *const names[1] = { "i" };
  Machine *m = o->m;
  const FamilyInfo *f = &m->families[ty->inst];
  const CtorInfo *ci;
  const Env *env = NULL;
  const Core *last;
  uint32_t i;
  int at;
  *done = 0;
  if (val_is(x, OP_HCOMP) && x->argc == 4u)
    return put_text(o, "{\"hcomp\":{\"phi\":") && hcomp_term_json(o, names, 1u, x->args[1])
      && put_text(o, ",\"tube\":") && hcomp_term_json(o, names, 1u, x->args[2]) && put_text(o, ",\"base\":")
      && hit_tree(o, ty, x->args[3], done) && (!*done || put_text(o, "}}"));
  if (!val_is(x, OP_CTOR) || x->inst >= m->ctor_count || m->ctors[x->inst].family != ty->inst)
    return 1;
  ci = &m->ctors[x->inst];
  if (ci->interval_count == 0u) {
    at = mentions_ivar(m, x);
    if (at != 0)
      return at == 1;
    *done = 1;
    return family_json(o, ty, x);
  }
  if (ci->interval_count != 1u || x->argc != f->param_count + ci->field_count + 1u)
    return 1;
  last = quote_value(m, 1u, x->args[x->argc - 1u]);
  if (last == NULL)
    return 0;
  if (last->kind != CORE_VAR || last->index != 0u)
    return 1;
  for (i = 0; i < f->param_count; i++)
    env = env_push(m, env, ty->args[i]);
  if (!put_text(o, "{\"path\":") || !put_string(o, ci->name) || !put_text(o, ",\"args\":["))
    return 0;
  for (i = 0; i < ci->field_count; i++) {
    const FieldInfo *fi = &ci->fields[i];
    const Value *field = x->args[f->param_count + i];
    const Value *ft;
    if (i > 0 && !put_text(o, ","))
      return 0;
    /* A field type is over the earlier field values (S4 item 13). */
    env = i == 0u ? env : env_push(m, env, x->args[f->param_count + i - 1u]);
    if (i > 0u && env == NULL)
      return 0;
    if (fi->recursive) {
      int field_done;
      ft = eval_core(m, env, fi->type);
      if (ft == NULL || !hit_tree(o, ft, field, &field_done))
        return 0;
      if (!field_done)
        return 1;
      continue;
    }
    at = mentions_ivar(m, field);
    if (at != 0)
      return at == 1;
    ft = eval_core(m, env, fi->type);
    if (ft == NULL || !value_json(o, ft, field))
      return 0;
  }
  *done = 1;
  return put_text(o, "]}");
}

static int path_rules(Out *o, const Value *line, const Value *a, const Value *b, const Value *v);

/* Rule 2 (S6b R12 (a)): a path V in a Sigma type is one object for each
   component path, {"witness": P0, "payload": P1}, each printed by the
   path rules over its own line. */
static int sigma_path_json(Out *o, const Value *line, const Value *a, const Value *b, const Value *v) {
  const Value *part[4];
  uint32_t f;
  for (f = 0; f < 2u; f++)
    if (!put_text(o, f == 0u ? "{\"witness\":" : ",\"payload\":") || !path_sigma_part(o->m, line, v, a, b, f, part)
        || !path_rules(o, part[0], part[2], part[3], part[1]))
      return 0;
  return put_text(o, "}");
}

/* Rule 3 (S6b R12 (a)): a path V of functions whose domain DOM at i0 is a
   literal carrier is the table {"paths": [P0, ...]} of the pointwise
   paths (path_pi_point), one for each point of DOM in the carrier_point
   order, each printed by the path rules over its own line. */
static int pi_path_json(Out *o, const Value *line, const Value *a, const Value *b, const Value *v, const Value *dom) {
  const Value *part[4];
  const Value *x;
  uint64_t k;
  if (!put_text(o, "{\"paths\":["))
    return 0;
  for (k = 0; (x = carrier_point(o->m, dom, k)) != NULL; k++)
    if ((k > 0 && !put_text(o, ",")) || !path_pi_point(o->m, line, v, a, b, x, part)
        || !path_rules(o, part[0], part[2], part[3], part[1]))
      return 0;
  return !o->m->diag->set && put_text(o, "]}");
}

/* The path rules in order 1, 2, 3, 5, 4, 6 at the line LINE (a PLAM in a
   universe) from A to B. Rule 1 first (table_json). Rule 2
   (sigma_path_json): a path in a Sigma type. Rule 3 (pi_path_json): a path
   of functions on a literal carrier at i0. Rule 5: the body at a fresh
   interval variable does not mention it, so the path is refl of its
   point. Rule 4 (hit_tree): a path at a HIT type. Rule 6: else the ends
   and the printed normal form of the path. An instance is closed, so the
   fresh level is 0. */
static int path_rules(Out *o, const Value *line, const Value *a, const Value *b, const Value *v) {
  Machine *m = o->m;
  const Value *at;
  const Value *head;
  const Value *start;
  const Core *body;
  const Value *point;
  size_t mark;
  int done;
  if (!table_json(o, line, a, b, v, &done))
    return 0;
  if (done)
    return 1;
  head = closure_apply(m, line, val_ivar(m, 0u));
  if (head == NULL)
    return 0;
  if (head->kind == VAL_SIGMA && v->kind == VAL_PLAM)
    return sigma_path_json(o, line, a, b, v);
  if (head->kind == VAL_PI && v->kind == VAL_PLAM) {
    start = closure_apply(m, line, val_iend(m, 0));
    if (start == NULL)
      return 0;
    if (start->kind == VAL_PI) {
      int literal = carrier_literal(m, start->dom);
      if (literal < 0)
        return 0;
      if (literal)
        return pi_path_json(o, line, a, b, v, start->dom);
    }
  }
  at = path_apply(m, v, val_ivar(m, 0u), a, b);
  body = quote_value(m, 1u, at);
  if (body == NULL)
    return 0;
  point = path_apply(m, v, val_iend(m, 0), a, b);
  if (!mentions(body, 0u))
    return put_text(o, "{\"refl\":") && point_json(o, closure_apply(m, line, val_iend(m, 0)), point) && put_text(o, "}");
  if (hit_family(m, head)) {
    mark = o->len;
    if (!hit_tree(o, head, at, &done))
      return 0;
    if (done)
      return 1;
    o->len = mark;
    o->data[mark] = '\0';
  }
  return put_text(o, "{\"from\":") && point_json(o, closure_apply(m, line, val_iend(m, 0)), a)
    && put_text(o, ",\"to\":") && point_json(o, closure_apply(m, line, val_iend(m, 1)), b)
    && put_text(o, ",\"term\":") && printed_json(o, v) && put_text(o, "}");
}

/* A PathP type holds its line as a closure: the rules take it as a PLAM. */
static int path_json(Out *o, const Value *type, const Value *v) {
  const Value *line = val_closure(o->m, VAL_PLAM, type->name, NULL, type->env, type->body);
  return line != NULL && path_rules(o, line, type->args[0], type->args[1], v);
}

static int typed_json(Out *o, const Value *type, const Value *v) {
  switch (type->kind) {
  case VAL_OP:
    return op_json(o, type, v);
  case VAL_SIGMA:
    return pack_json(o, type, v);
  case VAL_PATHP:
    return path_json(o, type, v);
  case VAL_BIG:
  case VAL_PI:
    return refuse(o, "a function");
  case VAL_UNIV:
    return refuse(o, "a type");
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PLAM:
  case VAL_IVAL:
  case VAL_PAPP:
    return refuse(o, "a value of a stuck type");
  }
  return internal(o, "an unknown type kind");
}

static int value_json(Out *o, const Value *type, const Value *v) {
  int ok;
  if (!ready(o, v))
    return 0;
  if (o->depth >= JSON_DEPTH_LIMIT)
    return diag_fail(o->m->diag, "JSON_DEPTH", o->def, "the instance nests deeper than %u levels", JSON_DEPTH_LIMIT);
  o->depth++;
  ok = typed_json(o, type, v);
  o->depth--;
  return ok;
}

/* Functions, types and families are not instances. A path is an instance
   (R8). */
/* A def at a function type, a type or a cofamily type (S4 Q7) is left out
   of the document. */
static int is_instance(const Machine *m, const DefInfo *d) {
  if (d->origin != ORIGIN_PROGRAM)
    return 0;
  switch (d->type->kind) {
  case VAL_PI:
  case VAL_UNIV:
  case VAL_BIG:
    return 0;
  case VAL_OP:
    return !(d->type->op == OP_FAMILY && d->type->inst < m->family_count && m->families[d->type->inst].cofamily);
  case VAL_PATHP:
  case VAL_SIGMA:
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PLAM:
  case VAL_IVAL:
  case VAL_PAPP:
    return 1;
  }
  return 1;
}

static int instance_json(Out *o, uint32_t index, char *type_text, int first) {
  Machine *m = o->m;
  const DefInfo *d = &m->defs[index];
  const Value *v;
  o->def = d->name;
  m->def = d->name;
  m->fuel = EVAL_FUEL_STEPS;
  m->depth = 0;
  v = def_value(m, index);
  if (v == NULL)
    return 0;
  if (!value_print(m, NULL, 0, d->type, type_text, JSON_TYPE_MAX))
    return diag_fail(m->diag, "JSON_TYPE", d->name, "the instance type cannot be printed as source syntax");
  return (first || put_text(o, ",")) && put_text(o, "{\"name\":") && put_string(o, d->name)
    && put_text(o, ",\"type\":") && put_string(o, type_text) && put_text(o, ",\"value\":")
    && value_json(o, d->type, v) && put_text(o, "}");
}

int json_document(Machine *m, const char **text, size_t *len) {
  Out o;
  char *type_text = arena_alloc(m->arena, JSON_TYPE_MAX);
  uint32_t i;
  int first = 1;
  int ok;
  memset(&o, 0, sizeof o);
  o.m = m;
  if (type_text == NULL)
    return diag_fail(m->diag, "OOM", NULL, "the arena limit is reached");
  ok = put_text(&o, "{") && put_string(&o, LANG_NAME) && put_text(&o, ":1,\"instances\":[");
  for (i = 0; ok && i < m->def_count; i++) {
    if (is_instance(m, &m->defs[i])) {
      ok = instance_json(&o, i, type_text, first);
      first = 0;
    }
  }
  if (!ok || !put_text(&o, "]}\n"))
    return 0;
  *text = o.data;
  *len = o.len;
  return 1;
}
