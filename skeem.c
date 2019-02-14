/**
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

/* Anonymous structs and unions are not part of the C standard, but they are
so useful that I can't get myself to remove them */
typedef struct Expr {
    enum {SYMBOL, VALUE, CONS, CFUN, TRUE, FALSE, LAMBDA, ERROR} type;
    union {
        char *value;
        c_function func;
        struct {
           struct Expr *car, *cdr; /* for cons cells */
        };
        struct {
           struct Expr *args, *body; /* for lambdas */
        };
    };
} Expr;

typedef struct hash_element {
    char *name;
    Expr *ex;
    struct hash_element *next;
} hash_element;

typedef struct Env {
    struct hash_element *table[HASH_SIZE];
    struct Env *parent;
} Env;

static void env_dtor(Env *env) {
    int i;
    for(i = 0; i < HASH_SIZE; i++) {
        while(env->table[i]) {
            hash_element* v = env->table[i];
            env->table[i] = v->next;
            if(v->ex) rc_release(v->ex);
            free(v->name);
            free(v);
        }
        env->table[i] = NULL;
    }
    if(env->parent) rc_release(env->parent);
}

Env *env_create(Env *parent) {
    int i;
    Env *env = rc_alloc(sizeof *env);
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

static hash_element *env_findg_r(Env *env, const char *name, unsigned int h) {
    hash_element *v;
    if(!env)
        return NULL;
    for(v = env->table[h]; v; v = v->next)
        if(!strcmp(v->name, name))
            return v;
    return env_findg_r(env->parent, name, h);
}

/* Find globally, use for get operations */
static hash_element *env_findg(Env *env, const char *name) {
    unsigned int h = hash(name);
    return env_findg_r(env, name, h);
}

/* Find locally, use for put operations */
static hash_element *env_find(Env *env, const char *name) {
    unsigned int h = hash(name);
    hash_element *v;
    if(!env)
        return NULL;
    for(v = env->table[h]; v; v = v->next)
        if(!strcmp(v->name, name))
            return v;
    return NULL;
}

Expr *env_get(Env *env, const char *name) {
    hash_element* v = env_findg(env, name);
    if(v)
        return v->ex;
    return errorf("no such variable '%s'", name);
}

Expr *env_put(Env *env, const char *name, Expr *e) {
    hash_element* v = env_find(env, name);
    if(!v) {
        unsigned int h = hash(name);
        v = malloc(sizeof *v);
        v->name = strdup(name);
        v->next = env->table[h];
        env->table[h] = v;
    } else
        rc_release(v->ex);
    v->ex = e;
    return e;
}

int equal(Expr *a, Expr *b) {
    if(!a || !b)
        return !a && !b;
    else if(a->type != b->type)
        return 0;
    else switch(a->type) {
        case CFUN: return a->func == b->func;
        case ERROR: return 0;
        case SYMBOL: return !strcmp(a->value, b->value);
        case VALUE: return !strcmp(a->value, b->value);
        case TRUE:
        case FALSE: return 1;
        case CONS: return equal(a->car, b->car) && equal(a->cdr, b->cdr);
        case LAMBDA: return equal(a->args, b->args) && equal(a->body, b->body);
    }
    return 1;
}

void write(Expr *e) {
    if(!e)
        printf("() ");
    else switch(e->type) {
        case CFUN: printf("#<cfun:%p> ", e->func); break;
        case ERROR: printf("#<error:%s> ", e->value); break;
        case SYMBOL: printf("%s ", e->value); break;
        case TRUE: printf("#t "); break;
        case FALSE: printf("#f "); break;
        case VALUE:
            /* FIXME: this is not great. */
            printf("\"%s\" ", e->value);
            break;
        case CONS:
            printf("( ");
            for(;;) {
                write(e->car);
                if(e->cdr) {
                    if(e->cdr->type == CONS)
                        e = e->cdr;
                    else {
                        printf(". ");
                        write(e->cdr);
                        break;
                    }
                } else
                    break;
            }
            printf(") ");
            break;
        case LAMBDA:
            printf("<lambda: ");
            write(e->args);
            printf(": ");
            write(e->body);
            printf("> ");
            break;
    }
}

static void expr_dtor(Expr *e) {
    switch(e->type) {
        case ERROR:
        case SYMBOL:
        case VALUE: /* FIXME free(e->value); */ break;
        case CONS:
            if(e->car) rc_release(e->car);
            if(e->cdr) rc_release(e->cdr);
            break;
        case LAMBDA: if(e->args) rc_release(e->args); rc_release(e->body); break;
        default: break;
    }
}

#ifdef NDEBUG
#  define MEMCHECK(p) if(!p) abort() /* You're doomed */
#else
#  define MEMCHECK(p) assert(p)
#endif

#ifdef NDEBUG
Expr *symbol(const char *value) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *symbol_(const char *value, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = SYMBOL;
    e->value = strdup(value);
    return e;
}

#ifdef NDEBUG
Expr *value(const char *val) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *value_(const char *val, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = VALUE;
    e->value = strdup(val);
    return e;
}

#ifdef NDEBUG
Expr *error(const char *val) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *error_(const char *val, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = ERROR;
    e->value = strdup(val);
    return e;
}

Expr *errorf(const char *fmt, ...) {
    va_list arg;
    char buffer[512];
    va_start(arg, fmt);
    vsnprintf (buffer, sizeof(buffer)-1, fmt, arg);
    va_end(arg);
    return error(buffer);
}

#ifdef NDEBUG
Expr *boolean(int val) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *boolean_(int val, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = val ? TRUE : FALSE;
    return e;
}


#define RESULT_SIZE 64
#define PRECISION   30
#ifdef NDEBUG
Expr *nvalue(double n) {
    char result[RESULT_SIZE];
    snprintf(result, sizeof result - 1, "%.*g", PRECISION, n);
    return value(result);
}
#else
Expr *nvalue_(double n, const char *file, int line) {
    char result[RESULT_SIZE];
    snprintf(result, sizeof result - 1, "%.*g", PRECISION, n);
    return value_(result, file, line);
}
#endif

#ifdef NDEBUG
Expr *cons(Expr *car, Expr *cdr) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *cons_(Expr *car, Expr *cdr, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = CONS;
    e->car = car;
    e->cdr = cdr;
    return e;
}

#ifdef NDEBUG
Expr *lambda(Expr *args, Expr *body) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *lambda_(Expr *args, Expr *body, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = LAMBDA;
    e->args = args;
    e->body = body;
    return e;
}

#ifdef NDEBUG
Expr *cfun(c_function func) {
    Expr *e = rc_alloc(sizeof *e);
#else
Expr *cfun_(c_function func, const char *file, int line) {
    Expr *e = rc_alloc_(sizeof *e, file, line);
#endif
    MEMCHECK(e);
    rc_set_dtor(e, (ref_dtor)expr_dtor);
    e->type = CFUN;
    e->func = func;
    return e;
}

/* O(1) way to append items to a list,
but it needs a separate pointer to track the last item in the list
*/
#ifdef NDEBUG
static void list_append1(Expr **list, Expr *a, Expr **last) {
    Expr *item = cons(NULL, NULL);
#else
#  define list_append1(l, a, ls) list_append1_(l, a, ls, __FILE__, __LINE__)
static void list_append1_(Expr **list, Expr *a, Expr **last, const char *file, int line) {
    Expr *item = cons_(NULL, NULL, file, line);
#endif
    item->car = a;
    if(!*last)
        *list = item;
    else
        (*last)->cdr = item;
    *last = item;
}

int check_numeric(const char *c) {
    return isdigit(c[0]) || (strchr("+-", c[0]) && isdigit(c[1]));
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
    } else if (strchr("()'[]", *in)) {
        tok[0] = *in;
        tok[1] ='\0';
        *rem =  ++in;
        return tok[0];
    } else if (*in == '\"') {
        while(*++in != '\"') {
            if(!*in) {
                snprintf(tok, n, "unterminated string constant");
                return SCAN_ERROR;
            } else if(i == n) {
                snprintf(tok, n, "token too long");
                return SCAN_ERROR;
            }
            /* TODO: Escape sequences... */
            tok[i++] = *in;
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

        if(check_numeric(tok)) return SCAN_VALUE;
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

static Expr *parse0(Parser *p) {
    if(accept(p, SCAN_ERROR))
        return error(p->tok);
    else if(accept(p, SCAN_END))
        return NULL;
    else if(accept(p, SCAN_SYMBOL))
        return symbol(p->tok);
    else if(accept(p, SCAN_VALUE))
        return value(p->tok);
    else if(accept(p, SCAN_TRUE))
        return boolean(1);
    else if(accept(p, SCAN_FALSE))
        return boolean(0);
    else if(accept(p, '(') || accept(p, '[')) {
        Expr *list = NULL, *last = NULL;
        char term = p->tok[0] == '(' ? ')' : ']';

        while(!accept(p, term)) {
            if(accept(p, SCAN_ERROR)) {
                if(list) rc_release(list);
                return error(p->tok);
            } else if(accept(p, SCAN_END)) {
                if(list) rc_release(list);
                return errorf("expected '%c'", term);
            }

            Expr *e = parse0(p);
            if(is_error(e)) {
                if(list) rc_release(list);
                return e;
            }

            list_append1(&list, e, &last);
        }
        return list;
    } else if(accept(p, '\'')) {
        Expr *e = parse0(p);
        if(is_error(e))
            return e;
        return cons(symbol("quote"), cons(e, NULL));
    } else if(accept(p, ')') || accept(p, ']'))
        return errorf("mismatched '%c'", p->tok[0]);

    return error("unhandled token type");
}

Expr *parse(const char *text) {
    Parser p;
    p.text = text;
    p.next_tok[0] = '\0';
    nextsym(&p);
    return parse0(&p);
}

Expr *parse_stmts(const char *text) {
    Parser p;
    Expr *root = cons(symbol("begin"), NULL), *last = root;

    p.text = text;
    p.next_tok[0] = '\0';
    nextsym(&p);

    for(;;) {
        if(accept(&p, SCAN_END)) break;
        Expr * e = parse0(&p);
        if(is_error(e)) {
            if(root) rc_release(root);
            return e;
        }
        list_append1(&root, e, &last);
    }
    return root;
}

static Env *get_global(Env *env) {
    while(env->parent)
        env = env->parent;
    return env;
}

const char *get_text(Expr *e) {
    return (e && (e->type == VALUE || e->type == SYMBOL || e->type == ERROR)) ? e->value : "";
}

int is_null(Expr *e) {
    return e == NULL;
}

int is_symbol(Expr *e) {
    return e && e->type == SYMBOL;
}

int is_cons(Expr *e) {
    return e && e->type == CONS;
}

int is_error(Expr *e) {
    return e && e->type == ERROR;
}

int is_boolean(Expr *e) {
    return e && (e->type == FALSE || e->type == TRUE);
}

int is_true(Expr *e) {
    return e && e->type != FALSE;
}

int is_list(Expr *e) {
    for(; e; e = e->cdr)
        if(e->type != CONS)
            return 0;
    return 1;
}

int is_procedure(Expr *e) {
    return e && (e->type == CFUN || e->type == LAMBDA);
}

int is_number(Expr *e) {
    return e && e->type == VALUE && check_numeric(e->value);
}

int length(Expr *e) {
    int count = 0;
    for(; e && e->type == CONS; e = e->cdr) count++;
    return count;
}

static Expr *bind_args(Env *env, Expr *e) {
    assert(e->type == CONS);
    Expr *args = NULL, *last = NULL;
    for(; e; e = e->cdr) {
        Expr *arg = eval(env, e->car);
        if(is_error(arg)) {
            if(args) rc_release(args);
            return arg;
        }
        list_append1(&args, arg, &last);
    }
    return args;
}

static int valid_lambda(Expr *l) {
    if(l->type != LAMBDA) return 0;
    if(!is_null(l->args)) {
        Expr *e;
        for(e = l->args; e; e = e->cdr)
            if(e->type != CONS || e->car->type != SYMBOL)
                return 0;
    }
    return is_list(l->body);
}

Expr *apply(Env *env, Expr *f, Expr *a) {
    Expr *result;
    if(!is_list(a))
        return error("'apply' expects a list");
    if(f->type == CFUN) {
        assert(f->func);
        result = f->func(env, a);
    } else if(f->type == LAMBDA) {
        Env *new_env = env_create(env);
        Expr *p = f->args;
        for(; a && p; a = a->cdr, p = p->cdr)
            env_put(new_env, get_text(p->car), a->car ? rc_retain(a->car) : NULL);
        if((a && !p) || (p && !a))
            result = errorf("too %s arguments passed to lambda", p ? "few" : "many");
        else
            result = eval(new_env, f->body);
        rc_release(new_env);
    } else
        result = error("'apply' on something that is not a procedure");
    return result;
}

Expr *eval(Env *env, Expr *e) {
    Expr *result = NULL, *args = NULL;
    Env *new_env = NULL;

    assert(env);
    for(;;) {
        if(result) {
            rc_release(result);
            result = NULL;
        }
        if(!e)
            result = NULL;
        else if(e->type == VALUE || e->type == TRUE || e->type == FALSE ||
            e->type == CFUN || e->type == LAMBDA || e->type == ERROR) {
            result = rc_retain(e);
        } else if(e->type == SYMBOL) {
            Expr *r = env_get(env, get_text(e));
            if(is_error(r))
                result = r;
            else
                result = r ? rc_retain(r) : NULL;
        } else if(e->type == CONS) {
            const char *what = get_text(e->car);
            if(!is_list(e)) {
                result = errorf("bad %s", what[0] ? what : "list");
                break;
            }
            if(!strcmp(what, "define") || !strcmp(what, "set!")) {
                if(length(e) != 3) {
                    result = errorf("bad %s", what);
                    break;
                }
                e = e->cdr;

                const char *varname;
                if(is_list(e->car)) {
                    /* `(define (f a b c) (body))` form */
                    Expr *f = e->car, *p = f->cdr;
                    if(length(f) < 1) {
                        result = error("define lambda needs function name");
                        break;
                    }
                    varname = get_text(f->car);

                    Expr *body = cons(symbol("begin"), e->cdr ? rc_retain(e->cdr) : NULL);

                    p = p ? rc_retain(p) : NULL;

                    result = lambda(p, body);

                    if(!valid_lambda(result)) {
                        rc_release(result);
                        result = error("invalid lambda define");
                        break;
                    }
                } else {
                    /* `(define v expr)` form */
                    varname = get_text(e->car);
                    result = eval(env, e->cdr->car);
                    if(is_error(result))
                        break;
                }
                Env *tgt_env = env;

                if(result) rc_retain(result);

                if(!strcmp(what, "define"))
                    tgt_env = get_global(tgt_env);

                env_put(tgt_env, varname, result);

            } else if(!strcmp(what, "let")) {
                if(length(e) < 3 || !is_list(e->cdr->car)) {
                    result = error("bad let");
                    break;
                }
                Expr *a = e->cdr->car, *b = e->cdr->cdr;

                Env *o = new_env;
                new_env = env_create(env);
                if(o) rc_release(o);

                for(; a; a = a->cdr) {
                    if(!is_list(a->car) || length(a->car) != 2
                        || !is_symbol(a->car->car)) {
                        result = error("bad clause in 'let'");
                        goto end_let;
                    }
                    const char *name = get_text(a->car->car);
                    Expr *v = eval(env, a->car->cdr->car);
                    if(is_error(v)) {
                        result = v;
                        goto end_let;
                    }
                    env_put(new_env, name, v);
                }
                for(; b && b->cdr; b = b->cdr) {
                    if(result) rc_release(result);
                    result = eval(new_env, b->car);
                    if(is_error(result))
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
                if(length(e) < 3) {
                    result = error("bad lambda");
                    break;
                }
                e = e->cdr;
                Expr *body = cons(symbol("begin"), rc_retain(e->cdr));

                result = lambda(e->car ? rc_retain(e->car) : NULL, body);

                if(!valid_lambda(result)) {
                    rc_release(result);
                    result = error("invalid lambda");
                    break;
                }
            } else if(!strcmp(what, "if")) {
                if(length(e) != 4) {
                    result = error("bad if");
                    break;
                }
                e = e->cdr;
                Expr *cond = eval(env, e->car);
                if(is_error(cond) && (result = cond))
                    break;

                if(is_true(cond))
                    e = e->cdr;
                else
                    e = e->cdr->cdr;

                if(cond) rc_release(cond);
                e = e->car;
                continue; /* TCO */

            } else if(!strcmp(what, "and")) {
                int ans = 1;
                for(e = e->cdr; ans && e; e = e->cdr) {
                    Expr *a = eval(env, e->car);
                    if(is_error(a)) {
                        result = a;
                        goto end;
                    } else if(!is_true(a))
                        ans = 0;
                    if(a) rc_release(a);
                }
                result = boolean(ans);
            } else if(!strcmp(what, "or")) {
                int ans = 0;
                for(e = e->cdr; !ans && e; e = e->cdr) {
                    Expr *a = eval(env, e->car);
                    if(is_error(a)) {
                        result = a;
                        goto end;
                    } else if(is_true(a))
                        ans = 1;
                    if(a) rc_release(a);
                }
                result = boolean(ans);
            } else if(!strcmp(what, "quote")) {
                if(length(e) != 2)
                    result = error("bad quote");
                else
                    result = e->cdr->car ? rc_retain(e->cdr->car) : NULL;
            } else if(!strcmp(what, "begin")) {
                for(e = e->cdr; e && e->cdr; e = e->cdr) {
                    if(result) rc_release(result);
                    result = eval(env, e->car);
                    if(is_error(result)) goto end;
                }
                if(e) {
                    e = e->car;
                    continue; /* TCO */
                }
            } else {
                /* Function call */
                if(args) rc_release(args);
                args = bind_args(env, e);
                assert(args);
                if(is_error(args)) {
                    result = args;
                    args = NULL;
                    break;
                }

                Expr *f = args->car, *a = args->cdr;
                if(f->type == CFUN) {
                    assert(f->func);
                    result = f->func(env, a);
                } else if(f->type == LAMBDA) {
                    assert(is_list(f->args));

                    Env *o = new_env;

                    new_env = env_create(env);

                    if(o) rc_release(o);

                    Expr *p = f->args;
                    for(; a && p; a = a->cdr, p = p->cdr)
                        env_put(new_env, get_text(p->car), a->car ? rc_retain(a->car) : NULL);

                    if((a && !p) || (p && !a))
                        result = errorf("too %s arguments passed to lambda", p ? "few" : "many");
                    else {
                        env = new_env;
                        e = f->body;
                        continue; /* TCO */
                    }
                } else
                    result = error("attempt to call something that is not a function");
            }
        } else
            result = error("don't know how to `eval()` this :(");
        break;
    } /* for(;;) */
end:

    if(args) rc_release(args);
    if(new_env) rc_release(new_env);

    return result;
}

Expr *eval_str(Env *global, const char *text) {
	Expr *program = parse_stmts(text), *result;
	if(is_error(program))
		return program;
	result = eval(global, program);
	rc_release(program);
	return result;
}

static Expr *bif_write(Env *env, Expr *e) {
    Expr *last = NULL;
    if(!e) return error("'write' expects an argument");
    for(; e; e = e->cdr) {
        last = e->car;
        write(last);
        fputc('\n', stdout);
    }
    return rc_retain(last);
}

static Expr *bif_display(Env *env, Expr *e) {
    const char *txt = "";
    for(; e; e = e->cdr) {
        txt = get_text(e->car);
        fputs(txt, stdout);
        fputc(e->cdr ? ' ' : '\n', stdout);
    }
    return value(txt);
}

/* `(apply + '(3 4 5))` or `(apply + (list 1 2))` */
static Expr *bif_apply(Env *env, Expr *e) {
    Expr *fun, *args, *res;
    if(length(e) != 2 || !is_list(e->cdr->car))
        return error("'apply' expects a function and arguments");
    fun = e->car;
    args = e->cdr->car;
    res = apply(env, fun, args);
    return res;
}

static Expr *bif_cons(Env *env, Expr *e) {
    if(length(e) != 2)
        return error("'cons' expects 2 arguments");
    return cons(rc_retain(e->car), rc_retain(e->cdr->car));
}

static Expr *bif_car(Env *env, Expr *e) {
    if(!e || !is_cons(e->car))
        return error("'car' expects a list");
    return e->car->car ? rc_retain(e->car->car) : NULL;
}

static Expr *bif_cdr(Env *env, Expr *e) {
    if(!e || !is_cons(e->car))
        return error("'cdr' expects a list");
    return e->car->cdr ? rc_retain(e->car->cdr) : NULL;
}

static Expr *bif_list(Env *env, Expr *e) {
    return e ? rc_retain(e) : NULL;
}

// predicates
#define TYPE_FUNCTION(cname, name, returns)             \
static Expr *cname(Env *env, Expr *e) {                 \
    if(!e)                                              \
        return error("'" name "' expects a parameter"); \
    return returns;                                     \
}

TYPE_FUNCTION(bif_is_list, "list?", boolean(is_list(e->car)))
TYPE_FUNCTION(bif_length, "length?", nvalue(length(e->car)))
TYPE_FUNCTION(bif_is_null, "null?", boolean(is_null(e->car)))
TYPE_FUNCTION(bif_is_symbol, "symbol?", boolean(is_symbol(e->car)))
TYPE_FUNCTION(bif_is_pair, "pair?", boolean(is_cons(e->car)))
TYPE_FUNCTION(bif_is_procedure, "procedure?", boolean(is_procedure(e->car)))
TYPE_FUNCTION(bif_is_number, "number?", boolean(is_number(e->car)))
TYPE_FUNCTION(bif_is_boolean, "boolean?", boolean(is_boolean(e->car)))
TYPE_FUNCTION(bif_is_zero, "zero?", boolean(atof(get_text(e->car)) == 0.0))
TYPE_FUNCTION(bif_not, "not", boolean(!is_true(e->car)))

static Expr *bif_equal(Env *env, Expr *e) {
    if(length(e) != 2)
        return error("'equal?' expects 2 arguments");
    return boolean(equal(e->car, e->cdr->car));
}

static Expr *bif_eq(Env *env, Expr *e) {
    if(length(e) != 2)
        return error("'eq?' expects 2 arguments");
    return boolean(e->car == e->cdr->car);
}

#define ARITH_FUNCTION(cname, operator)       \
static Expr *cname(Env *env, Expr *e) {       \
    if(!e) return nvalue(0);                  \
    double res = atof(get_text(e->car));      \
    for(e = e->cdr; e; e = e->cdr)            \
        res operator atof(get_text(e->car));  \
    return nvalue(res);                       \
}
ARITH_FUNCTION(bif_add, +=)
ARITH_FUNCTION(bif_sub, -=)
ARITH_FUNCTION(bif_mul, *=)

static Expr *bif_div(Env *env, Expr *e) {
    double res = 0;
    if(!e) return nvalue(0);
    res = atof(get_text(e->car));
    for(e = e->cdr; e; e = e->cdr) {
        double b = atof(get_text(e->car));
        if(!b) return error("divide by 0");
        res /= b;
    }
    return nvalue(res);
}

static Expr *bif_mod(Env *env, Expr *e) {
    int res = 0;
    if(!e) return nvalue(0);
    res = atoi(get_text(e->car));
    for(e = e->cdr; e; e = e->cdr) {
        int b = atoi(get_text(e->car));
        if(!b) return error("divide by 0");
        res %= b;
    }
    return nvalue(res);
}

#define COMPARE_FUNCTION(cname, name, operator)             \
static Expr *cname(Env *env, Expr *e) {                     \
    if(length(e) < 2)                                       \
        return error("'" name "' expects two arguments");   \
    double a = atof(get_text(e->car));                      \
    double b = atof(get_text(e->cdr->car));                 \
    return boolean(a operator b);                           \
}
COMPARE_FUNCTION(bif_number_eq, "=", ==)
COMPARE_FUNCTION(bif_gt, ">", >)
COMPARE_FUNCTION(bif_ge, ">=", >=)
COMPARE_FUNCTION(bif_lt, "<", <)
COMPARE_FUNCTION(bif_le, "<=", <=)

static Expr *bif_map(Env *env, Expr *e) {
    if(length(e) < 2 || !is_procedure(e->car) || !is_list(e->cdr->car))
        return error("'map' expects a procedure and a list");
    Expr *f = e->car, *result = NULL, *last = NULL;
    for(e = e->cdr->car; e; e = e->cdr) {
        Expr *a = cons(rc_retain(e->car), NULL);
        Expr *res = apply(env, f, a);
        rc_release(a);
        if(is_error(res)) {
            if(result) rc_release(result);
            return res;
        }
        list_append1(&result, res, &last);
    }
    return result;
}

static Expr *bif_filter(Env *env, Expr *e) {
    if(length(e) < 2 || !is_procedure(e->car) || !is_list(e->cdr->car))
        return error("'filter' expects a procedure and a list");
    Expr *f = e->car, *result = NULL, *last = NULL;
    for(e = e->cdr->car; e; e = e->cdr) {
        Expr *a = cons(rc_retain(e->car), NULL);
        Expr *res = apply(env, f, a);
        rc_release(a);
        if(is_error(res)) {
            if(result) rc_release(result);
            return res;
        } else if(is_true(res))
            list_append1(&result, rc_retain(e->car), &last);
        rc_release(res);
    }
    return result;
}

static Expr *bif_append(Env *env, Expr *e) {
    if(length(e) < 2 || !is_list(e->car) || !is_list(e->cdr->car))
        return error("'append' expects two lists");
    Expr *x, *result = NULL, *last = NULL;
    for(x = e->car; x; x = x->cdr)
        list_append1(&result, rc_retain(x->car), &last);
    for(x = e->cdr->car; x; x = x->cdr)
        list_append1(&result, rc_retain(x->car), &last);
    return result;
}

void add_c_function(Env *global, const char *name, c_function function) {
    env_put(global, name, cfun(function));
}

#define TEXT_LIB(g,t) do {Expr *x = eval_str(g, t); assert(!is_error(x));rc_release(x);} while(0)

Env *global_env() {
    Env *global = env_create(NULL);

    add_c_function(global, "write", bif_write);
    add_c_function(global, "display", bif_display);
    add_c_function(global, "apply", bif_apply);
    add_c_function(global, "cons", bif_cons);
    add_c_function(global, "car", bif_car);
    add_c_function(global, "cdr", bif_cdr);
    add_c_function(global, "list", bif_list);
    add_c_function(global, "list?", bif_is_list);
    add_c_function(global, "length", bif_length);
    add_c_function(global, "null?", bif_is_null);
    add_c_function(global, "symbol?", bif_is_symbol);
    add_c_function(global, "pair?", bif_is_pair);
    add_c_function(global, "procedure?", bif_is_procedure);
    add_c_function(global, "number?", bif_is_number);
    add_c_function(global, "boolean?", bif_is_boolean);
    add_c_function(global, "equal?", bif_equal);
    add_c_function(global, "eq?", bif_eq);
    add_c_function(global, "zero?", bif_is_zero);
    add_c_function(global, "not", bif_not);
    add_c_function(global, "+", bif_add);
    add_c_function(global, "-", bif_sub);
    add_c_function(global, "*", bif_mul);
    add_c_function(global, "/", bif_div);
    add_c_function(global, "%", bif_mod);     /* TODO: I don't think real scheme uses the % operator */
    add_c_function(global, "=", bif_number_eq);
    add_c_function(global, ">", bif_gt);
    add_c_function(global, "<", bif_lt);
    add_c_function(global, ">=", bif_ge);
    add_c_function(global, "<=", bif_le);
    add_c_function(global, "map", bif_map);
    add_c_function(global, "filter", bif_filter);
    add_c_function(global, "append", bif_append);
    
    TEXT_LIB(global,"(define (fold f i L) (if (null? L) i (fold f (f (car L) i) (cdr L))))");
    TEXT_LIB(global,"(define (fold-right f i L) (if (null? L) i (f (car L) (fold-right f i (cdr L)))))");
    TEXT_LIB(global,"(define (reverse l) (fold cons '() l))");
    TEXT_LIB(global,"(define (range a b) (if (= a b) (list b) (cons a (range (+ a 1) b))))");
    TEXT_LIB(global,"(define (nth n L) (if (or (null? L) (< n 0)) '() (if (= n 1) (car L) (nth (- n 1) (cdr L)))))");

    env_put(global, "pi", nvalue(M_PI));

    return global;
}
