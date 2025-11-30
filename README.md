# juno
A blazing fast programming language aimed for safety and simplicity, built with LLVM.

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
fn main() -> int {
    println("hello, world!");
}
```

Juno is a compiled language, so we need to define an entry point for our program. You can do so by defining the `main` function, this is where all of your code will start. The return value of the `main` function is the only function where the return value is implicit, it will default to `0`.
# Building from Source
### Prerequisites
* LLVM
* C++23 or higher
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