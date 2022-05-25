# tracy
C debug stack tracer

## Why use it? Pros and Cons

Gnu libc already provides a `backtrace()` function (see
[here](https://www.gnu.org/software/libc/manual/html_node/Backtraces.html)).
However, there are a few disadvantages to it:
 * it is not portable: the function is offered by `glibc`, but it's not
   to be found in `musl` which has gained a lot of ground in embedded
   Linux environments.
 * it requires additional compilation flags e.g. you have to use the
   `-rdynamic` linker flag.
 * it's not as allmighty as you'd think: for instance, it's not able
   to expose `static` functions.
 * to add to the previous point, the output is not as informative as
   you might like: the addresses of the functions are printed, but not
   the names. The names can be obtained with some extra work, though.

Using `tracy` on the other hand has the following advantages:
 * no extra linker or compiler flags are necessary
 * the output is more informative, showing the function names
 * it is not compiler or platform specific and can be used anywhere

But it has some significant drawbacks as well:
 * it requires more manual work and functions must be explicitly
   decorated to make `tracy` aware of them. That is to say, given
   functions `a`, `b`, and `c`, if `a` calls `b` and `b` calls `c` but
   only `c` is decorated, then as far as `tracy` knows only `c` is
   ever on the call stack.

These disadvantages might not be as significant as you might think:
 * for one thing, perhaps you're _interested_ in figuring out if one
   function ultimately ends up calling another. In that case, the
   ability to decorate only certain functions may actually be handy.
 * while the whole call stack is not revealed, having an application
   crashing inside a third-party library that perhaps can't be modified 
   won't help much anyway. You'll probably only want/be able to fix a problem
   in the calling function rather than the third party library
   function. In other words, you only see the part of the callstack
   that has functions in your code -- that you can modify and fix --
   which is typically what's of interest and where the bugs are.

For a much more powerful library, see [libunwind](https://www.nongnu.org/libunwind/).

## Example Output

Calling `__tracy_dump` will produce a stack trace like this:
```
-----------------------------------
=== Stack trace of 'test' ===
-----------------------------------
  ^
  | 6          *** f5(), example/test.c: L44
  | 5          *** f4(), example/test.c: L36
  | 4          *** f3(), example/test.c: L28
  | 3          *** f2(), example/test.c: L20
  | 2          *** f1(), example/test.c: L12
  | 1          *** main(), example/test.c: L54
```
where 
 * `test` is the name of the program
 * `f5()`, `f4()` etc are names of functions
 * `example/test.c` is the source file where `__tracy_dump` was called
 * `L44`, `L20` etc is the line number where `__tracy_dump` was called
 * the line number on the left is the nesting or recursion level. The
   stack starts with the `main` function and ultimately goes as deep
   as `f5()` on the call stack (_as far as tracy is aware_).

Additionally, `tracy` automatically registers a signal handler for
`SIGSEGV`, `SIGBUS`, and `SIGINT`. If a signal is caught, the trace
log at that point is dumped, along with some information germaine to
the signal that terminated the process, then the process is
`exit`ted.

```
^^^^^ 'test' terminated with signal 11 (Segmentation fault), caused by error code 1 (Operation not permitted)
-----------------------------------
=== Stack trace of 'test' ===
-----------------------------------
  ^
  | 5          *** f4(), example/test.c: L36
  | 4          *** f3(), example/test.c: L28
  | 3          *** f2(), example/test.c: L20
  | 2          *** f1(), example/test.c: L12
  | 1          *** main(), example/test.c: L56
```

## Usage
    
 - To use `tracy`, you must first initialize it with `__tracy_init`.
 - Conversely, at the very end, all the associated resources can be freed
   with `__tracy_destroy`.
 - each function must be decorated to make `tracy` aware of it.
   Specifically, you must _push_ the function onto the tracelog when
   you enter it (using `__tracy_push`) and then pop it off when
   returning. Use `TRETURN` instead of `return` for this purpose.
 - a stack trace log can be dumped at any point with `__tracy_dump`.
   Alternatively, it will be dumped automatically when a signal is
   caught, as mentioned above.

```C
#include <stdio.h>

#include "tracy.h"

static void f1(void);
static void f2(void);
static void f5(void);

void f1(void){
    __tracy_push;

    f2();

    TRETURN;
}

void f2(void){
    __tracy_push;

    f3();

    TRETURN;
}

void f5(void){
    __tracy_push;
    
    __tracy_dump;

    TRETURN;
}

int main(int argc, char *argv[]){
    __tracy_init;
    __tracy_push;
    f1();

    __tracy_destroy;
}
```

### Enabling and Disabling tracy

`tracy` functionality can be enabled by defining `TRACY_DEBUG_MODE`
and disabled by leaving that undefined in the C preprocessor. If
disabled, all the overhead is gone: all the macros (which are all
simply wrappers around functions) evaluate to nothing in that case.
The exception is of course `TRETURN` which will simply be replaced
with `return` , as usual, foregoing any call to `__tracy_pop`.

### Log location

`Tracy` can write the tracelog dump to either `stderr`, a file, or to
`syslog`. 

- It will write to a file if the `TRACY_USE_LOG_FILE`
  environment variable is set. By default, it will log to
  `/tmp/tracy.log` but this can be overwritten by another environment
  variable - `TRACY_LOG_FILE`.

- It will write to syslog if the `TRACY_USE_SYSLOG` environment
  variable is set. NOTE however that this is **not what you want**, in
  almost all cases. This is because the tracelog is multiline but
  syslog is line-oriented. This means newline escapes will _not_ be
  interpreted and as a result everything will end up on a single line,
  rendering it all difficult to parse visually.

- if neither of the aforementioned environment variables is set, by
  default `tracy` will log to stderr.
