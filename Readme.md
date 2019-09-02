# Skeem

An interpreter for a subset of [Scheme][scheme] implemented in C,
with [reference counting][refcnt] for memory management.

It has a simplified C API so that it can be embedded into other programs
to allow you to apply [Greenspun's tenth rule][greenspun] with ease.

Skeem is licensed under the terms of the MIT license. See the [LICENSE](LICENSE)
for details.

## Example

See [main.c](main.c) for an example of how to use Skeem in a C program.

Skeem has two main data structures:

* `SkObj` is used to represent variables, values and cons cells within the
interpreter. A program is also compiled to a tree of `SkObj` cons cells before
evaluation.
* `SkEnv` represents environments, which are hash tables where functions and
variables are kept.

The first step is to create a global environment where global functions
(including the built-ins) and variables are to be stored.

```
SkEnv *global = sk_global_env();
```

You can add functions or variables to the global environment using
`sk_env_put()`. This example creates a CFun object from `function_foo`
(see below) using `sk_cfun` and adds it to the global environment under
the name `foo`, so that it can be called from Skeem as `(foo)`:

```
sk_env_put(global, "foo", sk_cfun(function_foo));
```

The function `function_foo`'s declaration in C must look like the
`sk_cfun_t` prototype. The example below just returns a string value "foo"

```
static SkObj *function_foo(SkEnv *env, SkObj *args) {
    /* `args` contain the arguments passed to the function
        as a list of cons cells */
    return sk_value("foo");
}
```

You can now evaluate a text string against the global environment:

```
SkObj *result = sk_eval_str(global, "(display \"hello world\")");
```

Alternatively, `sk_parse()` or `sk_parse_stmts()` can be used to parse
a string into a tree of `SkObj` objects which can be evaluated later
through `sk_eval()`.

You can use `sk_is_error()` to check for errors after parsing or evaluation.
If a `SkObj` is a textual object, its text value can be retrieved through `sk_get_text()`. Otherwise `sk_serialize()` can be used to convert the
`SkObj` to a string representation.

```
if(sk_is_error(result)) {
    fprintf(stderr, "error: %s\n", sk_get_text(result));
} else if(!sk_is_null(result)) {
    char *text = sk_serialize(result);
    puts(text);
    free(text);
}
```

You need to tell the reference counter that you are done with the `result`
object:

```
rc_release(result);
```

At the end of your program you need to tell the reference counter
to release the global environment as well:

```
rc_release(global);
```

## References

It is based mostly on Peter Norvig's [lispy][] interpreter with some
elements from [part 2][lispy2]. The reference counter was inspired by
[this C implementation][refcnt-c]

- Peter Norvig's [lispy][] interpreter and [part2][lispy2]
  - [Chapter 22][chap22] of Peter Norvig's book [Paradigms of Artificial Intelligence Programming][paip] (PAIP)
    also talks about Scheme extensively
- <http://www.r6rs.org/>
- I've used the [The Racket Reference](https://docs.racket-lang.org/reference/index.html) for guidance on
  how some functions should be implemented
- Likewise for the [Towards a Standard Library](https://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours/Towards_a_Standard_Library)
  of the **Write Yourself a Scheme in 48 Hours** wikibook.
- Wikipedia entry for [cons](https://en.wikipedia.org/wiki/Cons)
- [The Scheme Programming Language](https://www.scheme.com/tspl4/)
- [Simply Scheme: Introducing Computer Science](https://people.eecs.berkeley.edu/~bh/ss-toc2.html)
- [An Introduction to Scheme and its Implementation](http://www.cs.utexas.edu/ftp/garbage/cs345/schintro-v13/schintro_toc.html)
- Some Scheme tutorials that I used for reference:
  - [Yet Another Scheme Tutorial](http://www.shido.info/lisp/idx_scm_e.html)
  - [Scheme for Java Programmers](http://cs.gettysburg.edu/~tneller/cs341/scheme-intro/index.html)
  - <https://www.st.cs.uni-saarland.de/edu/config-ss04/scheme-quickref.pdf>
  - <http://www.nada.kth.se/kurser/su/DA2001/sudata16/examination/schemeCheatsheet.pdf>
  - [Teach Yourself Scheme in Fixnum Days](https://ds26gte.github.io/tyscheme/index.html)
  - [The Guile Reference Manual](https://www.gnu.org/software/guile/manual/html_node/index.html)
  - <https://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours/Towards_a_Standard_Library>
  - <http://community.schemewiki.org/?fold>
- [Reference counting in ANSI-C][refcnt-c]
- The [krig/LISP][krig] project on GitHub inspired me to get started. It also gave me some cool ideas.

[scheme]: https://en.wikipedia.org/wiki/Scheme_(programming_language)
[lispy]: http://norvig.com/lispy.html
[lispy2]: http://norvig.com/lispy2.html
[refcnt]: https://en.wikipedia.org/wiki/Reference_counting
[refcnt-c]: https://xs-labs.com/en/archives/articles/c-reference-counting/
[greenspun]: https://en.wikipedia.org/wiki/Greenspun%27s_tenth_rule
[krig]: https://github.com/krig/LISP
[paip]: https://github.com/norvig/paip-lisp

## Implementation Notes

* `define` affects the global environment, wheras `set!` affects the local environment. [Norvig][chap22] said that `define` and `set!` are equivalent.
  * My implementation lets you `define` a variable more than once. The second `define` just replaces the first value.
* Like in [Racket](https://stackoverflow.com/a/41417968/115589), square brackets `[]` can be used interchangeably with parentheses `()`.
* Don't count on arithmetic to be too accurate, due to all the `atof`ing and `snprintf`ing going on behind the scenes.
  * Storing everything in strings sounded like a good idea at the start, but it doesn't seem that way any more.
* Skeem has tail call optimization, but not on reference counter:
  If you call `rc_release()` on a long list it will recursively call `rc_release()` on its cdr.
* My functions `string-ascii` and `string-char` are stand-ins for the `char->integer` and `integer->char` functions. See [here][scheme-types].

[scheme-types]: https://ds26gte.github.io/tyscheme/index-Z-H-4.html
[chap22]: https://github.com/norvig/paip-lisp/blob/master/docs/chapter22.md

### Numbers

All values are stored as strings internally. It uses the `"%.17g"` format string for `snprintf` - see 
this article [floating point precision][precision] for more detail.

Numeric performance suffers because a value needs to be converted to a double 
(through `atof()`) if it is used as a number, and then the result of any arithmetic 
needs to be converted back into a string (through `snprintf()`) when it is stored.

A solution would be a `NUMBER` type `SkObj` with a `double` value, but the `sk_get_text()` function will
need some buffer to which to print the text value to.
Having `sk_get_text()` change its parameter directly is out of the question because it violates the
immutability of the `SkObj` objects.

[precision]: https://randomascii.wordpress.com/2012/03/08/float-precisionfrom-zero-to-100-digits-2/

### Garbage collection

Skeem uses a reference counter (RC) for memory management.
A mark-and-sweep (MS) garbage collector was considered, but RC was ultimately chosen for a couple of reasons:

* RC has less overhead and simpler to implement.
* RC allows the C API to be much simpler. Apart from `rc_retain()` and `rc_release()` the user of the C API doesn't
  need to know much about the internals of the collector.
* RC allows the C API to be reentrant. A MS collector would have either required a global variable containing the
  roots or required all the API functions to pass a structure containing the roots around.
* It is difficult to estimate the paramters of MS, like how frequently it should be run, for various workloads.

The biggest drawback of RC is that the interpreter can't have closures. Closures require lambdas to have a
references to the environments in which they were created, which will ultimately, probably, have a reference to the
lamba itself, leading to circular references which the RC can't deal with, and therefore memory leaks.
