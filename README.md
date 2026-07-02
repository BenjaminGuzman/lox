# Lox Interpreter (C++)

This project is an implementation of the **Lox** programming language, based on Robert Nystrom's excellent book *Crafting Interpreters*. 

Written in modern C++, this interpreter aims to provide a solid and extended execution environment for Lox. Alongside the standard language features, it includes custom data structures, memory-efficient parsing techniques, and advanced language constructs like closures and named parameters.

---

## 🌟 Key Architectural Features

### 1. Custom `RealNumber` Implementation
To maintain precise control over fractional arithmetic, this interpreter implements a specialized `RealNumber` class. 
* **Design Philosophy**: This custom representation is used instead of standard IEEE 754 floating-point types (like `double`) specifically to explore, study, and implement custom arithmetic algorithms (e.g., Newton-Raphson method, Goldschmidt division) with a higher degree of control over precision and noise reduction.
* **Internal Representation**: Numbers are split into `integer`, `fractional`, and `n_fractional_digits` to help preserve exact decimal precision.
* **Noise Reduction**: Includes `.reduce_noise()` and `.clean_trailing_zeroes()` methods to help mitigate common floating-point artifacts.

### 2. Iterative Pratt Parser
The parser uses an iterative approach to evaluate expressions and build the Abstract Syntax Tree (AST). By maintaining operator precedence dynamically and avoiding recursion, it safely handles deeply nested expressions while keeping the call stack light.

### 3. Zero-Copy Tokenization
To improve memory efficiency, the scanner makes heavy use of `std::string_view` through a custom `BasicTokenView` template. This provides a lightweight, non-owning view of token data, which helps reduce heap allocations during the tokenization and parsing phases.

### 4. Interactive REPL
The interpreter features a fully functional Read-Eval-Print Loop (REPL). It natively maintains the state of your variables and functions across inputs. By executing single statements line-by-line, it immediately evaluates the expression and prints out the returned underlying type, making it a powerful tool for interactive experimentation.

---

## 📖 Complete Language Documentation

Here is the exhaustive documentation of the Lox language as supported by this interpreter.

### Data Types
The interpreter strictly supports the following data types:
* **Number**: Managed by the custom `RealNumber` class. Example: `42`, `3.1415`.
* **String**: Text wrapped in double quotes. Example: `"Hello, World!"`.
* **Boolean**: `true` or `false`.
* **Nil**: Represents the absence of a value. Example: `nil`.

---

### Variable Declaration & Assignment
Variables are dynamically typed and lexically scoped. They are declared using the `var` keyword.
If not explicitly initialized, variables default to `nil`.

```lox
var a = 10;
var b = "texto";
var emptyVar;     // Evaluates to nil

a = 20;           // Reassignment
```

---

### Operators and Advanced Type Coercion

The interpreter includes advanced implicit type coercion and operator overloading for extreme flexibility, far beyond standard Lox.

#### Arithmetic Operators

* **Addition (`+`)**: 
  - **Number + Number**: Standard mathematical addition.
  - **String + String**: String concatenation.
  - **Boolean + Boolean**: Booleans are implicitly cast to `1` (true) and `0` (false), returning a Number. (`true + true` = `2`).
  - **Number + Boolean** / **Boolean + Number**: The boolean is cast to an integer (`1` or `0`) and added to the number.
  - **String + Boolean**: Converts the boolean to its string representation and concatenates. (`"Value: " + true` = `"Value: true"`).
  - **String + Nil**: Converts `nil` to `"nil"` and concatenates. (`"Value: " + nil` = `"Value: nil"`).

* **Subtraction (`-`)**: 
  - **Unary `-Number`**: Returns negated Number.
  - **Number - Number**: Standard mathematical subtraction.
  - **Boolean - Boolean**: Booleans cast to `0/1` and subtracted.
  - **Number - Boolean** / **Boolean - Number**: Boolean cast to `0/1` and subtracted against the number.

* **Multiplication (`*`)**: 
  - **Number * Number**: Standard mathematical multiplication.
  - **String * Number** or **Number * String** *(String Repetition)*: Multiplies a string, repeating it the specified number of times! The number must be a positive integer.
  ```lox
  print "ha" * 3;     // Output: "hahaha"
  print 4 * "lox";    // Output: "loxloxloxlox"
  ```

* **Division (`/`)**: 
  - Defined strictly for `Number / Number` (returns Number).

#### Comparison Operators
* **Relative (`>`, `>=`, `<`, `<=`)**:
  - Strictly defined for comparing `Number` against `Number`.
* **Equality (`==`, `!=`)**:
  - Can compare any two values of the *same type* (String, Number, Boolean, Nil).
  - If you compare values of *different types*, it safely evaluates to `false` (for `==`) or `true` (for `!=`).

```lox
print 5 > 3;           // true
print "lox" == "lox";  // true
print 5 == "5";        // false
```

#### Logical Operators
* **AND (`and`)** / **OR (`or`)**:
  - They perform short-circuit evaluation.
  - They return the *evaluated actual underlying type*, not just a boolean.

```lox
print "hello" or 2; // "hello" (short-circuits)
print nil and "lox"; // nil
```

---

### Control Flow

#### If / Else
Evaluates a condition and branches execution. The condition relies on "truthiness" (everything is true except `nil` and `false`).

```lox
var age = 20;
if age >= 18 {
    print "Adult";
} else {
    print "Minor";
}
```

#### While Loop
Executes a block of code repeatedly while the condition evaluates to truthy.

```lox
var count = 0;
while count < 5 {
    print count;
    count = count + 1;
}
```

#### For Loop
A standard `for (init; condition; increment)` loop.
**Important**: All three components (`init`, `condition`, `increment`) are strictly optional. 
- If `condition` is omitted, it defaults to `true` (infinite loop).
- If `init` or `increment` are omitted, they default to a `no-op` (do nothing).
- The loop establishes a new isolated stackframe scope.

```lox
// Standard usage
for (var i = 0; i < 5; i = i + 1) {
    print i;
}

// Optional parameters usage (acts like a while loop)
var j = 0;
for (; j < 3; ) {
    print j;
    j = j + 1;
}

// Infinite loop (omitted condition evaluates to true)
// for (;;) { ... } 
```

---

### 👻 Syntax Quirks
Due to the custom iterative Pratt parser implementation, there are a few interesting edge cases where the language allows syntax that differs from traditional C-style languages:

1. **Optional Parentheses for `if` and `while` Conditions**:
   Because `if` and `while` are parsed as operators that naturally expect an expression operand, you are **not required** to wrap the condition in parentheses. This makes the language slightly more fluid if you prefer less punctuation.
   ```lox
   // Completely valid in this interpreter:
   if true print "Always executes!";
   
   var x = 0;
   while x < 3 {
       print x;
       x = x + 1;
   }
   ```
   *Note: This does not apply to `for` loops, which strictly expect parentheses `()` for their 3-part grouping.*

2. **Block Braces `{}` are Optional for Single Statements**:
   Any control flow block naturally accepts a single expression. You only need `{}` when you want to group multiple expressions.
   ```lox
   if 10 > 5 print "Greater"; else print "Smaller";
   ```

3. **Multi-Type Return in `and` / `or`**:
   As mentioned above, `and`/`or` do not coerce their return types to booleans; they return the actual last evaluated operand.

---

### Functions and Advanced Paradigms

Functions in this implementation are incredibly versatile, acting as first-class citizens.

#### Basic Declaration & Return
Declared with either the `fun` or `func` keyword and returns a value (or `nil` by default) with `return`.

```lox
// Both declarations are valid!
fun multiply(a, b) {
    return a * b;
}

func multiply(a, b) {
    return a * b;
}
print multiply(3, 4); // 12
```

#### Default Arguments
Functions support **default arguments**. 
Default argument expressions are evaluated **at call time**, not at definition time. 
This means if you use a mutable variable or a function call as a default parameter, it will always compute the most 
up-to-date value when the function is invoked!

```lox
var counter = 0;
fun incrementCounter() {
    counter = counter + 1;
    return counter;
}

fun printScore(score = incrementCounter()) {
    print "Score: " + score;
}

printScore(); // Score: 1
printScore(); // Score: 2
printScore(100); // Score: 100
```

#### Higher-Order Functions & Closures
Functions capture their surrounding lexical environment (closures). You can return functions from functions.
* **Function Chaining**: The interpreter supports chained function calls natively. If a function returns another 
* function, you can execute it immediately by appending another set of parentheses.

```lox
fun createGreeter(greeting) {
    fun greeter(name) {
        print greeting + ", " + name + "!";
    }
    return greeter;
}

// Normal usage
var sayHi = createGreeter("Hi");
sayHi("Alice"); // Output: Hi, Alice!

// Direct Chaining!
createGreeter("Hello")("Bob"); // Output: Hello, Bob!
```

#### Named Parameters
You can invoke functions using named parameters via the `:` operator. This allows you to pass arguments in any order, which greatly improves code readability.

```lox
fun computeStats(health, mana, stamina) {
    print "Health: " + health;
    print "Mana: " + mana;
}

// Order doesn't matter when using named parameters!
computeStats(stamina: 100, health: 50, mana: 200);
```

---

### Global Native Functions
The runtime environment comes with built-in utility native functions pre-loaded in the global scope:

* `clock()`: Returns the exact current time in seconds since the Epoch.
* `delete(varName)`: Manually removes a variable from the current stack frame execution environment, allowing for explicit memory/state control.

```lox
var start = clock();
// do some heavy work...
var end = clock();
print "Time taken: ";
print end - start;

var temp = "data";
delete(temp); // temp no longer exists in scope
```

---

## 🔮 Future Improvements / Areas for Enhancement
While this interpreter is highly capable, there are still a few areas planned for future improvements:

* **Tail Call Optimization (TCO)**: Currently, deep recursion can still grow the host C++ stack when executing function bodies. Implementing tail recursion optimization would allow for infinite recursive loops without stack overflows.
* **Multi-Return Support**: The AST and internal token structures have groundwork laid for `return a, b;` and `var a, b = f()`, but full interpreter support needs to be finalized.
* **Enhanced Standard Library**: Expanding the global native functions to include standard file I/O operations, random number generation, and math libraries.

---
