# Generating code coverage

```{contents}
:local:
```

## C++

### Background

GCC and LLVM have tooling built in to allow a developer to view the code
coverage from their tests.

This will work for native C++ code and any code executed via Python, as
the instrumentation is compiled into the binary output. Gcovr will also
filter out any non-project code automatically.

### Coverage on conda

Since the move to conda, <span class="title-ref">GCC</span> is installed
through the conda package <span class="title-ref">gxx_linux-64</span>.
This conda package does NOT include <span class="title-ref">gcov</span>,
required to assess code coverage. <span class="title-ref">gcov</span> is
not currently available on conda, so you will need to ensure that
<span class="title-ref">gcov</span> can be instead found on your system,
and that this version of <span class="title-ref">gcov</span> is the same
version as the <span class="title-ref">GCC</span> compiler we install
using conda. If this is not the case, errors will be raised during the
generation of coverage data.

As <span class="title-ref">gcov</span> is packaged with
<span class="title-ref">GCC</span>, you will need to install
<span class="title-ref">GCC</span> on your system. You can typically do
this using the <span class="title-ref">apt</span> package manager. If an
appropriate version of <span class="title-ref">GCC</span> is not
available for your OS you will unfortunately have to build it from
source: <https://gcc.gnu.org/wiki/InstallingGCC>

### C++ specific notes

The coverage target(s) do not run the tests automatically. Users must
either run the test(s) or file(s) they are interested in manually for
data to be produced by the tooling.

### Setup

- Enable coverage in CMake through the GUI or using

  ``` shell
  cmake /path/to/src -DCOVERAGE=ON
  ninja # or make
  ```

- Install gcovr (already included in mantid-developer environments)

  ``` shell
  pip3 install gcovr
  ```

- Run C++/Python test(s) using ctest or the executable, or similar.

### Producing a coverage report automatically

The following targets are available to make/ninja:

- <span class="title-ref">coverage</span> : Builds all available
  coverage reports
- <span class="title-ref">coverage_clean</span> : Removes all
  instrumented data and report data
- <span class="title-ref">coverage_cpp</span> : Builds coverage
  information on cpp files

HTML output is written into:
<span class="title-ref">\<build_dir\>/coverage/\<lang\>/html</span>

Where <span class="title-ref">\<lang\></span> is either
<span class="title-ref">cpp</span> or
<span class="title-ref">python</span>.

Note: If errors are thrown on certain directories during automatic
report production, you may have more luck specifying individual
directories manually.

### Producing a coverage report manually

- Create a folder for the html files, for example
  <span class="title-ref">html</span>

- From your build folder generate a HTML report for the coverage as
  follows:

  ``` shell
  gcovr -r <src_dir> -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .
  ```

  Where:

  - -r Points to the full path of your source directory. *Note: A
    trailing slash must be present, i.e. /my/src/ instead of /my/src*
  - -j is the number of threads to run whilst generating, if you're
    unsure pick a value like 8.
  - -o controls the html output directory and file name
  - --exclude-throw-branches Gcovr will incorrectly show compiler
    inserted branches as unused, despite them not appearing in the
    source code. This filters them out making the branch hit rate sane.
  - (Optional) --gcov-executable if using a manually specified compiler
    use this to specify the appropriate gcov tool, e.g.
    <span class="title-ref">gcov-10</span> for
    <span class="title-ref">gcc-10</span> or
    <span class="title-ref">llvm-cov-11</span> for
    <span class="title-ref">llvm-11</span>

### Filtering specific module(s)

The <span class="title-ref">-f</span> flag can be appended once or
multiple times to show output from a single folder or subset of folders.
This works similarly to the regex filter used in unit testing.

#### Examples:

To only include, say, the scientific interfaces, it would be the
relative path of the source files:

``` shell
gcovr -f qt/scientific_interfaces -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .
```

To include only API and Kernel sources:

``` shell
gcovr -f Framework/Kernel -f Framework/API -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .
```

Additionally, folders can be excluded using the -e flag, this is useful
to filter out test coverage. Tests will typically have 100% coverage so
tend to add noise.

``` shell
gcovr -r /path/to/src -e Framework/KernelTest -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .
```

## Python

Unit tests can also be run to generate coverage too, this requires us to
run the test using the Coverage module.

### IDE

(Recommended)

Your IDE may already have an option to run the test with coverage
enabled if it can already run the test directly.

To setup unit tests for Pycharm see the `pycharm-ref` page.

### CLI

(Advanced)

- Install Coverage:

  > pip3 install coverage

- Make a note of the directory or test file you'd like coverage
  information from. Pytest will search recursively from a given
  directory.

- cd to build directory and run your test with coverage as follows

  > \# Where \<path\> is the directory or file to run
  > PYTHONPATH=/path/to/build/bin python3 -m coverage run -m pytest
  > \<path\>

- To limit coverage information to only project files add the
  <span class="title-ref">--source</span> flag:

  > \# Where /path/to/src/ contains Mantid source code \# Note this
  > cannot use the home dir, i.e. ~/path/to/src
  > PYTHONPATH=/path/to/build/bin python3 -m coverage run
  > --source=\</path/to/src/\> -m pytest \<path\>

- Example: To run coverage for the entire project

  > PYTHONPATH=/path/to/build/bin python3 -m coverage run --source
  > /mantid_src -m pytest /mantid_src

- Generate out a html report in the directory you ran tests from:

  > > \# This will generate htmlcov/index.html coverage html
