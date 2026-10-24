#include <string.h>

#include "front/lexer.h"

typedef struct {
  const char *text;
  size_t len;
  size_t pos;
  int line;
  int col;
  Token *out;
  size_t count;
  Diag *diag;
  const char *source;
  Arena *arena;
  int core_names;
} Lexer;

static const struct {
  const char *word;
  TokKind kind;
} KEYWORDS[] = {
  {"def", TOK_KW_DEF},
  {"rec", TOK_KW_REC},
  {"fun", TOK_KW_FUN},
  {"family", TOK_KW_FAMILY},
  {"axiom", TOK_KW_AXIOM},
  {"Type", TOK_KW_TYPE},
  {"Sigma", TOK_KW_SIGMA},
  {"and", TOK_KW_AND},
  {"elim", TOK_KW_ELIM},
  {"cofamily", TOK_KW_COFAMILY},
  {"cell", TOK_KW_CELL},
  {"cocell", TOK_KW_COCELL},
};

const char *tok_kind_name(TokKind kind) {
  switch (kind) {
    case TOK_EOF: return "end of input";
    case TOK_IDENT: return "a name";
    case TOK_NAT: return "a Nat literal";
    case TOK_STR: return "a string literal";
    case TOK_LPAREN: return "'('";
    case TOK_RPAREN: return "')'";
    case TOK_COLON: return "':'";
    case TOK_DEFINE: return "':='";
    case TOK_ARROW: return "'->'";
    case TOK_FATARROW: return "'=>'";
    case TOK_STAR: return "'*'";
    case TOK_BAR: return "'|'";
    case TOK_AT: return "'@'";
    case TOK_TILDE: return "'~'";
    case TOK_MEET: return "'/\\'";
    case TOK_JOIN: return "'\\/'";
    case TOK_LBRACKET: return "'['";
    case TOK_RBRACKET: return "']'";
    case TOK_COMMA: return "','";
    case TOK_KW_DEF: return "'def'";
    case TOK_KW_REC: return "'rec'";
    case TOK_KW_FUN: return "'fun'";
    case TOK_KW_FAMILY: return "'family'";
    case TOK_KW_AXIOM: return "'axiom'";
    case TOK_KW_TYPE: return "'Type'";
    case TOK_KW_SIGMA: return "'Sigma'";
    case TOK_KW_AND: return "'and'";
    case TOK_KW_ELIM: return "'elim'";
    case TOK_KW_COFAMILY: return "'cofamily'";
    case TOK_KW_CELL: return "'cell'";
    case TOK_KW_COCELL: return "'cocell'";
  }
  return "a token";
}

static int is_digit(char c) {
  return c >= '0' && c <= '9';
}

static int is_ident_start(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_ident_char(char c) {
  return is_ident_start(c) || is_digit(c) || c == '\'';
}

static char peek(const Lexer *lx, size_t ahead) {
  return lx->pos + ahead < lx->len ? lx->text[lx->pos + ahead] : '\0';
}

static void advance(Lexer *lx, size_t width) {
  for (size_t i = 0; i < width && lx->pos < lx->len; i++) {
    int newline = lx->text[lx->pos] == '\n';
    lx->line += newline;
    lx->col = newline ? 1 : lx->col + 1;
    lx->pos++;
  }
}

static Token *push(Lexer *lx, TokKind kind, size_t start, int line, int col) {
  Token *tok = &lx->out[lx->count];
  lx->count++;
  tok->kind = kind;
  tok->text = lx->text + start;
  tok->len = lx->pos - start;
  tok->nat = 0;
  tok->str = NULL;
  tok->str_len = 0;
  tok->line = line;
  tok->col = col;
  return tok;
}

/* Skips white space and `--` comments to the end of the line. */
static void skip_trivia(Lexer *lx) {
  for (;;) {
    char c = peek(lx, 0);
    int space = lx->pos < lx->len && (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    int comment = c == '-' && peek(lx, 1) == '-';
    if (!space && !comment) return;
    if (space) {
      advance(lx, 1);
      continue;
    }
    while (lx->pos < lx->len && lx->text[lx->pos] != '\n') advance(lx, 1);
  }
}

static TokKind keyword_kind(const char *text, size_t len) {
  for (size_t i = 0; i < sizeof KEYWORDS / sizeof KEYWORDS[0]; i++) {
    if (strlen(KEYWORDS[i].word) == len && memcmp(KEYWORDS[i].word, text, len) == 0) return KEYWORDS[i].kind;
  }
  return TOK_IDENT;
}

static int lex_ident(Lexer *lx) {
  size_t start = lx->pos;
  int line = lx->line;
  int col = lx->col;
  while (is_ident_char(peek(lx, 0)) || (lx->core_names && peek(lx, 0) == '.')) advance(lx, 1);
  push(lx, keyword_kind(lx->text + start, lx->pos - start), start, line, col);
  return 1;
}

/* A Nat literal. A span at or above 2^64 gets nat = UINT64_MAX and keeps
   its digits in text/len: the checker re-reads the span (S3). */
static int lex_nat(Lexer *lx) {
  size_t start = lx->pos;
  int line = lx->line;
  int col = lx->col;
  uint64_t value = 0;
  int overflow = 0;
  while (is_digit(peek(lx, 0))) {
    uint64_t digit = (uint64_t)(peek(lx, 0) - '0');
    overflow |= value > (UINT64_MAX - digit) / 10u;
    value = value * 10u + digit;
    advance(lx, 1);
  }
  if (is_ident_start(peek(lx, 0))) {
    return diag_fail(lx->diag, "LEX_CHAR", NULL, "%s:%d:%d: a letter follows a Nat literal", lx->source, lx->line, lx->col);
  }
  push(lx, TOK_NAT, start, line, col)->nat = overflow ? UINT64_MAX : value;
  return 1;
}

/* The byte that the escape `\E` stands for, or -1 for an unknown escape.
   `\x` is the hex escape; hex_byte reads its two digits. */
static int escape_byte(char e) {
  switch (e) {
    case '"': return '"';
    case '\\': return '\\';
    case 'n': return '\n';
    case 't': return '\t';
    default: return -1;
  }
}

/* The value of a hex digit, or -1. */
static int hex_digit(char h) {
  return h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
}

/* The byte of the escape `\xHH` whose x is TEXT[0], or -1 when fewer than
   two hex digits follow it. AVAIL is the byte count from TEXT. */
static int hex_byte(const char *text, size_t avail) {
  int hi = avail >= 3u ? hex_digit(text[1]) : -1;
  int lo = avail >= 3u ? hex_digit(text[2]) : -1;
  return hi < 0 || lo < 0 ? -1 : hi * 16 + lo;
}

/* The width of the escape at TEXT (TEXT[0] is the backslash), 0 for an
   unknown one. */
static size_t escape_width(const char *text, size_t avail) {
  if (avail < 2u)
    return 0;
  if (text[1] == 'x')
    return hex_byte(text + 1, avail - 1u) < 0 ? 0 : 4;
  return escape_byte(text[1]) < 0 ? 0 : 2;
}

/* UTF-8 scalar values only: no overlong forms, surrogates or code points
   above U+10FFFF. Escapes and raw source bytes obey the same rule. */
static int valid_utf8(const char *bytes, size_t len) {
  size_t i = 0;
  while (i < len) {
    unsigned char lead = (unsigned char)bytes[i];
    size_t width, j;
    if (lead < 0x80u) {
      i++;
      continue;
    }
    width = lead >= 0xc2u && lead <= 0xdfu ? 2u
      : lead >= 0xe0u && lead <= 0xefu ? 3u : lead >= 0xf0u && lead <= 0xf4u ? 4u : 0u;
    if (width == 0u || width > len - i)
      return 0;
    for (j = 1; j < width; j++)
      if ((unsigned char)bytes[i + j] < 0x80u || (unsigned char)bytes[i + j] > 0xbfu)
        return 0;
    if ((lead == 0xe0u && (unsigned char)bytes[i + 1u] < 0xa0u)
        || (lead == 0xedu && (unsigned char)bytes[i + 1u] > 0x9fu)
        || (lead == 0xf0u && (unsigned char)bytes[i + 1u] < 0x90u)
        || (lead == 0xf4u && (unsigned char)bytes[i + 1u] > 0x8fu))
      return 0;
    i += width;
  }
  return 1;
}

/* A string literal (S5, from the agent-lang lexer). Double quotes only: a
   single quote stays LEX_CHAR. The escapes are \" \\ \n \t and \xHH
   (exactly two hex digits, one byte); the decoded bytes must be UTF-8.
   A raw newline or other control byte is refused. The first pass
   checks the literal and counts its bytes; the second copies them into the
   arena. */
static int lex_str(Lexer *lx) {
  size_t start = lx->pos;
  int line = lx->line;
  int col = lx->col;
  size_t count = 0;
  advance(lx, 1);
  while (lx->pos < lx->len && peek(lx, 0) != '"') {
    unsigned char c = (unsigned char)peek(lx, 0);
    size_t width = c == '\\' ? escape_width(lx->text + lx->pos, lx->len - lx->pos) : 1u;
    if (c < 0x20u || c == 0x7fu) {
      return diag_fail(lx->diag, "LEX_STR_CONTROL", NULL, "%s:%d:%d: a string literal holds the control byte 0x%02x (write \\n for a newline)",
                       lx->source, lx->line, lx->col, (unsigned)c);
    }
    if (width == 0u && lx->pos + 1 < lx->len) {
      return diag_fail(lx->diag, "LEX_STR_ESCAPE", NULL, "%s:%d:%d: an unknown escape in a string literal (the escapes are \\\" \\\\ \\n \\t \\xHH)",
                       lx->source, lx->line, lx->col);
    }
    advance(lx, width == 0u ? 2u : width);
    count++;
  }
  if (lx->pos >= lx->len) {
    return diag_fail(lx->diag, "LEX_STR_OPEN", NULL, "%s:%d:%d: a string literal has no closing quote", lx->source, line, col);
  }
  advance(lx, 1);
  char *bytes = arena_alloc(lx->arena, count + 1);
  if (bytes == NULL) return diag_fail(lx->diag, "OOM", NULL, "%s: no memory for a string literal", lx->source);
  size_t at = start + 1;
  for (size_t i = 0; i < count; i++) {
    size_t width = lx->text[at] == '\\' ? escape_width(lx->text + at, lx->len - at) : 1u;
    bytes[i] = width == 4u ? (char)hex_byte(lx->text + at + 1, 3) : width == 2u ? (char)escape_byte(lx->text[at + 1]) : lx->text[at];
    at += width;
  }
  bytes[count] = '\0';
  if (!valid_utf8(bytes, count))
    return diag_fail(lx->diag, "LEX_STR_UTF8", NULL, "%s:%d:%d: a string literal is not valid UTF-8", lx->source, line, col);
  Token *tok = push(lx, TOK_STR, start, line, col);
  tok->str = bytes;
  tok->str_len = count;
  return 1;
}

static int lex_symbol(Lexer *lx) {
  size_t start = lx->pos;
  int line = lx->line;
  int col = lx->col;
  char c = peek(lx, 0);
  char next = peek(lx, 1);
  TokKind kind = TOK_EOF;
  size_t width = 1;
  switch (c) {
    case '(': kind = TOK_LPAREN; break;
    case ')': kind = TOK_RPAREN; break;
    case '*': kind = TOK_STAR; break;
    case '|': kind = TOK_BAR; break;
    case '@': kind = TOK_AT; break;
    case '~': kind = TOK_TILDE; break;
    case '[': kind = TOK_LBRACKET; break;
    case ']': kind = TOK_RBRACKET; break;
    case ',': kind = TOK_COMMA; break;
    case '/':
      kind = next == '\\' ? TOK_MEET : TOK_EOF;
      width = 2;
      break;
    case '\\':
      kind = next == '/' ? TOK_JOIN : TOK_EOF;
      width = 2;
      break;
    case ':':
      kind = next == '=' ? TOK_DEFINE : TOK_COLON;
      width = next == '=' ? 2 : 1;
      break;
    case '-':
      kind = next == '>' ? TOK_ARROW : TOK_EOF;
      width = 2;
      break;
    case '=':
      kind = next == '>' ? TOK_FATARROW : TOK_EOF;
      width = 2;
      break;
    default: break;
  }
  if (kind == TOK_EOF) {
    return diag_fail(lx->diag, "LEX_CHAR", NULL, "%s:%d:%d: unexpected byte 0x%02x", lx->source, line, col, (unsigned)(unsigned char)c);
  }
  advance(lx, width);
  push(lx, kind, start, line, col);
  return 1;
}

static int lex_source_impl(Arena *arena, const char *source, const char *text, size_t len, TokenList *out, Diag *diag, int core_names) {
  /* Each token takes at least one byte, and one EOF token follows. */
  Token *tokens = arena_alloc(arena, (len + 1) * sizeof(Token));
  if (tokens == NULL) return diag_fail(diag, "OOM", NULL, "%s: no memory for the tokens", source);
  Lexer lx = {
    .text = text, .len = len, .pos = 0, .line = 1, .col = 1,
    .out = tokens, .count = 0, .diag = diag, .source = source,
    .arena = arena, .core_names = core_names,
  };
  for (;;) {
    skip_trivia(&lx);
    if (lx.pos >= lx.len) break;
    char c = peek(&lx, 0);
    int ok = is_ident_start(c) ? lex_ident(&lx) : is_digit(c) ? lex_nat(&lx) : c == '"' ? lex_str(&lx) : lex_symbol(&lx);
    if (!ok) return 0;
  }
  push(&lx, TOK_EOF, lx.pos, lx.line, lx.col);
  out->items = tokens;
  out->count = lx.count;
  return 1;
}

int lex_source(Arena *arena, const char *source, const char *text, size_t len, TokenList *out, Diag *diag) {
  return lex_source_impl(arena, source, text, len, out, diag, 0);
}

int lex_core_source(Arena *arena, const char *source, const char *text, size_t len, TokenList *out, Diag *diag) {
  return lex_source_impl(arena, source, text, len, out, diag, 1);
}
