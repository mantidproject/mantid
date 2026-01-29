# Tools Overview

```{toctree}
:hidden:

AlgorithmProfiler
```

```{contents}
:local:
```

(_class_maker_py)=
## Creating classes: class_maker.py

To make it faster to create a new cpp class. This is a small python
script located in
[buildconfig/class_maker.py](https://github.com/mantidproject/mantid/blob/main/buildconfig/class_maker.py).
It generates the `.cpp`, `.h`, and test files for a class along with
some code stubs. It can also flesh out more methods for new Algorithms,
using the `--alg` option which also creates a stub user documentation
page `.rst`.

```{literalinclude} class_maker.txt
```

## Moving/Renaming classes: move_class.py

This python script is located in in /buidconfig/. It will move a class
from one subproject to another and/or rename the class. Namespaces and
cmakelists are adjusted. For details, run:

`buildconfig/move_class.py --help`

## Deleting a class: delete_class.py

This python script is located in in /buildconfig/. It will delete a
class from one subproject. CMakeList.txt is adjusted. For details, run:

`buildconfig/delete_class.py --help`

## Profiling

Profiling could mean tracking start-up, the progress of an algoirthm or
performance. See `ProfilingOverview` for more details.

## Leak checking etc

### Linux

[Memcheck](http://valgrind.org/docs/manual/mc-manual.html)

- Keeps track of allocs/deallocs and reports anything missing at exit.
- Slow but thorough
- Useful options to run with
- See [valgrind](ProfilingWithValgrind) for details on how to install

`valgrind --tool=memcheck --leak-check=full --show-reachable=yes --num-callers=20 --track-fds=yes --track-origins=yes --freelist-vol=500000000``[args...]`

### Windows

[Visual Leak Detector](https://vld.codeplex.com/releases)

1.  Setup the additional paths as defined in the readme file
2.  Adjust the configuration file, "C:Program FilesVisual Leak
    Detectorvld.ini" to output to both File and debugger by changing the
    `ReportTo` to

`ReportTo = both`

1.  Add \#include \<vld.h\> to the system.h file in Kernel
2.  Compile everything in debug
3.  Running unit tests should now create a file memory_leak_report.txt
    in the test directory.
4.  IMPORTANT remove the \#include \<vld.ini\> before checking in.

## Thread checking

### [Helgrind](http://valgrind.org/docs/manual/hg-manual.html) or [drd](http://valgrind.org/docs/manual/drd-manual.html)

- Identifies race conditions & dead-locks
- Slow but accurate
- A pain to get working with OpenMP. GCC must be recompiled to use a
  different call to create OMP threads or helgrind/drd cannot "see" the
  thread calls. Use this
  [script](https://github.com/UCSCSlang/Adversarial-Helgrind/raw/master/drd/scripts/download-and-build-gcc)
  to recompile the same version off gcc that is onyour system. The
  script will need editing to change the appropriate variables.

## IWYU

[include what you use](https://code.google.com/p/include-what-you-use/)
(iwyu) is a clang-based tool for determining what include statements are
needed in C/C++ files. Below are instructions for getting it to run with
mantid on linux which is a filled in version of [this
bug](https://code.google.com/p/include-what-you-use/issues/detail?id=164).

1.  Install the software. The version available from system installs
    should be fine (e.g. yum or apt-get).
2.  Get a copy of
    [iwyu_tool.py](https://code.google.com/p/include-what-you-use/source/browse/trunk/iwyu_tool.py)
    which is in the project's repository, but may not be installed if
    you got it from your operating system locations (e.g. yum).
3.  Run `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE`. This will generate
    an extra file, `compile_commands.json`, in your build area which has
    instructions on compiling every file in mantid.
4.  Run `` iwyu_tool.py -p `pwd `` 2\> iwyu.log` to generate the report
    of changes redirecting into the file `iwyu.log`. This will take a
    long time since it is going through the whole repository. If you
    want it for a single file, then supply that as an additional
    argument with full path. Only one file can be supplied at a time.
5.  Run `fix_includes < iwyu.log` and compile the results. Depending on
    how you installed iwyu, the program may be called `fix_includes.py`.
    If it doesn't compile, the most likely suspect is that iwyu included
    a private header. See [iwyu instructions for
    users](https://code.google.com/p/include-what-you-use/wiki/InstructionsForUsers#How_to_Run)
    for ways to handle this. Generally, they suggest deleting the
    offending lines.
6.  Check that your build path didn't make it into source files. Since
    `compile_commands.json` has full paths, iwyu will put full paths in
    the include statements. This will not produce an error on your
    system, but it will on the build servers. The easiest way to check
    is to use [the silver
    searcher](https://github.com/ggreer/the_silver_searcher) to check
    for your username in your source tree.
7.  Enjoy your success.

**Note:** `iwyu` outputs to `stderr` and always returns a failure status
code since it generates no output. The output stream also affects
`iwyu_tool.py`

## Network Related Tools

### Wireshark

Linux distros should have this in their repositories. For other
platforms download it from <https://www.wireshark.org>.

Cross-platform tool for inspecting network packets. This is useful to
troubleshoot many different types of network-related problems.

### Wonder Shaper

Linux only. Install it from your distro's repository.

Wonder Shaper allows the user to limit the bandwidth of one or more
network adapters. This is useful for debugging issues when a network
interface is still active but very slow. More details can be found at
<http://xmodulo.com/limit-network-bandwidth-linux.html>.

## Clang-tidy

Clang-tidy is a set of tools which allows a developer to detect and fix
code which does not follow current best practices, such as unused
parameters or not using range-based for loops. Primarily this is used
for modernising C++ code.

The full list of clang-tidy checks can be seen
[here](https://clang.llvm.org/extra/clang-tidy/checks/list.html).

### Installing

Mantid does not come packaged with clang-tidy; each developer must
download it themselves.

- **Ubuntu**: Run `sudo apt-get install clang clang-tidy` in the command
  line. The `clang` package is needed to get the system headers.
- **Windows**: Download the [Visual Studio
  extension](https://marketplace.visualstudio.com/items?itemName=caphyon.ClangPowerTools).
  Windows can operate clang-tidy from Visual Studio alone and so do not
  need to touch cmake.
- **Conda** install `clang-tools` (for `clang-tidy`) and `clangxx` (for
  system headers and compiler to reduce clang-tidy diagnostic errors)
- Alternatively, download the latest clang-tidy [pre-compiled
  binary](http://releases.llvm.org/download.html). Windows users should
  add to path when prompted.

To make use of the sanitizers on the command line, on needs the re-run

``` sh
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=on .
```

in the build tree. Then symbolically link the `compile_commands.json`
from the build tree into the source tree.

Many IDEs support which is described in the [clang-tidy
documentation](https://clang.llvm.org/extra/clang-tidy/Integrations.html).

### Visual Studio

Once you have installed the clang-tidy extension, Visual Studio will
have additional options under `Tools -> Options -> Clang Power Tools`.
Here you can select which checks to run via a checkbox or by supplying a
custom check string. For example:

    -*,cppcoreguidelines-*,modernize-*

will disable basic default checks (`-*`) and run all conversions to
modern C++ (`modernize-*`), such as converting to range-based for loops
or using the auto keyword. Other settings *should* not require
alteration for clang-tidy to work.

To run clang-tidy, right click on a target and highlight
`Clang Power Tools`. You will have a number of options. `Tidy` will
highlight code which fails one of the checks, whereas `Tidy fix` will
automatically change your code.

*Note: clang-tidy does not work on* **ALL BUILD** *so it is necessary to
select a subtarget.*

### Cmake

**Not advised:** Mantid provides clang-tidy functionality through cmake.
The guide [How To Setup Clang Tooling For
LLVM](https://clang.llvm.org/docs/HowToSetupToolingForLLVM.html) gives
valuable information for configuration.

In the cmake gui, find the `CLANG_TIDY_EXE` parameter. If you are a
non-Linux developer, you may have to manually point to your clang-tidy
install. Configure, and check the cmake log for the message
<span class="title-ref">clang-tidy found</span>. If the
<span class="title-ref">clang-tidy not found</span> warning was posted
instead then it has not worked.

Once you have clang-tidy, there are several relevant parameters you will
want to change:

- `ENABLE_CLANG_TIDY` will turn on clang-tidy support. This will run
  clang-tidy as part of the compile step and greatly slow-down
  compilation. It can be disabled again with this variable.
- `CLANG_TIDY_CHECKS` is a semi-colon separated list of checks for
  clang-tidy to carry out. This defaults to all `modernize-` checks.
- `APPLY_CLANG_TIDY_FIX` will automatically change the code whenever a
  check has returned a result. The behaviour of `ENABLE_CLANG_TIDY`
  without this checked is to highlight issues only.

Configure the build to check that your selected options are reflected in
`CMAKE_CXX_CLANG_TIDY`, and then generate. When you next build,
clang-tidy will perform the selected checks on the code included in the
target.

*Note: There is a known issue that clang-tidy is only being applied to
certain directories within* `Framework` *and* `Mantidplot`.

### Command line

The `run-clang-tidy` wrapper will run multiple instances of `clang-tidy`
in parallel.

``` sh
run-clang-tidy -config-file=/full/path/to/src/code/.clang-tidy  Framework/Nexus/src/Nexus
```

will run `clang-tidy` in parallel on all files that contain the path
`Framework/Nexus/src/Nexus`. One can also supply a single file (e.g.
`Framework/Nexus/src/NexusFile.cpp`) or a single target (e.g.
`NexusTest`) which will analyze all files that contribute to that
target. A notable limitation of the wrapper is that it needs a full path
to the configuration file, but not to the source files.

To run a specific check and apply it

``` sh
run-clang-tidy -config "{Checks: '-*,modernize-use-nullptr'}" -p $(pwd) Framework/Nexus/src/NexusFile.cpp -fix
```

will automatically change the file in the source tree.

### Options

The full list of [current clang-tidy
checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html).

Some clang-tidy checks have optional arguments. For example,
`modernize-loop-convert`, which changes loops to range-based, assigns a
riskiness to each loop and can accept a `MinConfidence` argument to
determine which risk levels to address.

Adding the optional arguments is clunky and will rarely be required, so
it has not been directly added to Mantid's cmake setup. To add optional
arguments, add the following onto the end of `CLANG_TIDY_CHECKS`:

    -config={CheckOptions: [ {key: check-to-choose-option.option-name, value: option-value} ]}

For example, to convert all loops classified as *risky* or above, we
would append:

    -config={CheckOptions: [ {key: modernize-loop-convert.MinConfidence, value: risky} ]}

clang-tidy does not have a concept of c++ standards it should use and
leaves it to the developer to know which to turn on and off.

## Cmake chart of target dependencies

Cmake has the ability to make graphviz dot files of the dependencies for
targets in mantid. This is useful for understanding why compilation will
wait for individual libraries to link before compiling more files.

``` sh
cmake --graphviz=dependencies.dot .
dot -O -Tsvg dependencies.dot.WorkflowAlgorithms
```

[Cmake
reference](https://cmake.org/cmake/help/latest/module/CMakeGraphVizOptions.html)
for configuring the tool options.
