# Skeem

An interpreter for a small subset of [Scheme][].

[Scheme]: https://en.wikipedia.org/wiki/Scheme_(programming_language)

### Random notes

* `define` affects the global environment, wheras `set!` affects the local environment. [Norvig][chap22] said that `define` and `set!` are equivalent.
  * My implementation lets you `define` a variable more than once. The second `define` just replaces the first value.
* Don't count on arithmetic to be too accurate, due to all the `atof`ing and `snprintf`ing going on behind the scenes.
  * Actually I can just start using doubles now. Storing everything in strings sounded like a good idea at the start, but it doesn't seem that way any more.

### TODOs

* Missing operators:
  * `cond` special form
  * `append`, `reverse`
  * `foldr` and `foldl`
  * I can't see myself bothering with `eqv?` 
    * (Note to self: Racket's documentation on [booleans](https://docs.racket-lang.org/reference/booleans.html))
* Racket allows `[]` to be used interchangeably with `()`; see [here](https://stackoverflow.com/a/41417968/115589).

## References

- Peter Norvig's [lispy][] interpreter and [part2][lispy2]
  - [Chapter 22](https://github.com/norvig/paip-lisp/blob/master/docs/chapter22.md) of Peter Norvig's book [Paradigms of Artificial Intelligence Programming][paip] (PAIP) also talks about Scheme extensively
- <http://www.r6rs.org/>
- The[krig/LISP][krig] project on GitHub inspired me to get started. It also gave me some cool ideas.
- I've used the [The Racket Reference](https://docs.racket-lang.org/reference/index.html) for guidance on how some functions should be implemented
- Likewise for the [Towards a Standard Library](https://en.wikibooks.org/wiki/Write_Yourself_a_Scheme_in_48_Hours/Towards_a_Standard_Library) of the **Write Yourself a Scheme in 48 Hours** wikibook.
- Wikipedia entry for [cons](https://en.wikipedia.org/wiki/Cons)
- [The Scheme Programming Language](https://www.scheme.com/tspl4/)
- [Simply Scheme: Introducing Computer Science](https://people.eecs.berkeley.edu/~bh/ss-toc2.html)
- Some Scheme tutorials that I used for reference:
  - [Yet Another Scheme Tutorial](http://www.shido.info/lisp/idx_scm_e.html)
  - [Scheme for Java Programmers](http://cs.gettysburg.edu/~tneller/cs341/scheme-intro/index.html)
  - <https://www.st.cs.uni-saarland.de/edu/config-ss04/scheme-quickref.pdf>
  - <http://www.nada.kth.se/kurser/su/DA2001/sudata16/examination/schemeCheatsheet.pdf>
  - [Teach Yourself Scheme in Fixnum Days](https://ds26gte.github.io/tyscheme/index.html)

[lispy]: http://norvig.com/lispy.html
[lispy2]: http://norvig.com/lispy2.html
[krig]: https://github.com/krig/LISP

### Further Reading

Articles/links that might come in useful in the future

* [Chapter 23](https://github.com/norvig/paip-lisp/blob/master/docs/chapter23.md) of [PAIP][paip] explains Compiling Lisp
* [Beautiful Racket](https://beautifulracket.com/introduction.html)
* <http://www.civilized.com/files/lispbook.pdf>

[paip]: https://github.com/norvig/paip-lisp

## Design Considerations

* One of the issues with my error handling is how to track the line number in the source file in the `Expr` objects as the file is being parsed, like I do for other interpreters I've written, but this could be a bit problematic given that many (most?) of the `Expr` objects are created at run-time. Also, once you start adding lambdas into the mix.
  * It is possible to track the line number in the parsed `Expr`s and then copy that line number to run-time `Expr`s.
  * An alternative idea is to store a pointer to the problematic `Expr` as part of the error which you can just `dump()` in the error report. The error message will then just say something like `"error: divide by zero near (/ a 0)"`
    * This will again be problematic if the expression with the error is complicated.

### Numbers

Some mathematical expressions are problematic, like `(+ 0.1 1)`.

I wanted to add a `NUMBER` type of `Expr` with a `double` value, but the main reason I didn't do that is `get_text()` function will need some buffer to which to print the text value to.
The alternative is to have `get_text()` modify its parameter directly, but that violates the immutability of the `Expr` objects.

My `check_numeric()` function originally looked like the below, but the newer versions just checks for something that starts with an optional '+' or '-' and a digit:

```
int check_numeric(const char *c) {
	int ds = 0, de = 0;
	if(strchr("+-", *c))
		c++;
	if(!*c) return 0;
	while(isdigit(*c) || (*c == '.' && !ds++) || ((*c == 'e' || *c == 'E') && !de++)) {
        if(*c == 'e' || *c == 'E') {
            c++;
            if(*c == '-' || *c == '+')
                c++;
        } else
            c++;
    }
	return !*c;
}
```

### Garbage collection

I've gone back and forth about which type of garbage collector the interpreter should have: A reference counter (RC), or a mark-and-sweep (MS). The issue is that I want this interpreter to be used as an extension language to C programs, so I value simplicity in the C API. Both kinds of garbage collectors have trade-offs.

I'm leaning towards using the reference counter for garbage collection:

* RC is easier to reason about, and more predictable.
* RC has less overhead and simpler to implement.
* At the moment I'm willing to accept the caveat that RC doesn't play well with circular structures, and will just advise the user against them.
* The MS collector I've been using is not reentrant because it uses several global variables for tracking the roots, etc.
 * Non-reentrancy is a problem why again?
 * It wouldn't be too difficult to make the collector reentrant...
 * ...but you won't be able to share objects between `Environment`s. I can't remember why this is a problem exactly, but it had something to do with, say, the NPCs in a CRPG each having their own `Environment`, but having to share objects from time to time when they interact
* The MS has a drawback that the `Expr` structures of the parsed program itself has to be marked every GC iteration, but they won't be sweeped while the program is still in use.
 * A generational collector would alleviate this problem, but it comes with additional complexity, for example, the case where an object in an older generation acquires a pointer to a object in a younger generation
 * An alternative solution is to track the program's `Expr` objects with a separate MS, but this would require the MS to be made reentrant, with the same problems outlined above.
* My RC also has some non-reentrant features for debugging memory leaks, but the problems with sharing objects between interpreter environments

It seems the biggest drawback of using RC is that you can't have closures: The lambda has to keep a reference to the environment in which it was created, which in turn eventually has a reference to the global environment. So if you store a lambda with a closure in a global variable you'll have a circular reference, and therefore a leak.

_A possible way around this is to release the `Env` of the closure's parent pointer and set it to null after the lambda was called, so that the `Env` does not have reference to the global environment; So when you look up a variable, you have two separate paths: the path through the chain of environments, and the closure environment. Investigate further._

One problem with RC is that if you want to do something like
`env_put(env, "foo", atom("bar"))` in the C API you have to be careful with how
the environment takes ownership of the `atom()` pointer.

### Hash tables

I'm using DJB hash, same as [krig][], but DJB is itself [a bit controversial](http://dmytry.blogspot.com/2009/11/horrible-hashes.html).

I use [this advice](http://www.cse.yorku.ca/~oz/hash.html) that says you can use XOR instead of ADD. 
[Here's an example](https://cr.yp.to/cdb/cdb.txt) of djb using the XOR version,
but I didn't see anyone else using it like that.

The reason I'm sticking with DJB, and not going something like [MurmurHash](https://en.wikipedia.org/wiki/MurmurHash) is that 
I don't want the interpreter's LOC to be dominated by the hash function (which I'll admit is technically a terrible 
reason, but still).

[This StackOverflow answer](https://stackoverflow.com/a/13809282/115589) explains the constants in DJB.

Here is a blog about the [Python dictionary implementation](https://www.laurentluce.com/posts/python-dictionary-implementation/).

