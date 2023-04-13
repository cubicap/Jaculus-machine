# Pitfalls

## Only one context per Machine
From the beginning, the design of Jaculus-machine was to have only one context per machine. The honest reason is my lack of knowledge
of QuickJS and JavaScript internals at the time. Now it starts to show it was a bad decision and proves to be a limitation in some cases.

It seems like it can be fixed relatively easily, but it is not a priority at the moment.


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

The promise is created and immediately rejected. At that moment, the promise does not have a rejection handler and thus QuickJS reports
an unhandled promise rejection. However, the handler is added before the promise goes out of scope and handles the rejection.

To fix this, diving into the divine magic of QuickJS internals would be required, which would mean a lot of work.
As this does not pose any problem for running good JavaScript code and is problematic only when running bad code, it is not a priority.
