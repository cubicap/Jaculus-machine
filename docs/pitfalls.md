# Pitfalls

## Only one context per Machine
From the beginning, the design of Jaculus-machine was to have only one context per Machine instance. The honest reason is my lack of knowledge
of QuickJS and JavaScript internals at the time. Now it starts to show it was a wrong decision and proves to be a limitation in some cases.

For example, in REPL, all exceptions should be caught and reported to standard output. When starting REPL from a JavaScript program, the main
program should crash on unhandled exceptions, whereas the REPL should not. Implementation of this would require REPL and the main program to
be executed in separate contexts to distinguish between their behavior regarding exception handling.


## Unhandled promise rejections not being reported
This is a limitation of QuickJS (might even be called a bug). QuickJS does have a mechanism for reporting unhandled promise rejections,
but it reports some false positives. Consider this example:

```js
new Promise((resolve, reject) => {
    reject(null);
}).then(() => {
    console.log("ok");
}).catch(() => {
    console.log("error");
});
```

The promise is created and immediately rejected. At that moment, the promise does not have a rejection handler, and thus QuickJS reports
an unhandled promise rejection. However, the handler is added before the promise goes out of scope and handles the rejection.

To fix this, diving into the divine magic of QuickJS internals would be required, which would mean a lot of work.
As this does not pose any problem for running good JavaScript code and is problematic only when running bad code, it is not a priority.


## Conversion of JavaScript value to int

Standard JavaScript allows the dynamic conversion of values to number type. This means that the string `"123"` can be converted to the number `123`
without specifying it explicitly. JavaScript numbers, however, may also contain values such as `NaN`. `Nan` is, among other uses, used to indicate
that a conversion to a number failed. QuickJS function for converting a value to `int32_t` does not check for `NaN` and, on invalid conversion,
returns `0` instead.
