# Atom

`jac::Atom` is a type that represents identifiers of a JavaScript properties, variables, functions etc. It can be used in place of `std::string`
for better performance.

## Creating atoms

Atoms can be created from a string using the `jac::Atom::create` method:

```cpp
jac::ContextRef ctx = ...;

jac::Atom atom1 = jac::Atom::create(ctx, "foo");
jac::Atom atom2 = jac::Atom::create(ctx, 123);
```

## Usage

Most functions in Jaculus-machine that take a string identifier as an argument also have an overload that takes a `jac::Atom` argument.

```cpp
jac::ContextRef ctx = ...;

jac::Object global = ctx.getGlobalObject();

jac::Atom atom = jac::Atom::create(ctx, "foo");
jac::Value value = global.get(atom);
```
