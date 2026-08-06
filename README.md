<div align="center">

  <div>
    <img src="https://img.shields.io/badge/-C-A8B9CC?style=for-the-badge&logo=c&logoColor=black" alt="C" />
    <img src="https://img.shields.io/badge/-Compiler_Design-4B275F?style=for-the-badge&logo=cachet&logoColor=white" alt="Compiler Design" />
    <img src="https://img.shields.io/badge/-Stack_Based_VM-2E3440?style=for-the-badge" alt="Stack-based VM" />
  </div>

  <h3 align="center">AtomC</h3>

  <p align="center">
    A complete compiler + virtual machine for a small C-like teaching language, written from scratch in C —
    no lexer/parser generators, no external libraries.
  </p>

</div>

## 📋 Overview

AtomC implements the full pipeline of a language implementation, end to end:

```text
source (.c)
   │
   ▼
Lexer            → tokens
   │
   ▼
Parser           → recursive-descent parsing, driving domain/type analysis as it goes
   │
   ▼
Domain & Type Analysis   → symbol table, scoping, function/struct resolution, type checking
   │
   ▼
Code Generation   → bytecode for a custom instruction set
   │
   ▼
Virtual Machine   → stack-based execution engine that runs the generated code
```

Every stage is hand-written: tokenizing, recursive-descent parsing, a symbol table with proper domain
(scope) push/pop semantics, type checking with implicit conversions, bytecode generation, and a stack-based
VM with its own instruction set and calling convention (function frames, `FP`-relative locals, `CALL`/`RET`).

## 🧠 What it demonstrates

- Manual **lexical analysis**: hand-rolled tokenizer with its own token stream (`lexer.c/h`)
- **Recursive-descent parsing** for a C-subset grammar: declarations, structs, functions, control flow,
  expressions with correct precedence (`parser.c`)
- **Domain (scope) and type analysis** performed alongside parsing: a real symbol table with nested domains,
  struct member resolution, function signature checking (`ad.c/h`)
- **Code generation** into a custom bytecode with automatic type-conversion insertion where needed (`gc.c/h`)
- A **stack-based virtual machine** with a documented instruction set (`OP_ADD_I`, `OP_CALL`, `OP_ENTER`,
  `OP_JF`/`OP_JT`, `OP_FPLOAD`/`OP_FPSTORE`, type-specific load/store/convert ops, etc.) and its own function
  call frames (`vm.c/h`)
- Basic **garbage-collection scaffolding** (`gc.c`) alongside the domain/type/codegen logic

## 🏗️ Project structure

```text
lexer.c / lexer.h      Tokenizer
parser.c / parser.h    Recursive-descent parser (drives domain & type analysis)
ad.c / ad.h            Domain analysis: symbol table, types, scoping
gc.c / gc.h            Code generation helpers (conversions, rvalues)
vm.c / vm.h            Bytecode instruction set + stack-based VM execution
utils.c / utils.h      Shared helpers (file loading, error reporting)
main.c                 Entry point: tokenize → parse → run
tests/                 Sample AtomC programs used to exercise each stage
```

## ▶️ Running it

```bash
# Build (MSVC project files are included, or compile directly with gcc)
gcc -o atomc.exe lexer.c parser.c ad.c gc.c vm.c utils.c main.c

# main.c loads a test program (e.g. tests/testgc.c) and:
#  1. tokenizes it            → written to tokens-output.txt
#  2. parses + analyzes it    → builds the symbol table and generates bytecode
#  3. runs it on the VM       → prints the program's output to the console
```

Swap the file loaded in `main.c` (`loadFile("tests/testgc.c")`) to run any of the other sample programs in
`tests/`, or write a new `.c` file using AtomC's supported subset (functions, structs, `int`/`double`/`char`,
arrays, `if`/`while`, arithmetic and comparisons).

## 📚 Context

Built as a compiler-construction project — the goal was to implement every stage of a language toolchain
without relying on tools like flex/bison, to actually understand how a symbol table, a type checker, and a
bytecode VM interact.
