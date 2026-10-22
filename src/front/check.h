/* The type checker, the refusals R1-R4 and the eval command. SHARED by the
   tcc kits. */
#ifndef LANG_FRONT_CHECK_H
#define LANG_FRONT_CHECK_H

#include "front/core.h"

#define ENTRY_PARAMS_MAX 16u

/* An entry: a program definition of a first-order type (Nat, or Fin n at a
   literal n that is not 0), or a function of first-order arguments to a
   first-order result. A parameter bound is 0 for Nat and n for Fin n: the
   argument is a decimal below n. */
typedef struct {
  uint32_t param_count;
  uint64_t param_bound[ENTRY_PARAMS_MAX];
} Entry;

/* Checks the domain declarations, then the program, into M. Returns 1, or 0
   after a diagnostic. */
int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag);
/* Returns 1 when TYPE is the type of an entry. */
int entry_of(Machine *m, const Value *type, Entry *out);
/* Evaluates the definition NAME on ARGS and prints the result. Returns 0, 1
   after a diagnostic, or 2 after an EVAL_ARGS diagnostic. */
int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err);

#endif
