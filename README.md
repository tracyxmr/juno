# juno
A blazing fast programming language aimed for safety and simplicity.

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
print("hello, world!");
```

Juno is an interpreted language, so we don't need to define an entry point for our program.
# Building from Source
### Prerequisites
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