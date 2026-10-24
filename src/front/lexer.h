/* Tokens of the DSL. SHARED by the tcc kits. */
#ifndef LANG_FRONT_LEXER_H
#define LANG_FRONT_LEXER_H

#include "front/base.h"

typedef enum {
  TOK_EOF,
  TOK_IDENT,
  TOK_NAT,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_COLON,
  TOK_DEFINE,   /* := */
  TOK_ARROW,    /* -> */
  TOK_FATARROW, /* => */
  TOK_STAR,
  TOK_BAR,
  TOK_AT,       /* @ */
  TOK_TILDE,    /* ~ */
  TOK_MEET,     /* /\ */
  TOK_JOIN,     /* \/ */
  TOK_LBRACKET, /* [ */
  TOK_RBRACKET, /* ] */
  TOK_COMMA,
  TOK_KW_DEF,
  TOK_KW_REC,
  TOK_KW_FUN,
  TOK_KW_FAMILY,
  TOK_KW_AXIOM,
  TOK_KW_TYPE,
  TOK_KW_SIGMA,
  TOK_KW_AND,
  TOK_KW_ELIM,
  TOK_KW_COFAMILY,
  TOK_KW_CELL,
  TOK_KW_COCELL
} TokKind;

typedef struct {
  TokKind kind;
  const char *text;
  size_t len;
  uint64_t nat;
  int line;
  int col;
} Token;

/* The last token is TOK_EOF. */
typedef struct {
  const Token *items;
  size_t count;
} TokenList;

/* SOURCE names the input in diagnostics. Returns 1, or 0 after a diagnostic. */
int lex_source(Arena *arena, const char *source, const char *text, size_t len, TokenList *out, Diag *diag);
/* Kernel readback only: also accepts generated dotted family and field
   names. User sources always use lex_source. */
int lex_core_source(Arena *arena, const char *source, const char *text, size_t len, TokenList *out, Diag *diag);
const char *tok_kind_name(TokKind kind);

#endif
