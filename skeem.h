/**
 * Skeem - a small interpreter for a small subset of Scheme
 */
struct Expr;
typedef struct Expr Expr;

struct Env;
typedef struct Env Env;

typedef Expr *(*c_function)(Env *, Expr *);

Env *env_create(Env *parent);

Expr *env_get(Env *env, const char *name);

Expr *env_put(Env *env, const char *name, Expr *e);

int equal(Expr *a, Expr *b);

void write(Expr *e);

#ifdef NDEBUG
Expr *symbol(const char *value);
#else
#  define symbol(v) symbol_(v, __FILE__, __LINE__)
Expr *symbol_(const char *value, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *value(const char *val);
#else
#  define value(v) value_(v, __FILE__, __LINE__)
Expr *value_(const char *val, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *error(const char *val);
#else
#  define error(v) error_(v, __FILE__, __LINE__)
Expr *error_(const char *val, const char *file, int line);
#endif

Expr *errorf(const char *fmt, ...);

#ifdef NDEBUG
Expr *boolean(int val);
#else
#  define boolean(v) boolean_(v, __FILE__, __LINE__)
Expr *boolean_(int val, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *nvalue(double i);
#else
#  define nvalue(v) nvalue_(v, __FILE__, __LINE__)
Expr *nvalue_(double i, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *cons(Expr *car, Expr *cdr);
#else
#  define cons(car, cdr) cons_(car, cdr, __FILE__, __LINE__)
Expr *cons_(Expr *car, Expr *cdr, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *lambda(Expr *args, Expr *body);
#else
#  define lambda(args, body) lambda_(args, body, __FILE__, __LINE__)
Expr *lambda_(Expr *args, Expr *body, const char *file, int line);
#endif

#ifdef NDEBUG
Expr *cfun(c_function func);
#else
#  define cfun(f) cfun_(f, __FILE__, __LINE__)
Expr *cfun_(c_function func, const char *file, int line);
#endif

int check_numeric(const char *c);

Expr *parse(const char *text);

/* Parses several statements; wraps them in a `(begin stmt1 stmt2...)`.
 * Meant for parsing whole files.
 */
Expr *parse_stmts(const char *text);

const char *get_text(Expr *e);

int is_nil(Expr *e);

int is_list(Expr *e);

int is_symbol(Expr *e);

int is_cons(Expr *e);

int is_boolean(Expr *e);

int is_true(Expr *e);

int is_procedure(Expr *e);

int is_number(Expr *e);

int is_error(Expr *e);

int length(Expr *e);

Expr *apply(Env *env, Expr *f, Expr *p);

Expr *eval(Env *env, Expr *e);

Expr *eval_str(Env *global, const char *text);

void add_c_function(Env *global, const char *name, c_function function);

Env *global_env();
