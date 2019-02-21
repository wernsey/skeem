# Skeem

An interpreter for a subset of [Scheme][scheme] implemented in C,
with [reference counting][refcnt] for memory management.

It has a simplified C API so that it can be embedded into other programs
to allow you to apply [Greenspun's tenth rule][greenspun] with ease.

It is based mostly on Peter Norvig's [lispy][] interpreter with some
elements from [part 2][lispy2]. The reference counter was inspired by
[this C implementation][refcnt-c]

[scheme]: https://en.wikipedia.org/wiki/Scheme_(programming_language)
[lispy]: http://norvig.com/lispy.html
[lispy2]: http://norvig.com/lispy2.html
[refcnt]: https://en.wikipedia.org/wiki/Reference_counting
[refcnt-c]: https://xs-labs.com/en/archives/articles/c-reference-counting/
[greenspun]: https://en.wikipedia.org/wiki/Greenspun%27s_tenth_rule

## References

- Peter Norvig's [lispy][] interpreter and [part2][lispy2]
  - [Chapter 22](https://github.com/norvig/paip-lisp/blob/master/docs/chapter22.md) of
    Peter Norvig's book [Paradigms of Artificial Intelligence Programming][paip] (PAIP)
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

[lispy]: http://norvig.com/lispy.html
[lispy2]: http://norvig.com/lispy2.html
[krig]: https://github.com/krig/LISP
[refcnt-c]: https://xs-labs.com/en/archives/articles/c-reference-counting/

### Further Reading

Articles/links that might come in useful in the future

* [Chapter 23](https://github.com/norvig/paip-lisp/blob/master/docs/chapter23.md) of [PAIP][paip] explains Compiling Lisp
* [Beautiful Racket](https://beautifulracket.com/introduction.html)
* <http://www.civilized.com/files/lispbook.pdf>

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

### TODOs

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
  * [x] `(string=? str1 str2...)` and `(string<? str1 str2)` - _all the other comparisons can be made from =? and <?_
  * [x] `(string-upcase str)`
  * [x] `(string-downcase str)`
  * [x] `(string-replace str from to [all=#t])`
    * You need the `filter` in this line `(define x (filter (lambda (s) (> (string-length? s) 0)) (string-split (readfile 'test2.txt) "\r\n")))`
      because the DOS `\r` characters really breaks them when we don't have a `string-replace` function.  \
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
* [ ] The hash-tables in the environments need a variable capacity. The global environment needs a bit more
  slots than the current 32, while 32 slots seems like overkill for a typical lambda.
* Skeem can't have a proper `call/cc` for the same reasons it can't have closures (hint: reference counting).  \
  It might be possible to a _escaping continuation_ version of `call/cc` similar to how [lispy2][] does it.  \
  It would however require the addition of a new type of value that needs to be checked for in `sk_eval()` 
  every time `sk_eval()` calls itself recursively.  \
  The implementation of `call/cc` would need some way to identify itself if you were to have nested `call/cc`s.
  I think using the value of the `SkObj *` pointer passed to the `call_cc` CFun will be sufficient.  \
  Here's [another link][callcc]
* [x] **(6) Procedures with arbitrary number of arguments** from [lispy2][] shouldn't be too difficult to implement.
  * [x] It now supports the `(define x (lambda args (display args)))` syntax.
  * [x] If I ever implement the `(x . y)` syntax, the `(arg1 arg2 . rest)` syntax should also be doable.
* [x] Implement [dotted pairs](https://ds26gte.github.io/tyscheme/index-Z-H-4.html#node_sec_2.2.3)
* [ ] What is the normal Scheme hash table structure? Maybe I can repurpose the `Env` objects for it.

Here is the Awk script to renumber the tests in test/test.scm

    awk '{if($0 ~ /Test [0-9]+/)gsub(/Test [0-9]+/,"Test " (++i));print}' test/test.scm


[callcc]: https://ds26gte.github.io/tyscheme/index-Z-H-15.html#node_chap_13

### Bugs

* ~~This input broke the REPL interpreter: `(nth 3 (range 10 20))^Z`~~ - should've just pressed enter.

### Numbers

All values are stored as strings internally. 
This means that (a) precision is negatively affected, and (b) numeric performance is bad because a value needs to
be converted to a double (through `atof()`) if it is used as a number, and then converted back into a string
(through `snprintf()`) when it is stored.

A solution would be a `NUMBER` type `SkObj` with a `double` value, but the `sk_get_text()` function will
need some buffer to which to print the text value to.
Having `sk_get_text()` change its parameter directly is out of the question because it violates the
immutability of the `SkObj` objects.

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

### Hash tables

Skeem uses the DJB hash, which is [a bit controversial](http://dmytry.blogspot.com/2009/11/horrible-hashes.html).
It uses [the XOR variant](http://www.cse.yorku.ca/~oz/hash.html), but was chosen for its simplicity.
[This StackOverflow answer](https://stackoverflow.com/a/13809282/115589) explains the constants in DJB.
