# Skeem

An interpreter for a subset of [Scheme][scheme] implemented in C,
with [reference counting][refcnt] for memory management.

It has a simplified C API so that it can be embedded into other programs
to allow you to apply [Greenspun's tenth rule][greenspun] with ease.

Skeem is licensed under the terms of the MIT license. See the [LICENSE](LICENSE) 
for details.

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

[lispy]: http://norvig.com/lispy.html
[lispy2]: http://norvig.com/lispy2.html
[krig]: https://github.com/krig/LISP
[refcnt-c]: https://xs-labs.com/en/archives/articles/c-reference-counting/
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
