# cbed

Reusable code for personal projects.

## About

This is an aggregation and refinement of code used throughout the author's personal work. Starting new projects was becoming cumbersome as old code needed to be tracked down or ported from different languages. Creating a reusable library out of all this code seemed the most logical solution to the problem.

The decision to write the library in C was made for the following reasons:
- It can easily be used alongside other programming languages.
- The language is supported on practically anything.
- To help the author become more familiar with writing C code.
- To see how hard it would be to replace parts of the C standard library.
- As a fun challenge!

As of now, there is little to no documentation. Comments are scarce and hardly any code has been thoroughly tested. All code here is subject to breaking changes and should be used with caution, if at all. This project is shared in the hopes that someone might find it interesting or educational. If it should inspire others to make similar libraries of their own, all the better.

The project was renamed from "ceabed" to the more compact name "cbed" to make the library name both easier to remember and type out.

## Examples

The examples folder contains a collection of source files with simple demonstrations on how to use various features of the library. To build the examples, clone the repository, enter the directory and run the provided makefile:

```
git clone https://github.com/tspike2k/cbed
cd cbed
make
```

The examples will then be compiled into the bin folder.

## License

Source code is licensed under the [Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt).
