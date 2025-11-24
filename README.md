# juno
A blazing fast scripting language aimed for safety and simplicity.

# Features
* Bytecode virtual machine ([JNVM](https://github.com/tracyxmr/jnvm))
* Variables, simple declarations `let x = 5;`
* Expressions and literals
* Type safety, static typing to catch errors before runtime.
* Memory safe, native safeguards to mitigate against common issues.

# Quick Start
Installation
```bash
# clone the repo
git clone https://github.com/tracyxmr/juno.git
cd juno

# build from source
./build.sh
```

# Examples and Explanations
```
let x = 10;

{
    let y = 15;
}

let z = 20;
```

Juno has a global scope like the large majority of programming languages, each scope in Juno has a predefined start register for any variables declared in that scope. When the compiler leaves the scope, the compiler which take note of the exiting of the scope and overwrite these registers as they're no longer in use. This is one way Juno cleans up variables.

```
@profile {
    let x = 234 * 34562;
    let y = x / 23;
    print(y);
}
```
```
20
Block took 122us, processed 8 instructions.
```

Juno has special keywords and functions prefixed with `@`, for example the `@profile` can prefix a block of code or a functions body, it will the automatically print the time it took to execute the block of code, and how many instructions were executed. This is useful for optimizing your programs and measuring different approaches to a solution.

### Snippets
```
fn foo(a: int) -> int @profile {
    let sum = a * 10;
    print( sum );
}

foo(5);
```

This snippet demonstrates defining a user function, since the body of the function is a block, we can still prefix it with special keywords such as `@profile`, and of course variables are automatically cleaned up.

```
fn foo() -> double { return 2; }
// this should error because foo returns double
// and num expects string
let num: string = foo();
```

Juno comes with a nice type solver for any case, in this example we declare a function which is annotated to return a double, but the `num` variable is annotated with `string`, this is not going to work because the solver expects the return type of `foo` to be `string`, as `num` is the enforcer for this rule.
The error message follows:
```
[juno::solver_error] Type mismatch in variable 'num' declaration: expected 'string', but got 'double'
```

# Building from Source
### Prerequisites
* C++17 or higher
* CMake 3.15 or higher
* A C++ compiler ( GCC, Clang or MSVC )
* Git

### Steps
```bash
# clone the repo
git clone https://github.com/tracyxmr/juno.git
cd juno

# create a build directory
mkdir build && cd build

# configure cmake
cmake ..

# build for debug
cmake --build .

# OR

# build for release
cmake --build . --config Release
```

# Support
If you encounter any issues or have questions, please open an issue on GitHub.