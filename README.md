# Halide-elements
Elemental code snippets written in Halide language.


## Directory tree
```
.
├─ include  # Header files including Halide Func libraries and utils for the tests.
└─ src      # Test Modules
    └─ <Module>  # A module test directory.
        ├─ \<Module>_generator.cc  # A Halide Generator definition for the module code.
        └─ \<Module>_test.cc       # Test code for the module.
```


## Build and test
Set the following environment variables.
 - `HALIDE_ROOT`  : Path to the Halide root directory.
 - `HALIDE_BUILD` : Path to the Halide build directory.

### Unit test

```
$ cd src/<Module>
$ make
$ ./<Module>_test
```

### All test
 ```
 $ ./testall
 ```
