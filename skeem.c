/*
 * Skeem - a small interpreter for a small subset of Scheme
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#include "refcnt.h"
#include "skeem.h"

#define HASH_SIZE 32

#ifdef NDEBUG
#  define MEMCHECK(p) if(!p) abort() /* You're doomed */
#else
#  define MEMCHECK(p) assert(p)
#endif

/* Anonymous structs and unions are not part of the C standard, but they are
so useful that I can't get myself to remove them */
typedef struct SkObj {
    enum {SYMBOL, VALUE, CONS, CFUN, TRUE, FALSE, LAMBDA, CDATA, ERROR} type;
    union {
        char *value;
        sk_cfun func;
        struct {
           struct SkObj *car, *cdr; /* for sk_cons cells */
        };
        struct {
           struct SkObj *args, *body; /* for lambdas */
        };
        struct {
            void *cdata; ref_dtor cdtor;
        };
    };
} SkObj;

typedef struct hash_element {
    char *name;
    SkObj *ex;
    struct hash_element *next;
} hash_element;

typedef struct SkEnv {
    struct hash_element *table[HASH_SIZE];
    struct SkEnv *parent;
} SkEnv;

static void env_dtor(SkEnv *env) {
    int i;
    for(i = 0; i < HASH_SIZE; i++) {
        while(env->table[i]) {
            hash_element* v = env->table[i];
            env->table[i] = v->next;
            rc_release(v->ex);
            free(v->name);
            free(v);
        }
        env->table[i] = NULL;
    }
    rc_release(env->parent);
}

SkEnv *sk_env_create(SkEnv *parent) {
    int i;
    SkEnv *env = rc_alloc(sizeof *env);
    rc_set_dtor(env, (ref_dtor)env_dtor);
    for(i = 0; i < HASH_SIZE; i++)
        env->table[i] = NULL;
    env->parent = parent ? rc_retain(parent) : NULL;
    return env;
}

static unsigned int hash(const char *s) {
    unsigned int h = 5381;
    /* DJB hash, XOR variant */
    for(;s[0];s++)
        h = ((h << 5) + h) ^ s[0];
    return h % HASH_SIZE;
}

SkObj *sk_env_put(SkEnv *env, const char *name, SkObj *e) {
    unsigned int h = hash(name);

    hash_element *v, *f = NULL;
    if(!env)
        return NULL;

    for(v = env->table[h]; v; v = v->next)
        if(!strcmp(v->name, name)) {
            f = v;
            break;
        }

    if(f)
        rc_release(f->ex);
    else {
        f = malloc(sizeof *v);
        f->name = strdup(name);
        f->next = env->table[h];
        env->table[h] = f;
    }
    f->ex = e;
    return e;
}

static hash_element *env_findg_r(SkEnv *env, const char *name, unsigned int h) {
    hash_element *v;
    if(!env)
        return NULL;
    for(v = env->table[h]; v; v = v->next)
        if(!strcmp(v->name, name))
            return v;
    return env_findg_r(env->parent, name, h);
}

SkObj *sk_env_get(SkEnv *env, const char *name) {
    unsigned int h = hash(name);
    hash_element* v = env_findg_r(env, name, h);
    if(v)
        return v->ex;
    return sk_errorf("no such variable '%s'", name);
}

static void SkExpr_dtor(SkObj *e) {
    switch(e->type) {
        case ERROR:
        case SYMBOL:
        case VALUE: free(e->value); break;
        case CONS: rc_release(e->car); rc_release(e->cdr); break;
        case LAMBDA: rc_release(e->args); rc_release(e->body); break;
        case CDATA: if(e->cdtor) e->cdtor(e->cdata); break;
        default: break;
    }
}

#ifdef NDEBUG
SkObj *sk_symbol(const char *sk_value) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_symbol_(const char *sk_value, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = SYMBOL;
    e->value = strdup(sk_value);
    return e;
}

#ifdef NDEBUG
SkObj *sk_value(const char *val) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_value_(const char *val, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = VALUE;
    e->value = strdup(val);
    return e;
}

#ifdef NDEBUG
SkObj *sk_value_o(char *val) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_value_o_(char *val, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = VALUE;
    e->value = val;
    return e;
}

#ifdef NDEBUG
SkObj *sk_error(const char *val) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_error_(const char *val, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = ERROR;
    e->value = strdup(val);
    return e;
}

SkObj *sk_errorf(const char *fmt, ...) {
    va_list arg;
    char buffer[512];
    va_start(arg, fmt);
    vsnprintf (buffer, sizeof(buffer)-1, fmt, arg);
    va_end(arg);
    return sk_error(buffer);
}

#ifdef NDEBUG
SkObj *sk_boolean(int val) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_boolean_(int val, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = val ? TRUE : FALSE;
    return e;
}

#define RESULT_SIZE 64
#define PRECISION   30
#ifdef NDEBUG
SkObj *sk_number(double n) {
    char result[RESULT_SIZE];
    snprintf(result, sizeof result - 1, "%.*g", PRECISION, n);
    return sk_value(result);
}
#else
SkObj *sk_number_(double n, const char *file, int line) {
    char result[RESULT_SIZE];
    snprintf(result, sizeof result - 1, "%.*g", PRECISION, n);
    return sk_value_(result, file, line);
}
#endif

#ifdef NDEBUG
SkObj *sk_cons(SkObj *car, SkObj *sk_cdr) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_cons_(SkObj *car, SkObj *sk_cdr, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = CONS;
    e->car = car;
    e->cdr = sk_cdr;
    return e;
}

#ifdef NDEBUG
SkObj *sk_lambda(SkObj *args, SkObj *body) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_lambda_(SkObj *args, SkObj *body, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = LAMBDA;
    e->args = args;
    e->body = body;
    return e;
}

#ifdef NDEBUG
SkObj *sk_cfun(sk_cfun func) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_cfun_(sk_cfun func, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = CFUN;
    e->func = func;
    return e;
}

#ifdef NDEBUG
SkObj *sk_cdata(void *cdata, ref_dtor dtor) {
    SkObj *e = rc_alloc(sizeof *e);
#else
SkObj *sk_cdata_(void *cdata, ref_dtor dtor, const char *file, int line) {
    SkObj *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)SkExpr_dtor);
    e->type = CDATA;
    e->cdata = cdata;
    e->cdtor = dtor;
    return e;
}

void *sk_get_cdata(SkObj *e) {
    if(!e || e->type != CDATA) return NULL;
    return e->cdata;
}

ref_dtor sk_get_cdtor(SkObj *e) {
    if(!e || e->type != CDATA) return NULL;
    return e->cdtor;
}

/* O(1) way to append items to a list,
but it needs a separate pointer to track the last item in the list.

Also, be careful to only use it when constructing new lists as existing
lists are supposed to be immutable.
*/
#ifdef NDEBUG
static void list_append1(SkObj **list, SkObj *a, SkObj **last) {
    SkObj *item = sk_cons(NULL, NULL);
#else
#  define list_append1(l, a, ls) list_append1_(l, a, ls, __FILE__, __LINE__)
static void list_append1_(SkObj **list, SkObj *a, SkObj **last, const char *file, int line) {
    SkObj *item = sk_cons_(NULL, NULL, file, line);
#endif
    item->car = a;
    if(!*last)
        *list = item;
    else
        (*last)->cdr = item;
    *last = item;
}

int sk_equal(SkObj *a, SkObj *b) {
    if(!a || !b)
        return !a && !b;
    else if(a->type != b->type)
        return 0;
    else switch(a->type) {
        case CFUN: return a->func == b->func;
        case CDATA: return a->cdata == b->cdata && a->cdtor == b->cdtor;
        case ERROR: return 0;
        case SYMBOL: return !strcmp(a->value, b->value);
        case VALUE: return !strcmp(a->value, b->value);
        case TRUE:
        case FALSE: return 1;
        case CONS: return sk_equal(a->car, b->car) && sk_equal(a->cdr, b->cdr);
        case LAMBDA: return sk_equal(a->args, b->args) && sk_equal(a->body, b->body);
    }
    return 1;
}

static char *buffer_appendn(char **buf, int *n, int *a, const char *s, int len) {
    assert(buf && n && a && s);
    int nlen;
    if(!*buf) {
        *a = 32;
        while(*a < len + 1) *a <<= 1;
        *buf = malloc(*a);
        MEMCHECK(*buf);
        strncpy(*buf, s, len);
        (*buf)[len] = '\0';
        *n = len;
        return *buf;
    }
    nlen = *n + len;
    if(*a < nlen + 1) {
        while(*a < nlen + 1) *a <<= 1;
        *buf = realloc(*buf, *a);
        MEMCHECK(*buf);
    }
    strncpy(*buf + *n, s, len);
    *n = nlen;
    (*buf)[nlen] = '\0';
    return *buf;
}

static char *buffer_append(char **buf, int *n, int *a, const char *s) {
    return buffer_appendn(buf, n, a, s, strlen(s));
}

static char *buffer_appendf(char **buf, int *n, int *a, const char *fmt, ...) {
    va_list arg;
    char buffer[512];
    va_start(arg, fmt);
    vsnprintf (buffer, sizeof(buffer)-1, fmt, arg);
    va_end(arg);
    return buffer_append(buf, n, a, buffer);
}

static void serialize_r(char **buf, int *n, int *a, SkObj *e) {
    if(!e)
        buffer_append(buf, n, a, "'() ");
    else switch(e->type) {
        case CFUN: buffer_appendf(buf, n, a, "#<cfun:%p> ", e->func); break;
        case CDATA: buffer_appendf(buf, n, a, "<cdata:%p;%p>", e->cdtor, e->cdata); break;
        case ERROR: buffer_appendf(buf, n, a, "#<error:%s> ", e->value); break;
        case SYMBOL: buffer_appendf(buf, n, a, "%s ", e->value); break;
        case TRUE: buffer_append(buf, n, a, "#t "); break;
        case FALSE: buffer_append(buf, n, a, "#f "); break;
        case VALUE: {
            buffer_appendf(buf, n, a, "\"");
            char *s = e->value;
            int i = 0;
            while(s[i]) {
                if(s[i] < ' ') {
                    buffer_appendn(buf, n, a, s, i);
                    switch(s[i]) {
                    case '\n': buffer_appendn(buf, n, a, "\\n", 2); break;
                    case '\r': buffer_appendn(buf, n, a, "\\r", 2); break;
                    case '\t': buffer_appendn(buf, n, a, "\\t", 2); break;
                    case '\b': buffer_appendn(buf, n, a, "\\b", 2); break;
                    case '\a': buffer_appendn(buf, n, a, "\\a", 2); break;
                    default: buffer_appendf(buf, n, a, "\\%d", s[i]); break;
                    }
                    s += i + 1;
                    i = 0;
                } else if(strchr("\\\"", s[i])) {
                    buffer_appendn(buf, n, a, s, i);
                    buffer_appendf(buf, n, a, "\\%c", s[i]);
                    s += i + 1;
                    i = 0;
                } else
                    i++;
            }
            buffer_appendn(buf, n, a, s, i);
            buffer_appendf(buf, n, a, "\" ");
        } break;
        case CONS:
            buffer_append(buf, n, a, "( ");
            for(;;) {
                serialize_r(buf, n, a, e->car);
                if(e->cdr) {
                    if(e->cdr->type == CONS)
                        e = e->cdr;
                    else {
                        buffer_append(buf, n, a, ". ");
                        serialize_r(buf, n, a, e->cdr);
                        break;
                    }
                } else
                    break;
            }
            buffer_appendf(buf, n, a, ") ");
            break;
        case LAMBDA:
            buffer_append(buf, n, a, "(lambda ");
            serialize_r(buf, n, a, e->args);
            buffer_append(buf, n, a, " ");
            serialize_r(buf, n, a, e->body);
            buffer_append(buf, n, a, ") ");
            break;
    }
}

char *sk_serialize(SkObj *e) {
    int n = 0, a = 256;
    char *buf = malloc(a);
    MEMCHECK(buf);
    serialize_r(&buf, &n, &a, e);
    return buf;
}

int sk_check_numeric(const char *c) {
    /*return isdigit(c[0]) || (strchr("+-", c[0]) && isdigit(c[1]));*/
    int ds = 0, de = 0;
    if(strchr("+-", *c))
        c++;
    if(!*c) return 0;
    while(isdigit(*c) || (*c == '.' && !ds++) || ((*c == 'e' || *c == 'E') && !de++)) {
        if((*c == 'e' || *c == 'E') && (c[1] == '-' || c[1] == '+'))
            c++;
        c++;
    }
    return !*c;
}

const char *sk_get_text(SkObj *e) {
    if(!e) return "";
    if(e->type == TRUE)
        return "true";
    else if(e->type == FALSE)
        return "false";
    return (e->type == VALUE || e->type == SYMBOL || e->type == ERROR) ? e->value : "";
}

int sk_is_null(SkObj *e) {
    return e == NULL;
}

int sk_is_symbol(SkObj *e) {
    return e && e->type == SYMBOL;
}

int sk_is_cons(SkObj *e) {
    return e && e->type == CONS;
}

int sk_is_error(SkObj *e) {
    return e && e->type == ERROR;
}

int sk_is_boolean(SkObj *e) {
    return e && (e->type == FALSE || e->type == TRUE);
}

int sk_is_true(SkObj *e) {
    return e && e->type != FALSE;
}

int sk_is_procedure(SkObj *e) {
    return e && (e->type == CFUN || e->type == LAMBDA);
}

int sk_is_value(SkObj *e) {
    return e && e->type == VALUE;
}

int sk_is_number(SkObj *e) {
    return e && e->type == VALUE && sk_check_numeric(e->value);
}

int sk_is_cdata(SkObj *e) {
    return e && e->type == CDATA;
}

int sk_is_list(SkObj *e) {
    for(; e; e = e->cdr)
        if(e->type != CONS)
            return 0;
    return 1;
}

enum scan_result {
 SCAN_ERROR = -1,
 SCAN_END = 0,
 SCAN_SYMBOL = 1,
 SCAN_VALUE,
 SCAN_TRUE,
 SCAN_FALSE
};

static int scan(const char *in, char tok[], size_t n, const char **rem) {
    size_t i = 0;
    if(!in)
        in = *rem;
restart:
    if(!in)
        return SCAN_END;
    while(isspace(*in))
        in++;

    if (*in == '\0') {
        *rem = in;
        return SCAN_END;
    } else if(*in == ';') {
        while(*in != '\n') {
            if(*in == '\0') {
                *rem = in;
                return SCAN_END;
            }
            in++;
        }
        goto restart;
    } else if (strchr("()[]'", *in)) {
        tok[0] = *in;
        tok[1] ='\0';
        *rem =  ++in;
        return tok[0];
    } else if (*in == '\"') {
        ++in;
        while(*in != '\"') {
            if(!*in) {
                snprintf(tok, n, "unterminated string constant");
                return SCAN_ERROR;
            } else if(i == n) {
                snprintf(tok, n, "token too long");
                return SCAN_ERROR;
            }
            if(*in == '\\') {
                ++in;
                if(isdigit(*in)) {
                    int v = *in - '0';
                    while(isdigit(*++in))
                        v = v * 10 + (*in - '0');
                    tok[i++] = v & 0x7F;
                } else {
                    switch(*in) {
                        case '\0': snprintf(tok, n, "unterminated string constant"); return SCAN_ERROR;
                        case 'n' : tok[i++] = '\n'; break;
                        case 'r' : tok[i++] = '\r'; break;
                        case 't' : tok[i++] = '\t'; break;
                        case 'b' : tok[i++] = '\b'; break;
                        case 'a' : tok[i++] = '\a'; break;
                        default : tok[i++] = *in; break;
                    }
                    in++;
                }
            } else
                tok[i++] = *(in++);
        }
        tok[i] = '\0';
        *rem = ++in;
        return SCAN_VALUE;
    } else if(!isprint(*in)) {
        snprintf(tok, n, "bad character in input stream (%d)", *in);
        return SCAN_ERROR;
    } else {
        while(isgraph(*in) && !strchr("])", *in)) {
            if(i == n) {
                snprintf(tok, n, "token too long");
                return SCAN_ERROR;
            }
            tok[i++] = *in++;
        }
        tok[i] = '\0';
        *rem = in;

        if(sk_check_numeric(tok)) return SCAN_VALUE;
        if(tok[0] == '#' && !tok[2]) {
            if(tolower(tok[1]) == 't') return SCAN_TRUE;
            if(tolower(tok[1]) == 'f') return SCAN_FALSE;
        }
        for(i = 0; tok[i]; i++)
            tok[i] = tolower(tok[i]);
        return SCAN_SYMBOL;
    }
}

#define TOKEN_SIZE 1024
typedef struct {
    int sym;
    const char *text;
    char tok[TOKEN_SIZE], next_tok[TOKEN_SIZE];
} Parser;

static void nextsym(Parser *p) {
    memcpy(p->tok, p->next_tok, sizeof p->tok);
    p->sym = scan(NULL, p->next_tok, sizeof p->next_tok, &p->text);
}

static int accept(Parser *p, int s) {
    if(p->sym == s) {
        nextsym(p);
        return 1;
    }
    return 0;
}

static SkObj *parse0(Parser *p) {
    if(accept(p, SCAN_ERROR))
        return sk_error(p->tok);
    else if(accept(p, SCAN_END))
        return NULL;
    else if(accept(p, SCAN_SYMBOL))
        return sk_symbol(p->tok);
    else if(accept(p, SCAN_VALUE))
        return sk_value(p->tok);
    else if(accept(p, SCAN_TRUE))
        return sk_boolean(1);
    else if(accept(p, SCAN_FALSE))
        return sk_boolean(0);
    else if(accept(p, '(') || accept(p, '[')) {
        SkObj *list = NULL, *last = NULL;
        char term = p->tok[0] == '(' ? ')' : ']';

        while(!accept(p, term)) {
            if(accept(p, SCAN_ERROR)) {
                rc_release(list);
                return sk_error(p->tok);
            } else if(accept(p, SCAN_END)) {
                rc_release(list);
                return sk_errorf("expected '%c'", term);
            }

            SkObj *e = parse0(p);
            if(sk_is_error(e)) {
                rc_release(list);
                return e;
            }

            list_append1(&list, e, &last);
        }
        return list;
    } else if(accept(p, '\'')) {
        SkObj *e = parse0(p);
        if(sk_is_error(e))
            return e;
        return sk_cons(sk_symbol("quote"), sk_cons(e, NULL));
    } else if(accept(p, ')') || accept(p, ']'))
        return sk_errorf("mismatched '%c'", p->tok[0]);

    return sk_error("unhandled token type");
}

SkObj *sk_parse(const char *text) {
    Parser p;
    p.text = text;
    p.next_tok[0] = '\0';
    nextsym(&p);
    return parse0(&p);
}

SkObj *parse_stmts(const char *text) {
    Parser p;
    SkObj *root = sk_cons(sk_symbol("begin"), NULL), *last = root;

    p.text = text;
    p.next_tok[0] = '\0';
    nextsym(&p);

    for(;;) {
        if(accept(&p, SCAN_END)) break;
        SkObj * e = parse0(&p);
        if(sk_is_error(e)) {
            rc_release(root);
            return e;
        }
        list_append1(&root, e, &last);
    }
    return root;
}

static SkEnv *get_global(SkEnv *env) {
    while(env->parent)
        env = env->parent;
    return env;
}

SkObj *sk_car(SkObj *e) {
    if(!e || e->type != CONS) return NULL;
    return e->car;
}

SkObj *sk_cdr(SkObj *e) {
    if(!e || e->type != CONS) return NULL;
    return e->cdr;
}

int sk_length(SkObj *e) {
    int count = 0;
    for(; e && e->type == CONS; e = e->cdr) count++;
    return count;
}

static SkObj *bind_args(SkEnv *env, SkObj *e) {
    assert(e->type == CONS);
    SkObj *args = NULL, *last = NULL;
    for(; e; e = e->cdr) {
        SkObj *arg = sk_eval(env, e->car);
        if(sk_is_error(arg)) {
            rc_release(args);
            return arg;
        }
        list_append1(&args, arg, &last);
    }
    return args;
}

static int valid_lambda(SkObj *l) {
    if(l->type != LAMBDA) return 0;
    if(!sk_is_null(l->args)) {
        SkObj *e;
        for(e = l->args; e; e = e->cdr)
            if(e->type != CONS || e->car->type != SYMBOL)
                return 0;
    }
    return sk_is_list(l->body);
}

SkObj *sk_eval(SkEnv *env, SkObj *e) {
    SkObj *result = NULL, *args = NULL;
    SkEnv *new_env = NULL;

    assert(env);
    for(;;) {
        if(result) {
            rc_release(result);
            result = NULL;
        }
        if(!e)
            result = NULL;
        else if(e->type == SYMBOL) {
            SkObj *r = sk_env_get(env, sk_get_text(e));
            result = sk_is_error(r) ? r : rc_retain(r); /* don't retain errors */
        } else if(e->type == CONS) {
            const char *what = sk_get_text(e->car);
            if(!sk_is_list(e)) {
                result = sk_errorf("bad %s", what[0] ? what : "list");
                break;
            }
            if(!strcmp(what, "define") || !strcmp(what, "set!")) {
                if(sk_length(e) != 3) {
                    result = sk_errorf("bad %s", what);
                    break;
                }
                e = e->cdr;

                const char *varname;
                if(sk_is_list(e->car)) {
                    /* `(define (f a b c) (body))` form */
                    SkObj *f = e->car, *p = f->cdr;
                    if(sk_length(f) < 1) {
                        result = sk_error("define lambda needs function name");
                        break;
                    }
                    varname = sk_get_text(f->car);

                    SkObj *body = sk_cons(sk_symbol("begin"), e->cdr ? rc_retain(e->cdr) : NULL);

                    p = p ? rc_retain(p) : NULL;

                    result = sk_lambda(p, body);

                    if(!valid_lambda(result)) {
                        rc_release(result);
                        result = sk_error("invalid lambda define");
                        break;
                    }
                } else {
                    /* `(define v expr)` form */
                    varname = sk_get_text(e->car);
                    result = sk_eval(env, e->cdr->car);
                    if(sk_is_error(result))
                        break;
                }
                SkEnv *tgt_env = env;

                if(result) rc_retain(result);

                if(!strcmp(what, "define"))
                    tgt_env = get_global(tgt_env);

                sk_env_put(tgt_env, varname, result);

            } else if(!strcmp(what, "let")) {
                if(sk_length(e) < 3 || !sk_is_list(e->cdr->car)) {
                    result = sk_error("bad let");
                    break;
                }
                SkObj *a = e->cdr->car, *b = e->cdr->cdr;

                SkEnv *o = new_env;
                new_env = sk_env_create(env);
                rc_release(o);

                for(; a; a = a->cdr) {
                    if(!sk_is_list(a->car) || sk_length(a->car) != 2
                        || !sk_is_symbol(a->car->car)) {
                        result = sk_error("bad clause in 'let'");
                        goto end_let;
                    }
                    const char *name = sk_get_text(a->car->car);
                    SkObj *v = sk_eval(env, a->car->cdr->car);
                    if(sk_is_error(v) && (result = v))
                        goto end_let;
                    sk_env_put(new_env, name, v);
                }
                for(; b && b->cdr; b = b->cdr) {
                    rc_release(result);
                    result = sk_eval(new_env, b->car);
                    if(sk_is_error(result))
                        goto end_let;
                }
                if(b) {
                    e = b->car;
                    env = new_env;
                    continue;
                }
end_let:
                rc_release(new_env);
                new_env = NULL;

            } else if(!strcmp(what, "lambda")) {
                if(sk_length(e) < 3) {
                    result = sk_error("bad lambda");
                    break;
                }
                e = e->cdr;
                SkObj *body = sk_cons(sk_symbol("begin"), rc_retain(e->cdr));

                result = sk_lambda(e->car ? rc_retain(e->car) : NULL, body);

                if(!valid_lambda(result)) {
                    rc_release(result);
                    result = sk_error("invalid lambda");
                    break;
                }
            } else if(!strcmp(what, "if")) {
                if(sk_length(e) != 4) {
                    result = sk_error("bad if");
                    break;
                }
                e = e->cdr;
                SkObj *cond = sk_eval(env, e->car);
                if(sk_is_error(cond) && (result = cond))
                    break;

                if(sk_is_true(cond))
                    e = e->cdr;
                else
                    e = e->cdr->cdr;

                rc_release(cond);
                e = e->car;
                continue; /* TCO */

            } else if(!strcmp(what, "and")) {
                int ans = 1;
                for(e = e->cdr; ans && e; e = e->cdr) {
                    SkObj *a = sk_eval(env, e->car);
                    if(sk_is_error(a) && (result = a))
                        goto end;
                    else if(!sk_is_true(a))
                        ans = 0;
                    rc_release(a);
                }
                result = sk_boolean(ans);
            } else if(!strcmp(what, "or")) {
                int ans = 0;
                for(e = e->cdr; !ans && e; e = e->cdr) {
                    SkObj *a = sk_eval(env, e->car);
                    if(sk_is_error(a) && (result = a))
                        goto end;
                    else if(sk_is_true(a))
                        ans = 1;
                    rc_release(a);
                }
                result = sk_boolean(ans);
            } else if(!strcmp(what, "quote")) {
                if(!e->cdr)
                    result = sk_error("bad quote");
                else
                    result = rc_retain(e->cdr->car);
            } else if(!strcmp(what, "begin")) {
                for(e = e->cdr; e && e->cdr; e = e->cdr) {
                    rc_release(result);
                    result = sk_eval(env, e->car);
                    if(sk_is_error(result)) goto end;
                }
                if(e) {
                    e = e->car;
                    continue; /* TCO */
                }
            } else {
                /* Function call */
                rc_release(args);
                args = bind_args(env, e);
                assert(args);
                if(sk_is_error(args)) {
                    result = args;
                    args = NULL;
                    break;
                }

                SkObj *f = args->car, *a = args->cdr;
                if(f->type == CFUN) {
                    assert(f->func);
                    result = f->func(env, a);
                } else if(f->type == LAMBDA) {
                    assert(sk_is_list(f->args));

                    SkEnv *o = new_env;
                    new_env = sk_env_create(env);
                    rc_release(o);

                    SkObj *p = f->args;
                    for(; a && p; a = a->cdr, p = p->cdr)
                        sk_env_put(new_env, sk_get_text(p->car), a->car ? rc_retain(a->car) : NULL);

                    if((a && !p) || (p && !a))
                        result = sk_errorf("too %s arguments passed to lambda", p ? "few" : "many");
                    else {
                        env = new_env;
                        e = f->body;
                        continue; /* TCO */
                    }
                } else
                    result = sk_error("attempt to call something that is not a function");
            }
        } else {
            assert (e->type == VALUE || e->type == TRUE || e->type == FALSE ||
                    e->type == CFUN || e->type == CDATA || e->type == LAMBDA ||
                    e->type == ERROR);
            result = rc_retain(e);
        }
        break;
    } /* for(;;) */
end:

    rc_release(args);
    rc_release(new_env);

    return result;
}

SkObj *sk_eval_str(SkEnv *global, const char *text) {
    SkObj *program = parse_stmts(text), *result;
    if(sk_is_error(program))
        return program;
    result = sk_eval(global, program);
    rc_release(program);
    return result;
}

SkObj *sk_apply(SkEnv *env, SkObj *f, SkObj *a) {
    SkObj *c = sk_cons(rc_retain(f), rc_retain(a));
    SkObj *r = sk_eval(env, c);
    rc_release(c);
    return r;
}

#if 0
/* TODO: use this instead of the external ref counter */
typedef struct refobj {
    unsigned int refcnt;
    ref_dtor dtor;
} RefObj;

void *rc_alloc(size_t size) {
    void *data;
    RefObj *r = malloc((sizeof *r) + size);
    data = (char*)r + sizeof *r;
    r->refcnt = 1;
    r->dtor = NULL;
    return data;
}

void *rc_retain(void *p) {
    RefObj *r;
    if(!p)
        return NULL;
    r = (RefObj *)((char *)p - sizeof *r);
    r->refcnt++;
    return p;
}

void rc_release(void *p) {
    RefObj *r;
    if(!p)
        return;
    r = (RefObj *)((char *)p - sizeof *r);
    if(--r->refcnt == 0) {
        if(r->dtor != NULL)
            r->dtor(p);
        free(r);
    }
}

typedef void (*ref_dtor)(void *);

void rc_set_dtor(void *p, ref_dtor dtor) {
    RefObj *r;
    if(!p) return;
    r = (RefObj *)((char *)p - sizeof *r);
    r->dtor = dtor;
}
#endif

static SkObj *bif_serialize(SkEnv *env, SkObj *e) {
    if(sk_is_null(sk_car(e)))
        return sk_error("'serialize' expects an argument");
    char *text = sk_serialize(sk_car(e));
    return sk_value_o(text);
}

static SkObj *bif_display(SkEnv *env, SkObj *e) {
    for(; e; e = sk_cdr(e)) {
        fputs(sk_get_text(sk_car(e)), stdout);
        fputc(sk_cdr(e) ? ' ' : '\n', stdout);
    }
    return NULL;
}

/* `(apply + '(3 4 5))` or `(apply + (list 1 2))` */
static SkObj *bif_apply(SkEnv *env, SkObj *e) {
    if(sk_length(e) != 2 || !sk_is_list(sk_cadr(e)))
        return sk_error("'apply' expects a function and a list of arguments");
    return sk_apply(env, sk_car(e), sk_cadr(e));
}

static SkObj *bif_cons(SkEnv *env, SkObj *e) {
    if(sk_length(e) != 2)
        return sk_error("'cons' expects 2 arguments");
    return sk_cons(rc_retain(sk_car(e)), rc_retain(sk_cadr(e)));
}

static SkObj *bif_car(SkEnv *env, SkObj *e) {
    if(!sk_is_cons(sk_car(e)))
        return sk_error("'car' expects a cons");
    return rc_retain(sk_caar(e));
}

static SkObj *bif_cdr(SkEnv *env, SkObj *e) {
    if(!sk_is_cons(sk_car(e)))
        return sk_error("'cdr' expects a cons");
    return rc_retain(sk_cdar(e));
}

static SkObj *bif_list(SkEnv *env, SkObj *e) {
    return rc_retain(e);
}

// predicates
#define TYPE_FUNCTION(cname, name, returns) static SkObj *cname(SkEnv *env, SkObj *e){return (e)?(returns):sk_error("'" name "' expects a parameter");}

TYPE_FUNCTION(bif_is_list, "list?", sk_boolean(sk_is_list(e->car)))
TYPE_FUNCTION(bif_length, "sk_length?", sk_number(sk_length(e->car)))
TYPE_FUNCTION(bif_is_null, "null?", sk_boolean(sk_is_null(e->car)))
TYPE_FUNCTION(bif_is_symbol, "symbol?", sk_boolean(sk_is_symbol(e->car)))
TYPE_FUNCTION(bif_is_pair, "pair?", sk_boolean(sk_is_cons(e->car)))
TYPE_FUNCTION(bif_is_procedure, "procedure?", sk_boolean(sk_is_procedure(e->car)))
TYPE_FUNCTION(bif_is_cdata, "cdata?", sk_boolean(sk_is_cdata(e->car)))
TYPE_FUNCTION(bif_is_value, "value?", sk_boolean(sk_is_value(e->car)))
TYPE_FUNCTION(bif_is_number, "number?", sk_boolean(sk_is_number(e->car)))
TYPE_FUNCTION(bif_is_boolean, "boolean?", sk_boolean(sk_is_boolean(e->car)))
TYPE_FUNCTION(bif_not, "not", sk_boolean(!sk_is_true(e->car)))

static SkObj *bif_equal(SkEnv *env, SkObj *e) {
    if(sk_length(e) != 2)
        return sk_error("'equal?' expects 2 arguments");
    return sk_boolean(sk_equal(e->car, e->cdr->car));
}

static SkObj *bif_eq(SkEnv *env, SkObj *e) {
    if(sk_length(e) != 2)
        return sk_error("'eq?' expects 2 arguments");
    return sk_boolean(e->car == e->cdr->car);
}

#define ARITH_FUNCTION(cname, operator)          \
static SkObj *cname(SkEnv *env, SkObj *e) {      \
    if(!e) return sk_number(0);                  \
    double res = atof(sk_get_text(e->car));      \
    for(e = e->cdr; e; e = e->cdr)               \
        res operator atof(sk_get_text(e->car));  \
    return sk_number(res);                       \
}
ARITH_FUNCTION(bif_add, +=)
ARITH_FUNCTION(bif_sub, -=)
ARITH_FUNCTION(bif_mul, *=)

static SkObj *bif_div(SkEnv *env, SkObj *e) {
    double res = 0;
    if(!e) return sk_number(0);
    res = atof(sk_get_text(e->car));
    for(e = e->cdr; e; e = e->cdr) {
        double b = atof(sk_get_text(e->car));
        if(!b) return sk_error("divide by 0");
        res /= b;
    }
    return sk_number(res);
}

static SkObj *bif_mod(SkEnv *env, SkObj *e) {
    int res = 0;
    if(!e) return sk_number(0);
    res = atoi(sk_get_text(e->car));
    for(e = e->cdr; e; e = e->cdr) {
        int b = atoi(sk_get_text(e->car));
        if(!b) return sk_error("divide by 0");
        res %= b;
    }
    return sk_number(res);
}

#define COMPARE_FUNCTION(cname, name, operator)                \
static SkObj *cname(SkEnv *env, SkObj *e) {                    \
    if(sk_length(e) < 2)                                       \
        return sk_error("'" name "' expects two arguments");   \
    const char *a = sk_get_text(e->car);                       \
    const char *b = sk_get_text(e->cdr->car);                  \
    return sk_boolean(operator);                               \
}
COMPARE_FUNCTION(bif_number_eq, "=", atof(a) == atof(b))
COMPARE_FUNCTION(bif_gt, ">", atof(a) > atof(b))
COMPARE_FUNCTION(bif_ge, ">=", atof(a) >= atof(b))
COMPARE_FUNCTION(bif_lt, "<", atof(a) < atof(b))
COMPARE_FUNCTION(bif_le, "<=", atof(a) <= atof(b))

static SkObj *bif_map(SkEnv *env, SkObj *e) {
    if(!sk_is_procedure(sk_car(e)) || !sk_is_list(sk_cadr(e)))
        return sk_error("'map' expects a procedure and a list");
    SkObj *f = sk_car(e), *result = NULL, *last = NULL;
    for(e = sk_cadr(e); e; e = sk_cdr(e)) {
        SkObj *a = sk_cons(rc_retain(sk_car(e)), NULL);
        SkObj *res = sk_apply(env, f, a);
        rc_release(a);
        if(sk_is_error(res)) {
            rc_release(result);
            return res;
        }
        list_append1(&result, res, &last);
    }
    return result;
}

static SkObj *bif_filter(SkEnv *env, SkObj *e) {
    if(!sk_is_procedure(sk_car(e)) || !sk_is_list(sk_cadr(e)))
        return sk_error("'filter' expects a procedure and a list");
    SkObj *f = sk_car(e), *result = NULL, *last = NULL;
    for(e = sk_cadr(e); e; e = sk_cdr(e)) {
        SkObj *a = sk_cons(rc_retain(sk_car(e)), NULL);
        SkObj *res = sk_apply(env, f, a);
        rc_release(a);
        if(sk_is_error(res)) {
            rc_release(result);
            return res;
        } else if(sk_is_true(res))
            list_append1(&result, rc_retain(sk_car(e)), &last);
        rc_release(res);
    }
    return result;
}

static SkObj *bif_append(SkEnv *env, SkObj *e) {
    if(sk_length(e) < 2 || !sk_is_list(e->car) || !sk_is_list(e->cdr->car))
        return sk_error("'append' expects two lists");
    SkObj *x, *result = NULL, *last = NULL;
    /* Need to shallow copy the first list, but not the second */
    for(x = sk_car(e); x; x = sk_cdr(x))
        list_append1(&result, rc_retain(sk_car(x)), &last);
    last->cdr = rc_retain(sk_cadr(e));
    return result;
}

static SkObj *bif_string_length(SkEnv *env, SkObj *e) {
    return sk_number(strlen(sk_get_text(sk_car(e))));
}

static SkObj *bif_string_append(SkEnv *env, SkObj *e) {
    char *buf = NULL;
    int n, a;
    for(; e; e = sk_cdr(e))
        buffer_append(&buf, &n, &a, sk_get_text(sk_car(e)));
    if(!buf) return sk_value("");
    return sk_value_o(buf);
}

static SkObj *bif_string_split(SkEnv *env, SkObj *e) {
    const char *str = sk_get_text(sk_car(e)), *sep = sk_get_text(sk_cadr(e));
    SkObj *result = NULL, *last = NULL;

    if(!sep[0])
        sep = " \t\r\n";

    char *find = strpbrk(str, sep);
    while(find) {
        size_t len = find - str;
        char *buf = malloc(len + 1);
        MEMCHECK(buf);
        strncpy(buf, str, len);
        buf[len] = '\0';
        list_append1(&result, sk_value_o(buf), &last);

        str = find + 1;
        find = strpbrk(str, sep);
    }
    list_append1(&result, sk_value(str), &last);

    return result;
}

static SkObj *bif_substring(SkEnv *env, SkObj *e) {
    const char *str = sk_get_text(sk_car(e));
    SkObj *eo = sk_car(sk_cddr(e));
    int start = atoi(sk_get_text(sk_cadr(e))), end;

    size_t len = strlen(str);

    if(!sk_is_null(eo)) {
        end = atoi(sk_get_text(eo));
        if(end > len)
            end = len;
    } else
        end = len;

    if(start < 0 || end <= start || start >= len)
        return sk_value("");

    len = end - start;

    char *buf = malloc(len+1);
    MEMCHECK(buf);

    strncpy(buf, str + start, len);
    buf[len] = '\0';

    return sk_value_o(buf);
}

static SkObj *bif_string_upcase(SkEnv *env, SkObj *e) {
    char *s = strdup(sk_get_text(sk_car(e))), *c;
    for(c = s; c[0]; c++) *c = toupper(*c);
    return sk_value_o(s);
}

static SkObj *bif_string_downcase(SkEnv *env, SkObj *e) {
    char *s = strdup(sk_get_text(sk_car(e))), *c;
    MEMCHECK(s);
    for(c = s; c[0]; c++) *c = tolower(*c);
    return sk_value_o(s);
}

static SkObj *bif_string_ascii(SkEnv *env, SkObj *e) {
    const char *s = sk_get_text(sk_car(e));
    return sk_number(s[0]);
}

static SkObj *bif_string_char(SkEnv *env, SkObj *e) {
    int s = atoi(sk_get_text(sk_car(e))) & 0x7F;
    char *buf = malloc(2);
    MEMCHECK(buf);
    buf[0] = s; buf[1] = '\0';
    return sk_value_o(buf);
}

static SkObj *bif_string_trim(SkEnv *env, SkObj *e) {
    char *s = strdup(sk_get_text(sk_car(e))), *b;
    int i;
    MEMCHECK(s);
    for(b = s; *b && isspace(b[0]); b++);
    for(i = strlen(b); i > 0 && isspace(b[i-1]); i--) b[i-1] = '\0';
    SkObj *r = sk_value(b);
    free(s);
    return r;
}

static SkObj *bif_string_find(SkEnv *env, SkObj *e) {
    const char *haystack = sk_get_text(sk_car(e));
    const char *needle = sk_get_text(sk_cadr(e));
    if(haystack[0] == '\0')
        return NULL;
    if(needle[0] == '\0')
        return sk_error("`string-find` requires a haystack and a needle");
    char *found = strstr(haystack, needle);
    if(!found)
        return NULL;
    return sk_number(found - haystack);
}

static SkObj *bif_string_replace(SkEnv *env, SkObj *e) {
    const char *str, *srch, *rep;
    char *buf = NULL, *find;
    int n, a, sl;

    str = sk_get_text(sk_car(e));
    srch = sk_get_text(sk_cadr(e));
    rep = sk_get_text(sk_car(sk_cddr(e)));

    sl = strlen(srch);
    if(!sl) return rc_retain(sk_car(e));

    while((find = strstr(str, srch))) {
        buffer_appendn(&buf, &n, &a, str, find - str);
        buffer_append(&buf, &n, &a, rep);
        str = find + sl;
    }
    buffer_append(&buf, &n, &a, str);
    return sk_value_o(buf);
}

COMPARE_FUNCTION(bif_string_eq, "string=?", !strcmp(a, b))
COMPARE_FUNCTION(bif_string_lt, "string<?", strcmp(a, b) < 0)

#define MATH_FUN(f) static SkObj *bif_ ## f(SkEnv *env, SkObj *e) { return sk_number(f(atof(sk_get_text(sk_car(e))))); }

MATH_FUN(sin)
MATH_FUN(cos)
MATH_FUN(tan)
MATH_FUN(asin)
MATH_FUN(acos)
MATH_FUN(log)
MATH_FUN(exp)
MATH_FUN(sqrt)
MATH_FUN(ceil)
MATH_FUN(floor)
MATH_FUN(fabs)

static SkObj *bif_atan(SkEnv *env, SkObj *e) {
    double p, q;
    p = atof(sk_get_text(sk_car(e)));
    if(sk_is_null(sk_cadr(e)))
        return sk_number(atan(p));
    q = atof(sk_get_text(sk_cadr(e)));
    return sk_number(atan2(p, q));
}

static SkObj *bif_pow(SkEnv *env, SkObj *e) {
    double x, y;
    x = atof(sk_get_text(sk_car(e)));
    y = atof(sk_get_text(sk_cadr(e)));
    return sk_number(pow(x, y));
}

#define TEXT_LIB(g,t) do {SkObj *x=sk_eval_str(g,t);assert(!sk_is_error(x));rc_release(x);} while(0)

/** ## Built-in Functions */
SkEnv *sk_global_env() {
    SkEnv *global = sk_env_create(NULL);

    /** `(serialize val)` - Serializes a value into a string */
    sk_env_put(global, "serialize", sk_cfun(bif_serialize));
    /** `(write val)` - writes a serialized value to `stdout` */
    TEXT_LIB(global,"(define (write val) (display (serialize val)))");
    /** `(display str)` - writes string to `stdout` */
    sk_env_put(global, "display", sk_cfun(bif_display));
    /** `(cons car cdr)` - Creates a cons cell with the given car and cdr */
    sk_env_put(global, "cons", sk_cfun(bif_cons));
    /** `(car c)` - returns the car of the cons cell `c` */
    sk_env_put(global, "car", sk_cfun(bif_car));
    /** `(cdr c)` - returns the cdr of the cons cell `c` */
    sk_env_put(global, "cdr", sk_cfun(bif_cdr));
    /** `(caar c)`, `(cadr c)`, `(cdar c)`, `(cddr c)` - extensions around `car` and `cdr` */
    TEXT_LIB(global,"(define (caar x) (car (car x)))");
    TEXT_LIB(global,"(define (cadr x) (car (cdr x)))");
    TEXT_LIB(global,"(define (cdar x) (cdr (car x)))");
    TEXT_LIB(global,"(define (cddr x) (cdr (cdr x)))");
    /** `(list e1 e2 e3...)` - Creates a list consisting of `e1`, `e2`, `e3` etc */
    sk_env_put(global, "list", sk_cfun(bif_list));
    /** `(length L)` - finds the length of the list `L` */
    sk_env_put(global, "length", sk_cfun(bif_length));

    /** `(list? x)` - returns `#t` if `x` is a list */
    sk_env_put(global, "list?", sk_cfun(bif_is_list));
    /** `(null? x)` - returns `#t` if `x` is null (also represented as `'()`) */
    sk_env_put(global, "null?", sk_cfun(bif_is_null));
    /** `(symbol? x)` - returns `#t` if `x` is a symbol */
    sk_env_put(global, "symbol?", sk_cfun(bif_is_symbol));
    /** `(pair? x)` - returns `#t` if `x` is a pair (a cons cell) */
    sk_env_put(global, "pair?", sk_cfun(bif_is_pair));
    /** `(procedure? x)` - returns `#t` if `x` is a callable procedure (a lambda or a CFun object) */
    sk_env_put(global, "procedure?", sk_cfun(bif_is_procedure));
    /** `(cdata? x)` - returns `#t` if `x` is a CData object */
    sk_env_put(global, "cdata?", sk_cfun(bif_is_cdata));
    /** `(value? x)` - returns `#t` if `x` is a value object */
    sk_env_put(global, "value?", sk_cfun(bif_is_value));
    /** `(string? x)` - returns `#t` if `x` is a string value object */
    TEXT_LIB(global,"(define (string? x) (and (value? x) (not (number? x))))");
    /** `(number? x)` - returns `#t` if `x` is a number value object */
    sk_env_put(global, "number?", sk_cfun(bif_is_number));
    /** `(zero? x)` - returns `#t` if `x` is 0 */
    TEXT_LIB(global,"(define (zero? x) (and (number? x) (= 0 x)))");
    /** `(boolean? x)` - returns `#t` if `x` is a boolean object (`#t` or `#f`) */
    sk_env_put(global, "boolean?", sk_cfun(bif_is_boolean));
    /** `(true? x)` - returns `#t` if `x` evaluates to truth */
    TEXT_LIB(global,"(define (true? x) (if x #t #f))");

    /** `(equal? x y)` - Compares `x` and `y` for equality */
    sk_env_put(global, "equal?", sk_cfun(bif_equal));
    /** `(eq? x y)` - returns true if and only if `x` and `y` references the same object */
    sk_env_put(global, "eq?", sk_cfun(bif_eq));
    /** `(not x)` - logical not. Returns `#f` if and only if `x` evaluates to `#t` */
    sk_env_put(global, "not", sk_cfun(bif_not));
    /** `(apply f '(arg1 arg2))` - Applies a function to the given arguments */
    sk_env_put(global, "apply", sk_cfun(bif_apply));
    /** `(+ v1 v2...)`, `(- v1 v2...)`, `(* v1 v2...)`, `(/ v1 v2...)`, `(% v1 v2...)` - Arithmetic operators */
    sk_env_put(global, "+", sk_cfun(bif_add));
    sk_env_put(global, "-", sk_cfun(bif_sub));
    sk_env_put(global, "*", sk_cfun(bif_mul));
    sk_env_put(global, "/", sk_cfun(bif_div));
    sk_env_put(global, "%", sk_cfun(bif_mod));
    /** `(= v1 v2)`, `(> v1 v2)`, `(< v1 v2)`, `(>= v1 v2)`, `(<= v1 v2)` - Comparison operators */
    sk_env_put(global, "=", sk_cfun(bif_number_eq));
    sk_env_put(global, ">", sk_cfun(bif_gt));
    sk_env_put(global, "<", sk_cfun(bif_lt));
    sk_env_put(global, ">=", sk_cfun(bif_ge));
    sk_env_put(global, "<=", sk_cfun(bif_le));
    /** `(map f L)` - Returns a list where each element is the result of the function `f` applied to the
     * corresponding element in the list `L` */
    sk_env_put(global, "map", sk_cfun(bif_map));
    /** `(filter f L)` - Returns a list  */
    sk_env_put(global, "filter", sk_cfun(bif_filter));
    /** `(append L1 L2)` - Returns containing the elements of */
    sk_env_put(global, "append", sk_cfun(bif_append));

    /** `(fold f i L)` and `(fold-right f i L)` - Folds a list `L` using function `f` with the initial value `i` */
    TEXT_LIB(global,"(define (fold f i L) (if (null? L) i (fold f (f (car L) i) (cdr L))))");
    TEXT_LIB(global,"(define (fold-right f i L) (if (null? L) i (f (car L) (fold-right f i (cdr L)))))");
    /** `(reverse L)` - Reverses a list `L` */
    TEXT_LIB(global,"(define (reverse l) (fold cons '() l))");
    /** `(range a b)` - Returns a list of all the integers between `a` and `b` */
    TEXT_LIB(global,"(define (range a b) (if (= a b) (list b) (cons a (range (+ a 1) b))))");
    /** `(nth n L)` - Returns the `n`-th element of the list `L` */
    TEXT_LIB(global,"(define (nth n L) (if (or (null? L) (< n 0)) '() (if (= n 1) (car L) (nth (- n 1) (cdr L)))))");

    /** `(string-length s)` - returns the length of the string `s` */
    sk_env_put(global, "string-length?", sk_cfun(bif_string_length));
    /** `(string-append s1 s2...)` - Appends all parameters into a new string. */
    sk_env_put(global, "string-append", sk_cfun(bif_string_append));
    /** `(string-replace str find repl)` - */
    sk_env_put(global, "string-replace", sk_cfun(bif_string_replace));
    /** `(string-split str sep)` - Splits a string `str` into a list of substrings */
    sk_env_put(global, "string-split", sk_cfun(bif_string_split));
    /** `(substring str start [end])` - Retrieves the substring of `str` between `start` and `end`. */
    sk_env_put(global, "substring", sk_cfun(bif_substring));
    /** `(string-upcase str)` - Converts a string `str` to uppercase */
    sk_env_put(global, "string-upcase", sk_cfun(bif_string_upcase));
    /** `(string-downcase str)` - Converts a string `str` to lowercase */
    sk_env_put(global, "string-downcase", sk_cfun(bif_string_downcase));
    /** `(string-ascii c)` - Returns the ASCII value of the first character in the string `c` */
    sk_env_put(global, "string-ascii", sk_cfun(bif_string_ascii));
    /** `(string-char a)` - Converts the ASCII value `a` to a string */
    sk_env_put(global, "string-char", sk_cfun(bif_string_char));
    /** `(string-trim s)` - Trims whitespace from the start and end of a string `s` */
    sk_env_put(global, "string-trim", sk_cfun(bif_string_trim));
    /** `(string-find h n)` - Searches for the substring `n` in the string `h` and returns the position, `'()` if not found. */
    sk_env_put(global, "string-find", sk_cfun(bif_string_find));

    /** `(string-contains? h n)` - Returns `#t` if the string `h` contains the substring `n`. */
    TEXT_LIB(global,"(define (string-contains? h n) (not (null? (string-find h n))))");
    /** `(string-prefix? h n)` - Returns `#t` if the string `h` starts with the substring `n`. */
    TEXT_LIB(global,"(define (string-prefix? h n) (true? (zero? (string-find h n))))");
    /** `(string-suffix? h n)` - Returns `#t` if the string `h` ends with the substring `n`. */
    TEXT_LIB(global,"(define (string-suffix? h n) (= (string-find h n) (- (string-length? h) (string-length? n)) ))");
    /** `(non-empty-string? s)` - Returns `#t` if the string is not empty. */
    TEXT_LIB(global,"(define (non-empty-string? s) (not (= 0 (string-length? s))))");
    /** `(string=? s1 s2)`, `(string<? s1 s2)`, `(string<=? s1 s2)`, `(string>? s1 s2)` and `(string>=? s1 s2)` - string comparisons between `s1` and `s2` */
    sk_env_put(global, "string=?", sk_cfun(bif_string_eq));
    sk_env_put(global, "string<?", sk_cfun(bif_string_lt));
    TEXT_LIB(global,"(define (string<=? a b) (or (string<? a b) (string=? a b)))");
    TEXT_LIB(global,"(define (string>? a b) (not (string<=? a b)))");
    TEXT_LIB(global,"(define (string>=? a b) (not (string<? a b)))");

    /** `(sin x)` - sine of `x` */
    sk_env_put(global, "sin", sk_cfun(bif_sin));
    /** `(cos x)` - cosine of `x` */
    sk_env_put(global, "cos", sk_cfun(bif_cos));
    /** `(tan x)` - tangent of `x` */
    sk_env_put(global, "tan", sk_cfun(bif_tan));
    /** `(asin x)` - arc-sine of `x` */
    sk_env_put(global, "asin", sk_cfun(bif_asin));
    /** `(acos x)` - arc-cosine of `x` */
    sk_env_put(global, "acos", sk_cfun(bif_acos));
    /** `(atan p)` or `(atan y x)` - arc-tangent of `p` or `y/x` */
    sk_env_put(global, "atan", sk_cfun(bif_atan));
    /** `(log x)` - natural logartihm of `x` */
    sk_env_put(global, "log", sk_cfun(bif_log));
    /** `(exp x)` - exponential of `x` */
    sk_env_put(global, "exp", sk_cfun(bif_exp));
    /** `(sqrt x)` - square root of `x` */
    sk_env_put(global, "sqrt", sk_cfun(bif_sqrt));
    /** `(ceil x)` - ceiling of `x` */
    sk_env_put(global, "ceil", sk_cfun(bif_ceil));
    /** `(floor x)` - floor of `x` */
    sk_env_put(global, "floor", sk_cfun(bif_floor));
    /** `(abs x)` - absolute value of `x` */
    sk_env_put(global, "abs", sk_cfun(bif_fabs));
    /** `(pow x y)` - `x` raised to the power of `y` */
    sk_env_put(global, "pow", sk_cfun(bif_pow));
    /** `pi` - 3.14159... */
    sk_env_put(global, "pi", sk_number(M_PI));

    return global;
}
