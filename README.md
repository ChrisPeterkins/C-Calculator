# C Calculator

A collection of calculator implementations in C, each under 1000 lines of code with no external dependencies (except standard system libraries).

## Project Overview

This project demonstrates different approaches to building a fully-featured calculator in C:
- Expression parsing with proper operator precedence
- Mathematical functions (trigonometry, logarithms, etc.)
- Error handling
- Different user interface paradigms

## Implementations

### 1. CLI Calculator (calculator.c) - 487 lines
Basic command-line REPL calculator with a simple prompt interface.

**Features:**
- Interactive prompt
- Full expression evaluation
- Help command
- Clear command

**Build and Run:**
```bash
gcc -o calculator calculator.c -lm
./calculator
```

### 2. Terminal UI Calculator (calculator_tui.c) - 536 lines
Enhanced terminal interface with visual elements and history.

**Features:**
- Visual border and layout
- Expression history (navigate with arrow keys)
- Live cursor editing
- Color-coded output
- No external dependencies

**Build and Run:**
```bash
gcc -o calculator_tui calculator_tui.c -lm
./calculator_tui
```

**Controls:**
- Arrow keys: Navigate history and cursor
- Enter: Calculate expression
- Escape: Clear current expression
- Ctrl+D: Exit

### 3. Native macOS GUI (calculator_mac.m) - 489 lines
Native Cocoa application with graphical interface.

**Features:**
- Native macOS window
- Clickable button interface
- Keyboard input support
- Visual feedback for results and errors

**Build and Run:**
```bash
clang -framework Cocoa -o calculator_mac calculator_mac.m
./calculator_mac
```

### 4. X11 GUI Calculator (calculator_gui.c) - 547 lines
Cross-platform GUI using X11 (for Linux/Unix systems with X11).

**Features:**
- Graphical button interface
- Mouse and keyboard support
- Display screen for expressions

**Build and Run:**
```bash
gcc -o calculator_gui calculator_gui.c -lX11 -lm
./calculator_gui
```

Note: Requires X11 development libraries installed.

## Supported Operations

### Basic Arithmetic
- Addition (+)
- Subtraction (-)
- Multiplication (*)
- Division (/)
- Modulo (%)
- Power (^)

### Functions
- Trigonometric: sin(), cos(), tan()
- Square root: sqrt()
- Logarithm: log()
- Exponential: exp()
- Absolute value: abs()

### Constants
- pi - Mathematical constant pi
- e - Euler's number

### Features
- Parentheses for grouping
- Proper operator precedence (PEMDAS)
- Floating-point arithmetic
- Error handling (division by zero, domain errors, etc.)

## Expression Examples

```
2 + 3 * 4         = 14
(2 + 3) * 4       = 20
sin(pi/2)         = 1
sqrt(16)          = 4
2^8               = 256
log(e)            = 1
10 % 3            = 1
```

## Architecture

All implementations share the same core calculator engine:

1. **Lexer/Tokenizer**: Converts input strings into tokens
2. **Parser**: Recursive descent parser implementing operator precedence
3. **Evaluator**: Computes results while parsing

The parsing follows standard mathematical operator precedence:
1. Parentheses
2. Functions (sin, cos, sqrt, etc.)
3. Exponentiation (^)
4. Multiplication, Division, Modulo (*, /, %)
5. Addition, Subtraction (+, -)

## Error Handling

The calculator handles various error conditions:
- Division by zero
- Square root of negative numbers
- Logarithm of non-positive numbers
- Syntax errors
- Mismatched parentheses
- Unknown operators or functions

## Building All Versions

```bash
# CLI version
gcc -o calculator calculator.c -lm

# Terminal UI version
gcc -o calculator_tui calculator_tui.c -lm

# macOS GUI version (macOS only)
clang -framework Cocoa -o calculator_mac calculator_mac.m

# X11 GUI version (requires X11)
gcc -o calculator_gui calculator_gui.c -lX11 -lm
```

## Requirements

- C compiler (gcc or clang)
- Math library (-lm flag)
- For macOS GUI: macOS with Cocoa framework
- For X11 GUI: X11 development libraries

## License

This project is provided as-is for educational purposes.