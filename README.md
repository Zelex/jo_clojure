# jo_lisp

Author Jon Olick. @JonOlick on twitter. 

A Fast, Embeddable Clojure language implementation in C. 

Just started the project, nothing is done, everything is messy. Will clean up before version 1.0.

The near-term goal is to re-make clojure in an embedable form which is as fast or faster than the JVM implementation. 

The long-term goal is to add types/functions for machine learning, matrices, tensors, etc... And specifically try to do automatic parallelization. 

# Currently:
* Native implementation. Can be as fast as or faster than original Clojure (which uses Java's JVM). 
* Parses code into native structures, then executes. Essentially interpreted. 
* Lazy sequences
* Startup time is ridiculously fast by comparison
* Implementations of persistent lists, vectors, maps (WIP: set, etc).

# Differences:
* Not entirely complete implementation (yet)

# Future:
* Native types and operations for Matrices and Tensors. 
* Focus on automatic parallelization in general, and explicit when "you know what you are doing". 
* Software Transactional Memory

# Compile/install:

Unix:
Run `make` to build and `make install` to install to `/usr/local/bin` (might require `sudo`)

Windows:
Run `c.bat`
