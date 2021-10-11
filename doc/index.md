PerceMon       {#mainpage}
========

\tableofcontents


Overview
--------

PerceMon is an online monitoring tool for Spatio-Temporal Quality Logic (STQL), designed
specifically to define properties on perception data.


Installation (from source)
--------------------------

### Requirements

You need:

- [CMake](https://cmake.org/download/) version greater than or equal to 3.5
- [Git](https://git-scm.com/)
- A C++ compiler that supports C++17. See
  [cppreference](https://en.cppreference.com/w/cpp/compiler_support#cpp17) for a
  compiler that supports most of the features.
- (Optional) [Ninja build tool](https://ninja-build.org/) for faster building than
  `make` and for better cross-platform availability.

### Building the library

Note: this assumes a Linux environment with bash/zsh, but there are equivalent commands
in other shells.

Clone the repository and it's submodules:

```shell
$ git clone https://github.com/anand-bala/PerceMon.git
$ cd PerceMon
$ git submodule update --init
```

Then do the CMake dance in the repository root:

```shell
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make -j4
```

Or else, if you have installed Ninja

```shell
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja
$ ninja
```


