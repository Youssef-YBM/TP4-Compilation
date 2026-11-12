/*
 * Lexical analyzer for a small French-keyword language.
 *
 * Tokens:
 *   SI, SINON, ALORS      keywords (case-insensitive)
 *   OPREL                 <  <=  <>  >  >=  =
 *   ID                    letter (letter | digit)*
 *   NB                    digit+ [. digit+] [(e|E) [+|-] digit+]
 *
 * Usage: lexer [-v] [input.txt [sortie_analyseur.txt]]
 *   -v  verbose: print "TOKEN 'lexeme' (line N)" one per line
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TOK_SI, TOK_SINON, TOK_ALORS, TOK_OPREL, TOK_ID, TOK_NB, TOK_EOF } Token;

static const char *TOKEN_NAMES[] = { "SI", "SINON", "ALORS", "OPREL", "ID", "NB", "EOF" };

typedef struct {
    const char *src;  
    size_t      len;
    size_t      pos;
    int         line;
    int         errors;
} Lexer;// Lexer est une structure qui contient les informations nécessaires pour analyser le code source. Elle contient un pointeur vers la chaîne de caractères source, la longueur de cette chaîne, la position actuelle dans la chaîne, le numéro de ligne actuel et le nombre d'erreurs rencontrées lors de l'analyse lexicale.


static int is_letter(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int is_digit(int c)  { return c >= '0' && c <= '9'; }
static int is_alnum(int c)  { return is_letter(c) || is_digit(c); }
static int to_lower(int c)  { return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c; }

/* ---------- lexer helpers ---------- */

static int peek_at(const Lexer *lx, size_t offset){ // sert pour regarder le caractère à l'offset sans avancer la position
    size_t i = lx->pos + offset;
    return i < lx->len ? (unsigned char)lx->src[i] : EOF;
}

static void skip_blanks(Lexer *lx){
    int c;
    while ((c = peek_at(lx, 0)) == ' ' || c == '\t' || c == '\r' || c == '\n') {
        if (c == '\n') lx->line++;
        lx->pos++;
    }
}

/* Case-insensitive comparison of src[start, start+n) with a lowercase keyword. */
static int word_equals(const Lexer *lx, size_t start, size_t n, const char *kw){
    if (strlen(kw) != n) return 0;
    for (size_t i = 0; i < n; i++)
        if (to_lower((unsigned char)lx->src[start + i]) != kw[i]) return 0;
    return 1;
}

/* Scans digits starting at offset; returns the offset just after them. */
static size_t scan_digits(const Lexer *lx, size_t off)
{
    while (is_digit(peek_at(lx, off))) off++;
    return off;
}

/*
 * Scans a number. Optional parts (fraction, exponent) are only consumed if
 * they are complete, so "12.", "12.x", "3e" and "3e+" yield NB "12"/"3"
 * and leave the rest for the next token. No ungetc needed: we just look ahead.
 */
static size_t scan_number(const Lexer *lx)
{
    size_t off = scan_digits(lx, 0);

    if (peek_at(lx, off) == '.' && is_digit(peek_at(lx, off + 1)))
        off = scan_digits(lx, off + 1);

    int e = peek_at(lx, off);
    if (e == 'e' || e == 'E') {
        size_t o = off + 1;
        int s = peek_at(lx, o);
        if (s == '+' || s == '-') o++;
        if (is_digit(peek_at(lx, o)))
            off = scan_digits(lx, o);
    }
    return off;
}

/*
 * Returns the next token. [*start, *end) is its lexeme in lx->src.
 * Unknown characters are reported on stderr and skipped.
 */
static Token next_token(Lexer *lx, size_t *start, size_t *end)
{
    for (;;) {
        skip_blanks(lx);

        int c = peek_at(lx, 0);
        if (c == EOF) return TOK_EOF;

        *start = lx->pos;

        if (is_letter(c)) {
            size_t n = 1;
            while (is_alnum(peek_at(lx, n))) n++;
            lx->pos += n;
            *end = lx->pos;

            if (word_equals(lx, *start, n, "si"))    return TOK_SI;
            if (word_equals(lx, *start, n, "sinon")) return TOK_SINON;
            if (word_equals(lx, *start, n, "alors")) return TOK_ALORS;
            return TOK_ID;
        }

        if (is_digit(c)) {
            lx->pos += scan_number(lx);
            *end = lx->pos;
            return TOK_NB;
        }

        if (c == '<') {
            int d = peek_at(lx, 1);
            lx->pos += (d == '>' || d == '=') ? 2 : 1;
            *end = lx->pos;
            return TOK_OPREL;
        }
        if (c == '>') {
            lx->pos += (peek_at(lx, 1) == '=') ? 2 : 1;
            *end = lx->pos;
            return TOK_OPREL;
        }
        if (c == '=') {
            lx->pos++;
            *end = lx->pos;
            return TOK_OPREL;
        }

        fprintf(stderr, "Unknown character '%c' (0x%02X) in line %d\n", c, c, lx->line);
        lx->errors++;
        lx->pos++;                       /* skip it and keep scanning */
    }
}

/* ---------- I/O ---------- */

static char *read_file(const char *path, size_t *len_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    size_t cap = 4096, len = 0;
    char *buf = malloc(cap + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t n;
    while ((n = fread(buf + len, 1, cap - len, f)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap + 1);
            if (!tmp) { free(buf); fclose(f); return NULL; }
            buf = tmp;
        }
    }
    int failed = ferror(f);
    fclose(f);
    if (failed) { free(buf); return NULL; }

    buf[len] = '\0';
    *len_out = len;
    return buf;
}

int main(int argc, char **argv)
{
    int verbose = 0;
    const char *in_path = "input.txt", *out_path = "sortie_analyseur.txt";

    int i = 1;
    if (i < argc && strcmp(argv[i], "-v") == 0) { verbose = 1; i++; }
    if (i < argc) in_path = argv[i++];
    if (i < argc) out_path = argv[i++];

    size_t len;
    char *src = read_file(in_path, &len);
    if (!src) {
        fprintf(stderr, "Error: could not read '%s'\n", in_path);
        return EXIT_FAILURE;
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Error: could not open '%s' for writing\n", out_path);
        free(src);
        return EXIT_FAILURE;
    }

    Lexer lx = { src, len, 0, 1, 0 };
    int last_line = 1;
    size_t start = 0, end = 0;
    Token t;

    while ((t = next_token(&lx, &start, &end)) != TOK_EOF) {
        if (verbose) {
            fprintf(out, "%-5s '%.*s' (line %d)\n",
                    TOKEN_NAMES[t], (int)(end - start), src + start, lx.line);
        } else {
            if (lx.line > last_line) {   /* keep the source's line structure */
                fputc('\n', out);
                last_line = lx.line;
            }
            fprintf(out, "%s ", TOKEN_NAMES[t]);
        }
    }
    if (!verbose) fputc('\n', out);

    int write_failed = ferror(out);
    if (fclose(out) != 0) write_failed = 1;
    free(src);

    if (write_failed) {
        fprintf(stderr, "Error: failed writing '%s'\n", out_path);
        return EXIT_FAILURE;
    }
    return lx.errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
