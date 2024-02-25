# Notes about the alternative parser

## Introduction

The aim of the parser is to provide access to the abstract syntax tree (AST) of an input program and to allow for the manipulation of the AST. The parser is implemented in `src/jac/machine/parser/`.

The parser implements a subset of the ECMAScript 2023 specification. The supported features are described later in this document.

In contrast to the ECMAScript grammar, comments are part of the token stream. Docstrings (i.e., comments preceding a declaration) are saved in the AST. They will allow for supplying argument and return types of objects and functions, which can be used for optimization.


## Unsupported features

- Hashbang is not properly ignored
- Regular expressions
- Template literals are parsed as strings
- Incorrectly terminated legacy octal literals are not detected
- Unicode is not supported in identifiers (only alphanumeric, _, $, not beginning with a digit)


## Non-standard features

- Any numeric literal suffix is saved in the token stream (not just `n` for BigInt)
  - The suffix matches the following regular expression: `([0-9a-zA-Z]_?)+`
