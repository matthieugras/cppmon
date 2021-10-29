## CPPMon

A monitor for metric first-order temporal properties written in C++.

### Build instructions

#### Build dependencies

- [Clang](https://clang.llvm.org) compiler (including sanitizer support for debug builds)
- [LLVM linker (lld)](https://lld.llvm.org)
- Recent version of the [Conan](https://conan.io) package manager
- [CMake](https://cmake.org)
- [Ninja](https://ninja-build.org)
- Static version of `libstdc++`, which you can install with your system's package manager

#### Build commands

To configure the project, switch to the project root and run the command`./setup_release.sh` for release builds
or `./setup_debug.sh` for debug builds, respectively. This will create the folder `builddir` (or `builddir_debug` for
debug builds), compile and install all the needed dependencies and configure the project in the created folder.

After configuring the build, run `ninja -C <configured_folder> <target>` to compile the wanted target. The possible
targets are:

- **cppmon** to build the monitor
- **dbgen** to build the synthetic test case generator
- **testexe** to build tests
- specify no target if you want to build everything

The built binaries can be found in the folder `<configured_folder>/bin`.

#### Notes

Debug builds are mainly useful to find bugs in the monitor's code. Debug builds enable assertions that check internal
invariants, `libstdc++` assertions (to detect out-of-bounds reads/writes as well as incorrect use of STL iterators), and
Clang sanitizers (to detect undefined behavior and invalid memory reads/writes).

Release builds are compiled with full optimizations (including LTO) for the native CPU architecture. They also contain
debuginfo for profiling.

### Usage

#### Dependencies

CPPMon needs the Monpoly monitor to parse and verify formulas.

#### Input format

CPPMon reads preprocessed formulas in json format. The preprocessing is done using the Monpoly monitor. We aim to
support the same language fragment as Monpoly without the extensions introduced by VeriMon+.

Signatures have the same format as those accepted by Monpoly.

Event traces should be in simple Monpoly format, i.e. the variant of the Monpoly trace input format without partial
databases.

#### Output format

CPPMon uses the same output format as Monpoly. Additionally, we aim to produce *identically formatted* output. In
particular, the satisfactions at every timepoint are lexicographically sorted.

#### Running the monitor

1. Preprocess the formula file in monpoly format:

```
./monpoly -verified -dump_verified -sig <signature_file>  -formula <formula_file>
```

This will pretty-print the formula in json format to stdout. You can redirect the output to a file. You can also add
other flags supported by monpoly like e.g. `-neg` (if you wish to negate the formula) to the command.

2. Execute the monitor:

```
./cppmon --sig <signature_file> --formula <formula_json_file> --log <input_trace>
```

CPPMon will (hopefully) print all satisfactions of the formula given the input trace to stdout.

### Documentation

See the `docs` folder.

### References

CPPMon is based on the following works:
- [Schneider, Joshua, et al. *"A formally verified monitor for metric first-order temporal
logic*](https://link.springer.com/chapter/10.1007/978-3-030-32079-9_18)
- [Basin, David, et al. *"A formally verified, optimized monitor for metric first-order dynamic
logic*](https://link.springer.com/chapter/10.1007/978-3-030-51074-9_25)
