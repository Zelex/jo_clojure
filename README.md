# basic

Author Jon Olick. @JonOlick on twitter. 

A Fast, Embeddable Clojure-like language implementation in C/C++. 

The near-term goal is to re-make clojure in an embedable form. 

The long-term goal is to add types/functions for machine learning, tensors, etc... 

# Currently:
* Native implementation of almost entire core lib. See TODO.md
* Parses code into native structures (AST), then executes. Essentially interpreted - though does some clever things here and there. 
* Lazy sequences
* Startup time is ridiculously fast by comparison to clojure
* Implementations of persistent lists, vectors, hash-map, hash-set, matrix
* Software Transactional Memory (STM)
* Atoms
* IO

# Differences:
* IO is very different than clojure's IO cause JVM Clojure's IO functions are practically non-existant. 
* Software Transactional Memory (STM) now works seamlessly with Atoms - thus refs are only useful if you need the history feature?
* Not entirely complete implementation (yet). There is a LOT to do.
* See TODO.md for information on what is left to do.

# Future:
* Compile to C/C++ code so you can compile a native executable. 
* Save/Restore program state (continue later, saved in binary format). 
* Native types and operations for Matrices and Tensors. 
* Automatic Parallelization.
* sorted-set
* sorted-map
* refs
* agents
* other etc....

# Compile/install:

Unix:
Run `make` to build and `make install` to install to `/usr/local/bin` (might require `sudo`)

Windows:
Run `c.bat` for Microsoft C/C++ compiler. Run `make` for mingw gcc.
