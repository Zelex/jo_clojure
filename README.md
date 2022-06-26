# jo_clojure

Author Jon Olick. @JonOlick on twitter. 

A Fast, Embeddable Clojure language implementation in C/C++. 

Just started the project, nothing is done, everything is messy. Will clean up before version 1.0.

The near-term goal is to re-make clojure in an embedable form. 

The long-term goal is to add types/functions for machine learning, matrices, tensors, etc... 

# Currently:
* Native implementation of entire core lib.  
* Parses code into native structures (AST), then executes. Essentially interpreted - though does some clever things here and there. 
* Lazy sequences
* Startup time is ridiculously fast by comparison
* Implementations of persistent lists, vectors, hash-map, hash-set.
* Software Transactional Memory (STM)
* Atoms
* IO

# Differences:
* IO is very different than clojure's IO cause I didn't like the clojure IO functions AT ALL.
* Software Transactional Memory (STM) now works seamlessly with Atoms - thus refs are only useful if you need the history feature?
* Not entirely complete implementation (yet). There is a LOT to do.
* See TODO.md for information on what is left to do.

# Future:
* Compile to C/C++ code so you can compile a native executable. 
* Native types and operations for Matrices and Tensors. 
* Automatic Parallelization.

# Compile/install:

Unix:
Run `make` to build and `make install` to install to `/usr/local/bin` (might require `sudo`)

Windows:
Run `c.bat` for Microsoft C/C++ compiler. Run `make` for mingw gcc.
