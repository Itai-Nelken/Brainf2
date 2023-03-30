# Brainf2
An optimizing brainfuck compiler and interpreter.

## Features
* Folding optimization (e.g. `++++` is optimized to `+:4`).
* Translation to C for faster execution.

## Usage
```
Usage: ./brainf [options]
Options:
    [code]    Execute code directly from the first argument.
    -f [file] Execute a file.
    -c [file] Compile a file to C code.
    -o        Optimize the program.
    -d        Dump the compiled (and optimized if '-o' set) instructions.
```

## Compiling
```shell
git clone https://github.com/Itai-Nelken/brainf2.git
cd brainf2
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```
The executable will be in the `build` folder and will be named `brainf` (not `brainf2`).

## Name origin (or why is there `2` in the name?)
I have already written [`brainf`](https://github.com/Itai-Nelken/brainf), so this improved version is version 2, hence `brainf2`.
