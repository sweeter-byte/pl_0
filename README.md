# PL/0 Compiler

<div align="center">

```

     ██████╗ ██╗      ██╗ ██████╗      ██████╗ 
     ██╔══██╗██║     ██╔╝██╔═████╗    ██╔════╝ 
     ██████╔╝██║    ██╔╝ ██║██╔██║    ██║      
     ██╔═══╝ ██║   ██╔╝  ████╔╝██║    ██║      
     ██║     ██████╔╝    ╚██████╔╝    ╚██████╗ 
     ╚═╝     ╚═════╝      ╚═════╝      ╚═════╝ 
```

**A modern PL/0 compiler and interpreter with Clang-style diagnostics**

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)]()

</div>

---

## ✨ Features

- **One-pass Compilation**: Syntax-directed translation with integrated code generation
- **Clang-style Diagnostics**: Beautiful error messages with source location, code snippets, and fix suggestions
- **Stack-based Virtual Machine**: Complete interpreter with static chain for nested scopes
- **Double-buffered Lexer**: Efficient tokenization with 4096-byte buffers and sentinel characters
- **Rich CLI Options**: Flexible output control for debugging and learning
- **Colorful Output**: ANSI color support for better readability (can be disabled)

## 🚀 Quick Start


## 📖 Usage


## 📁 Project Structure


## 🔤 PL/0 Language

PL/0 is a simplified Pascal-like language designed by Niklaus Wirth for teaching compiler construction.

### Grammar (EBNF)

```ebnf
<prog>      → program <id>；<block>
<block>     → [<condecl>][<vardecl>][<proc>]<body>
<condecl>   → const <const>{,<const>};
<const>     → <id>:=<integer>
<vardecl>   → var <id>{,<id>};
<proc>      → procedure <id>（[<id>{,<id>}]）;<block>{;<proc>}
<body>      → begin <statement>{;<statement>} end
<statement> → <id> := <exp>
            | if <lexp> then <statement> [else <statement>]
            | while <lexp> do <statement>
            | call <id>（[<exp>{,<exp>}]）
            | <body>
            | read (<id>{,<id>})
            | write (<exp>{,<exp>})
<lexp>      → <exp> <lop> <exp> | odd <exp>
<exp>       → [+|-]<term>{<aop><term>}
<term>      → <factor>{<mop><factor>}
<factor>    → <id> | <integer> | (<exp>)
<lop>       → = | <> | < | <= | > | >=
<aop>       → + | -
<mop>       → * | /
```

### Example Program

```pascal
program factorial;
var n, result;

procedure fact(x);
var temp;
begin
    if x <= 1 then
        result := 1
    else
    begin
        temp := x;
        call fact(x - 1);
        result := temp * result
    end
end;

begin
    read(n);
    call fact(n);
    write(result)
end
```

## ⚙️ Virtual Machine

The PL/0 virtual machine is a stack-based architecture with:

### Registers
| Register | Description |
|----------|-------------|
| `P` | Program counter (next instruction address) |
| `B` | Base pointer (current activation record) |
| `T` | Top of stack pointer |
| `I` | Current instruction register |

### Instruction Set
| Instruction | Format | Description |
|-------------|--------|-------------|
| `LIT` | `LIT 0, a` | Push constant `a` onto stack |
| `OPR` | `OPR 0, a` | Execute operation `a` (see below) |
| `LOD` | `LOD L, a` | Load variable at level `L`, offset `a` |
| `STO` | `STO L, a` | Store top of stack to variable |
| `CAL` | `CAL L, a` | Call procedure at address `a` |
| `INT` | `INT 0, a` | Allocate `a` stack spaces |
| `JMP` | `JMP 0, a` | Unconditional jump to `a` |
| `JPC` | `JPC 0, a` | Jump to `a` if top of stack is 0 |
| `RED` | `RED L, a` | Read input to variable |
| `WRT` | `WRT 0, 0` | Write top of stack to output |

### OPR Operations
| Code | Operation | Description |
|------|-----------|-------------|
| 0 | RET | Return from procedure |
| 1 | NEG | Negate top of stack |
| 2 | ADD | Addition |
| 3 | SUB | Subtraction |
| 4 | MUL | Multiplication |
| 5 | DIV | Division |
| 6 | ODD | Test if odd |
| 7 | — | (unused) |
| 8 | EQ | Equal |
| 9 | NE | Not equal |
| 10 | LT | Less than |
| 11 | GE | Greater or equal |
| 12 | GT | Greater than |
| 13 | LE | Less or equal |

### Activation Record
```
┌─────────────┐ ← T (top)
│   Locals    │
├─────────────┤
│     SL      │  Static Link (lexical parent)
├─────────────┤
│     DL      │  Dynamic Link (caller's base)
├─────────────┤
│     RA      │  Return Address
└─────────────┘ ← B (base)
```

## 🎨 Diagnostics

The compiler provides Clang-style error messages with helpful suggestions:

```
example.pl0:5:12: error: undefined identifier 'x'
    result := x + 1;
              ^
    Did you mean 'n'?

example.pl0:8:5: error: expected ';' after statement
    y := 10
        ^
        ;
```

## 🧪 Testing


## 🛠️ Build Options

## 📚 References

- Wirth, N. (1976). *Algorithms + Data Structures = Programs*
- Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2006). *Compilers: Principles, Techniques, and Tools* (2nd ed.)
- 陈火旺 等. (2000). *程序设计语言编译原理* (第3版). 国防工业出版社

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

<div align="center">

*There's no case that can't be solved, and no code that can't be compiled.*

Made with ❤️ by Maoyin Ran @ NUAA

</div>