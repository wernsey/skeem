# Notes

## TODOs

* [x] Escape sequences in string literals!
* [x] The way `VALUE`s are written in `sk_serialize()` should escape special characters.
    * You can use the new `buffer_appendn()` function with a `s` as a `char[2]` and `len = 1`
* Special forms:
    * [x] `let*` - see [here](http://www.cs.utexas.edu/ftp/garbage/cs345/schintro-v13/schintro_59.html)
    * [ ] `cond` special form
* String functions. I've been looking at [Racket's](https://docs.racket-lang.org/reference/strings.html),
  but I'm not going to do mutable strings:
    * [x] `(string-length str)` from which `(non-empty-string? x)` can be implemented
    * [x] `(substring str start [end=(string-length str)])`
    * [x] `(string-append str...)`
    * [x] `(string=? str1 str2...)` and `(string<? str1 str2)` - _all the other comparisons can be made from `=?` and `<?`_
    * [x] `(string-upcase str)`
    * [x] `(string-downcase str)`
    * [x] `(string-replace str from to [all=#t])`
        * You need the `filter` in this line `(define x (filter (lambda (s) (> (string-length? s) 0)) (string-split (readfile 'test2.txt) "\r\n")))`
      because the DOS `\r` characters really breaks them when we don't have a `string-replace` function.
      Fixed: You can now do `(define x (string-split (string-replace (readfile 'test2.txt) "\r" "") "\n"))`
    * [x] `(string-split str [sep=' '])`
    * [x] `(string-trim str)`
    * [x] `(string-contains? s what)`, `(string-prefix? s what)` and `(string-suffix? s what)`
        * [x] Actually, I only need a `(string-find haystack needle)` function in C that returns the index of the needle in the
       haystack, and then these other functions can be defined in terms of that.
* [x] Math functions: `sin`, `cos`, `tan`, `atan`, etc
* [x] If you look at `bif_string_append()` (and `bif_string_split()` and elsewhere) there is clearly a need for an
  API function `sk_value_o(buf)` that takes ownership of the parameter `buf` passed to it.
  It would work like `sk_value(buf)`, but replaces the `strdup()` with a normal assignment.
  This would allow you to remove the `free()` from `bif_string_append()`.
* [x] The hash-tables in the environments need a variable capacity. The global environment needs a bit more
  slots than the current 32, while 32 slots seems like overkill for a typical lambda.
* [ ] Skeem can't have a proper `call/cc` for the same reasons it can't have closures (hint: reference counting).
  It might be possible to a _escaping continuation_ version of `call/cc` similar to how [lispy2][] does it.
  It would however require the addition of a new type of value that needs to be checked for in `sk_eval()`
  every time `sk_eval()` calls itself recursively.
  The implementation of `call/cc` would need some way to identify itself if you were to have nested `call/cc`s.
  I think using the value of the `SkObj *` pointer passed to the `call_cc` CFun will be sufficient.
  Here's [another link][callcc]
* [x] **(6) Procedures with arbitrary number of arguments** from [lispy2][] shouldn't be too difficult to implement.
    * [x] It now supports the `(define x (lambda args (display args)))` syntax.
    * [x] If I ever implement the `(x . y)` syntax, the `(arg1 arg2 . rest)` syntax should also be doable.
* [x] Implement [dotted pairs](https://ds26gte.github.io/tyscheme/index-Z-H-4.html#node_sec_2.2.3)
* [ ] Maybe I can repurpose the `SkEnv` objects for implementing hash tables similar to
  [Racket's hash tables][hashtables].
    * I don't think it is necessary to use anything other than strings as keys (so `SYMBOL`s or `VALUE`s),
    so I only need a `(make-hash)` function.
    * I'll make an exception wrt the immutability of Skeem objects just this one time.
        * So no need for `(make-immutable-hash)`
    * [ ] You might even be able to do the `(hash-set)` function by using the `SkEnv`'s parent.
    * [ ] Serialization should lead to something like `(hash "apple" 'red "banana" 'yellow)`.
      * You don't need to go so far as the ` #hash` form.
    * To implement a function like `(hash-map)` you'll need to be able to iterate through the
    hash table. Here's how my [old hash table does it](https://github.com/wernsey/miscsrc/blob/master/hash.c#L177)
    * See also Racket's [hash table reference][hashref]

Here is the Awk script to renumber the tests in test/test.scm

    awk '{if($0 ~ /Test [0-9]+/)gsub(/Test [0-9]+/,"Test " (++i));print}' test/test.scm

[callcc]: https://ds26gte.github.io/tyscheme/index-Z-H-15.html#node_chap_13
[hashtables]: https://docs.racket-lang.org/guide/hash-tables.html
[hashref]: https://docs.racket-lang.org/reference/hashtables.html
[lispy2]: http://norvig.com/lispy2.html

## Debugging the Reference Counter

Older versions of Skeem used a separate reference counter that has functionality
to check for unreleased references when the interpreter terminates if it was
compiled in debug mode (`NDEBUG` not defined).

I removed this functionality because I want Skeem to be self contained and
not require the separate `refcnt.c` and `refcnt.h`.

I just want to keep these snippets around for reference. In the header:

```
#ifdef NDEBUG
SkObj *sk_symbol(const char *sk_value);
#else
#  define sk_symbol(v) sk_symbol_(v, __FILE__, __LINE__)
SkObj *sk_symbol_(const char *sk_value, const char *file, int line);
#endif
```

Then where the function is declared in the source file:

```
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
```

This allows you to see where a particular call of `sk_symbol()` allocated an object
that wasn't released.

## Hash Tables

Skeem uses the DJB hash, which is [a bit controversial](http://dmytry.blogspot.com/2009/11/horrible-hashes.html).
It uses [the XOR variant](http://www.cse.yorku.ca/~oz/hash.html), but was chosen for its simplicity.
[This StackOverflow answer](https://stackoverflow.com/a/13809282/115589) explains the constants in DJB.

I have a TODO to add hash tables to the language similar to Racket's `(make-hash)` function

Functions I meant to use to iterate through hash tables...

```
static void sk_env_flatten_r(SkEnv *env, SkEnv *into) {
    unsigned int h;
    hash_element *e;
    if(!env) return;
    sk_env_flatten_r(env->parent, into);
    for(h = 0; h <= env->mask; h++) {
        for(e = env->table[h]; e; e = e->next)
            sk_env_put(into, e->name, rc_retain(e->ex));
    }
}
SkEnv *sk_env_flatten(SkEnv *env) {
    SkEnv *into = sk_env_create(NULL);
    sk_env_flatten_r(env, into);
    return into;
}

const char *sk_env_next(SkEnv *env, const char *name) {
    unsigned int h;
    hash_element *e;
    if(!env) return NULL;
    if(!name) {
        for(h = 0; h <= env->mask; h++) {
            if((e = env->table[h]))
                return e->name;
        }
    } else {
        h = hash(name) & env->mask;
        for(e = env->table[h]; e; e = e->next) {
            if(!strcmp(e->name, name)) {
                if(e->next)
                    return e->next->name;
                for(h++; h <= env->mask; h++) {
                    if(env->table[h])
                        return env->table[h]->name;
                }
            }
        }
    }
    return NULL;

    // cant do this: What if the key is in the child _and_ the parent?
    // Althrough I really want to:
    //return sk_env_next(env->parent, name);
}
```

### Further Reading

Articles/links that might come in useful in the future

* [Chapter 23](https://github.com/norvig/paip-lisp/blob/master/docs/chapter23.md) of [PAIP][paip] explains Compiling Lisp
* [Beautiful Racket](https://beautifulracket.com/introduction.html)
* <http://www.civilized.com/files/lispbook.pdf>

[paip]: https://github.com/norvig/paip-lisp