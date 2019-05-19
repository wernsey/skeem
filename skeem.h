/**
 * # Skeem
 *
 * An interpreter for a subset of [Scheme][scheme] implemented in C,
 * with [reference counting][refcnt] for memory management.
 *
 * It has a simplified C API so that it can be embedded into other programs
 * to allow you to apply [Greenspun's tenth rule][greenspun] with ease.
 *
 * It is based mostly on Peter Norvig's [lispy][] interpreter with some
 * elements from [part 2][lispy2]. The reference counter was inspired by
 * [this C implementation][refcnt-c]
 *
 * ![toc-]
 *
 * [scheme]: https://en.wikipedia.org/wiki/Scheme_(programming_language)
 * [lispy]: http://norvig.com/lispy.html
 * [lispy2]: http://norvig.com/lispy2.html
 * [refcnt]: https://en.wikipedia.org/wiki/Reference_counting
 * [refcnt-c]: https://xs-labs.com/en/archives/articles/c-reference-counting/
 * [greenspun]: https://en.wikipedia.org/wiki/Greenspun%27s_tenth_rule
 */

#ifndef SKEEM_H
#define SKEEM_H
#if defined(__cplusplus) || defined(c_plusplus)
extern "C"
{
#endif

/**
 * ## Expression objects
 *
 * All values and expressions in Skeem are represented in `SkObj` objects.
 *
 *     typedef struct SkObj SkObj;
 *
 * The `SkObj` structure represents a Skeem expression or value, that can be
 * either a list of other objects or an atomic value.
 */
struct SkObj;
typedef struct SkObj SkObj;
struct SkEnv;
typedef struct SkEnv SkEnv;

typedef void (*ref_dtor_t)(void *);

/**
 * ### Cons cells
 *
 * #### `SkObj *sk_cons(SkObj *sk_car, SkObj *sk_cdr);`
 *
 * Constructs a new [cons][] cell with the given `car` and `cdr`
 *
 * [cons]: https://en.wikipedia.org/wiki/Cons
 */
SkObj *sk_cons(SkObj *sk_car, SkObj *sk_cdr);

/**
 * #### `int sk_is_null(SkObj *e);`
 *
 * Tests whether the given expression `e` is an empty list `'()`.
 */
int sk_is_null(SkObj *e);

/**
 * #### `int sk_is_cons(SkObj *e);`
 *
 * Tests whether the given expression `e` is a cons cell.
 */
int sk_is_cons(SkObj *e);

/**
 * #### CAR and CDR functions
 *
 * * `SkObj *sk_car(SkObj *e)` - returns the car of the cons cell, `NULL`
 *    if the expression is not a cons cell
 * * `SkObj *sk_cdr(SkObj *e)` - returns the cdr of the cons cell, `NULL`
 *    if the expression is not a cons cell
 * * `sk_caar(e)` - equivalent to `sk_car(sk_car(e))`, implemented as a macro.
 * * `sk_cadr(e)` - equivalent to `sk_car(sk_cdr(e))`, implemented as a macro.
 * * `sk_cdar(e)` - equivalent to `sk_cdr(sk_car(e))`, implemented as a macro.
 * * `sk_cddr(e)` - equivalent to `sk_cdr(sk_cdr(e))`, implemented as a macro.
 */
SkObj *sk_car(SkObj *e);
SkObj *sk_cdr(SkObj *e);
#define sk_caar(e) sk_car(sk_car(e))
#define sk_cadr(e) sk_car(sk_cdr(e))
#define sk_cdar(e) sk_cdr(sk_car(e))
#define sk_cddr(e) sk_cdr(sk_cdr(e))

/**
 * #### `int sk_is_list(SkObj *e);`
 *
 * Tests whether the given expression `e` ia a valid
 * Skeem list.
 * Valid Skeem lists are either the empty list `'()` or
 * cons cells of which the car is a value and the cdr is a
 * valid list.
 */
int sk_is_list(SkObj *e);

/**
 * #### `int sk_length(SkObj *e);`
 *
 * Returns the number of elements in the given expression `e`,
 * assuming `e` is a valid Skeem list (see `sk_is_list()`)
 */
int sk_length(SkObj *e);

/**
 * ### Symbols
 *
 * #### `SkObj *sk_symbol(const char *sk_value);`
 *
 * Creates a new symbol object with the given string value.
 */
SkObj *sk_symbol(const char *sk_value);

/**
 * #### `int sk_is_symbol(SkObj *e);`
 *
 * Tests whether the given expression `e` is a symbol.
 */
int sk_is_symbol(SkObj *e);

/**
 * ### Values
 *
 * #### `SkObj *sk_value(const char *val);`
 *
 * Creates a new value object with the given string value.
 */
SkObj *sk_value(const char *val);

/*
 * #### `SkObj *sk_value_o(const char *val);`
 *
 * Creates a new value object with the given string value.
 * It will take _ownership_ of the pointer passed to it: The
 * pointer passed to it must be allocated on the heap.
 * The interpreter will take responsibility for `free()`ing
 * the memory at some point.
 */
SkObj *sk_value_o(char *val);

/**
 * #### `int sk_is_value(SkObj *e);`
 *
 * Tests whether the given expression `e` is a value.
 */
int sk_is_value(SkObj *e);

/**
 * #### `SkObj *sk_number(double n);`
 *
 * Creates a new value object with the given numeric value.
 *
 * Skeem stores numbers as strings internallyy for technical reasons.
 */
SkObj *sk_number(double n);

/**
 * #### `int sk_is_number(SkObj *e);`
 *
 * Tests whether the given expression `e` is a numeric value.
 */
int sk_is_number(SkObj *e);

/**
 * ### Booleans
 *
 * #### `SkObj *sk_boolean(int val)`
 *
 * Creates a new boolean object with the given value `val` that is either
 * `#t` for non-zero and `#f` for zero.
 */
SkObj *sk_boolean(int val);

/**
 * #### `int sk_is_boolean(SkObj *e);`
 *
 * Tests whether the given expression `e` is a boolean value.
 */
int sk_is_boolean(SkObj *e);

/**
 * #### `int sk_is_true(SkObj *e);`
 *
 * Tests whether the given expression `e` evaluates to true.
 *
 * All Skeem objects that do not evaluate to false (`#f`)
 * are considered true.
 */
int sk_is_true(SkObj *e);

/**
 * ### Functions
 *
 * Skeem handles two types of functions: Lambdas, that are implemented in
 * the Skeem language itself, and CFuns whic are functions implemented in
 * C and then registered with the interpreter so that they can be called
 * from the Skeem language.
 *
 * The built-in functions in the global envronment are all built on top of
 * CFun functions.
 *
 * #### `SkObj *sk_lambda(SkObj *args, SkObj *body)`
 *
 * Creates a new object of type Lambda with pointers to the list of
 * arguments and the body of the lambda.
 */
SkObj *sk_lambda(SkObj *args, SkObj *body);

/**
 * #### `typedef SkObj *(*sk_cfun_t)(SkEnv *env, SkObj *args);`
 *
 * A `sk_cfun_t` is a pointer to a C function that can be called from
 * the Skeem interpreter.
 *
 * The `env` parameter passed to the function is the current execution
 * environment.
 *
 * The `args` parameter is a list of arguments passed to the function
 * from the interpreter.
 *
 * The function should return a _retained_ `SkObj` object that is
 * to be used as the return value of the function.
 */
typedef SkObj *(*sk_cfun_t)(struct SkEnv *env, SkObj *args);

/**
 * #### `SkObj *sk_cfun(sk_cfun fun);`
 *
 * Creates a new object of type CFun with a pointer to the `sk_cfun_t` C function.
 */
SkObj *sk_cfun(sk_cfun_t func);

/**
 * #### `int sk_is_procedure(SkObj *e);`
 *
 * Tests whether the given expression `e` is a callable procedure; that is
 * either a lambda expression or a CFun.
 */
int sk_is_procedure(SkObj *e);

/**
 * ### CData objects
 *
 * CData objects are special objects used carry data from C programs
 * through the interpreter from where they can be passed to CFun functions.
 *
 * Skeem cannot interact with CData objects other than to pass them to
 * functions, so CFun functions need to be created to interact with them.
 * CData objects always evaluates to `#t` when used in boolean expressions,
 * and evaluates to `""` and 0 when used as strings and numbers.
 *
 * #### `SkObj *sk_cdata(void *cdata, ref_dtor_t dtor);`
 *
 * Creates a new object of type CData with a pointer to the data. The pointer
 * can later be retrieved through `sk_get_cdata()`
 *
 * The `dtor` parameter is a pointer to a function that will be called by
 * the reference counter to clean up after the object when all references
 * have been removed.
 */
SkObj *sk_cdata(void *cdata, ref_dtor_t dtor);

/**
 * #### `int sk_is_cdata(SkObj *e);`
 *
 * Tests whether the given expression `e` is a CData object.
 */
int sk_is_cdata(SkObj *e);

/**
 * #### `void *sk_get_cdata(SkObj *e);`
 *
 * Gets the pointer to the data contained within the CData object.
 */
void *sk_get_cdata(SkObj *e);

/**
 * #### `ref_dtor_t sk_get_cdtor(SkObj *e)`
 *
 * Gets the destructor of the CData object. This can be useful for
 * checking whether a CData object is of a specific type.
 *
 * Returns `NULL` if none or if `e` is not a CData object.
 */
ref_dtor_t sk_get_cdtor(SkObj *e);

/**
 * ### Error objects
 *
 * Skeem uses special error type objects to handle errors
 * within the interpreter.
 *
 * Whenever the parser or the interpreter encounters objects
 * of this type it stops immediately and returns the error.
 *
 * #### `SkObj *sk_error(const char *msg)`
 *
 * Creates a new error object with the error message `msg`.
 */
SkObj *sk_error(const char *msg);

/**
 * #### `SkObj *sk_errorf(const char *fmt, ...)`
 *
 * Creates an error object from a `printf()`-style format string;
 * It uses `vsnprintf()` internally.
 */
SkObj *sk_errorf(const char *fmt, ...);

/**
 * #### `int sk_is_error(SkObj *e)`
 *
 * Tests whether the given expression `e` is a CData object.
 */
int sk_is_error(SkObj *e);

/**
 * ### Other functions
 *
 * #### `int sk_equal(SkObj *a, SkObj *b);`
 *
 * Compares two `SkObj` objects `a` and `b` for equality.
 *
 * Returns 1 if they are equal, zero otherwise
 */
int sk_equal(SkObj *a, SkObj *b);

/**
 * #### `char *sk_serialize(SkObj *e);`
 *
 * Serializes the object `e` into a text buffer.
 *
 * The returned buffer is on the heap and needs to be `free()`d after use.
 */
char *sk_serialize(SkObj *e);

/**
 * #### `void sk_write(SkObj *e, FILE *f);`
 *
 * Serializes the object `e` and writes it to the file `f`.
 */
void sk_write(SkObj *e, FILE *f);

/**
 * #### `int sk_check_numeric(const char *s);`
 *
 * Checks the string `s` to see if it can be used as a number
 * inside the interpreter.
 */
int sk_check_numeric(const char *s);

/**
 * #### `const char *sk_get_text(SkObj *e);`
 *
 * Gets the text associated with an expression.
 *
 * Only value, symbol, boolean and error objects have textual
 * representations. Other oject types return an empty string, `""`.
 */
const char *sk_get_text(SkObj *e);

/**
 * ## Environments
 *
 * ### Types
 *
 * #### `typedef struct SkEnv SkEnv;`
 *
 * The `SkEnv` structure is the environment where variables are
 * stored while a script is being executed.
 *
 * An environment is normally linked to a parent environment to manage
 * variables' scope: If a variable can not be found in an environment
 * the interpreter.
 *
 * Use `sk_global_env()` to create a global environment for the
 * interpreter containing the built in functions. `sk_env_create(SkEnv *parent)`
 * can also be used to create environments for specific purposes.
 *
 * Environments are managed through the refeerence counter, and should
 * be freed through `rc_release()` when no longer in use.
 */

/**
 * ### Functions
 *
 * #### `SkEnv *sk_global_env();`
 *
 * Creates a new environment that can be used for the interpreter's
 * global environment, and that has no parent.
 *
 * It populates this global environment with the interpreter's built in
 * functions.
 */
SkEnv *sk_global_env();

/**
 * #### `SkEnv *sk_env_createn(SkEnv *parent, unsigned int size);`
 *
 * Creates a new environment, with an internal hash table with `size` slots
 * and its `parent` set as its parent environment.
 *
 * `size` _must_ be a power of 2 because `size-1` is used as a bitmask internally
 */
SkEnv *sk_env_createn(SkEnv *parent, unsigned int size);

/**
 * #### `SkEnv *sk_env_create(SkEnv *parent);`
 *
 * Creates a new environment, with `parent` set as its parent environment.
 *
 * The internal hash table has a small number of slots meant for local
 * variables and function parameters.
 */
SkEnv *sk_env_create(SkEnv *parent);

/**
 * #### `SkObj *sk_env_put(SkEnv *env, const char *name, SkObj *e);`
 *
 * Puts a new variable named `name` in the environment and associates
 * it with the expression object `e`.
 *
 * `sk_env_put()` does **not** call `rc_retain()` on `e`. This makes it easier
 * to insert objects created through functions like `sk_value()` and `sk_cons()`
 * to an environment, but it is something to be aware of.
 */
SkObj *sk_env_put(SkEnv *env, const char *name, SkObj *e);

/**
 * #### `SkObj *sk_env_get(SkEnv *env, const char *name);`
 *
 * Retrieves an expression object named `name` from the environment `env`.
 * It will recurse through an environment's parents to search for the variable.
 *
 * Returns `NULL` if there is no such variable in the environment.
 */
SkObj *sk_env_get(SkEnv *env, const char *name);

/**
 * ## Parser and Interpreter
 *
 * ### Parsing
 *
 * #### `SkObj *sk_parse(const char *text);`
 *
 * Parses a string into a `SkObj` object
 *
 * On a error it will return an error object (see `sk_is_error()`)
 * with t
 */
SkObj *sk_parse(const char *text);

/**
 * #### `SkObj *sk_parse_stmts(const char *text)`
 *
 * Parses several statements; wraps them in a `(begin stmt1 stmt2...)`.
 * This function is meant for parsing files or strings containing several
 * statements.
 *
 * It will also return an error object on failure; see `sk_is_error()`.
 */
SkObj *sk_parse_stmts(const char *text);

/**
 * ### Evaluating
 *
 * #### `SkObj *sk_eval(SkEnv *env, SkObj *e)`
 *
 * Evaluates an expression from a `SkObj` object `e` within a
 * given environment `env`.
 *
 * It returns an object containing the result of the evaluated expression.
 * The result must be released through `rc_release()` after use.
 *
 * The result can be checked for errors with `sk_is_error()`.
 */
SkObj *sk_eval(SkEnv *env, SkObj *e);

/**
 * #### `SkObj *sk_eval_str(SkEnv *global, const char *text);`
 *
 * Parses and evaluates a text string.
 *
 * It is equivalent to calling `sk_parse()` on `text`, then calling
 * `sk_eval()` on the parsed `SkObj`. It checks the parsed `SkObj` for
 * errors before
 */
SkObj *sk_eval_str(SkEnv *global, const char *text);

/**
 * #### `SkObj *sk_apply(SkEnv *env, SkObj *f, SkObj *a);`
 *
 * Applies the function `f` to a list of aruments `a` and
 * returns the result.
 *
 * `f` must be a CFun or a lambda; that is `sk_is_procedure(f)` must
 * hold.
 */
SkObj *sk_apply(SkEnv *env, SkObj *f, SkObj *a);

/**
 * ### Reference counter
 *
 * #### `typedef void (*ref_dtor_t)(void *);`
 *
 * The destructor for a reference counted object. It is called on
 * an object when all its references have been released. Its purpose
 * is to free up any resources used by an object, including releasing
 * any references to other objects that this one may hold.
 *
 * #### `void *rc_alloc(size_t size);`
 *
 * Allocates a reference counted block of memort
 */
void *rc_alloc(size_t size);

/**
 * #### `void rc_release(void *p);`
 *
 * Releases a reference to a reference counted block of memory.
 * If the reference count drops to zero, the dtor is called and
 * the memory is freed.
 */
void rc_release(void *p);

/**
 * #### `void *rc_retain(void *p);`
 *
 * Retains a pointer to a reference counted block (by incrementing
 * the object's reference count).
 */
void *rc_retain(void *p);

/**
 * #### `void rc_set_dtor(void *p, ref_dtor_t dtor);`
 *
 * Sets the destructor of a reference counted object.
 */
void rc_set_dtor(void *p, ref_dtor_t dtor);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
#endif /* ifdef SKEEM_H */
