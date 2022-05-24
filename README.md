# tracy
C debug stack tracer

# Example Output

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
