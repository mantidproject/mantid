========================
Generating Code Coverage
========================

.. contents::
    :local:


C++
====

Background
###########

GCC and LLVM have tooling built in to allow a developer to view the code coverage from their tests.

This will work for native C++ code and any code executed via Python, as the instrumentation is compiled into the binary output. Gcovr will also filter out any non-project code automatically.

Coverage on Conda
#################

Since the move to `conda`, `GCC` is installed through the conda package `gxx_linux-64`. This conda package does NOT include `gcov`, required to assess code coverage. `gcov` is not currently available on `conda`, so you will need to ensure that `gcov` can be instead found on your system, and that this version of `gcov` is the same version as the `GCC` compiler we install using `conda`. If this is not the case, errors will be raised during the generation of coverage data.

As `gcov` is packaged with `GCC`, you will need to install `GCC` on your system. You can typically do this using the `apt` package manager. If an appropriate version of `GCC` is not available for your OS you will unfortunately have to build it from source: https://gcc.gnu.org/wiki/InstallingGCC

C++ Specific Notes
##################

The coverage target(s) do not run the tests automatically. Users must either run the test(s) or file(s) they are interested in manually for data to be produced by the tooling.

Setup
#####

- Enable coverage in CMake through the GUI or using

  .. code-block:: shell

    cmake /path/to/src -DCOVERAGE=ON
    ninja # or make

- Install gcovr (already included in mantid-developer environments)

  .. code-block:: shell

      pip3 install gcovr

- Run C++/Python test(s) using ctest or the executable, or similar.

Producing a coverage report automatically
#########################################

The following targets are available to make/ninja:

- `coverage` : Builds all available coverage reports
- `coverage_clean` : Removes all instrumented data and report data
- `coverage_cpp` : Builds coverage information on cpp files

HTML output is written into: `<build_dir>/coverage/<lang>/html`

Where `<lang>` is either `cpp` or `python`.

Note: If errors are thrown on certain directories during automatic report production, you may have more luck specifying individual directories manually.

Producing a coverage report manually
####################################

- Create a folder for the html files, for example `html`
- From your build folder generate a HTML report for the coverage as follows:

  .. code-block:: shell

    gcovr -r <src_dir> -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .

  Where:

  - -r Points to the full path of your source directory. *Note: A trailing slash must be present, i.e. /my/src/ instead of /my/src*
  - -j is the number of threads to run whilst generating, if you're unsure pick a value like 8.
  - -o controls the html output directory and file name
  - --exclude-throw-branches Gcovr will incorrectly show compiler inserted branches as unused, despite them not appearing in the source code. This filters them out making the branch hit rate sane.
  - (Optional) --gcov-executable if using a manually specified compiler use this to specify the appropriate gcov tool, e.g. `gcov-10` for `gcc-10` or `llvm-cov-11` for `llvm-11`

Filtering specific module(s)
############################

The `-f` flag can be appended once or multiple times to show output from a single folder or subset of folders. This works similarly to the regex filter used in unit testing.

Examples:
*********

To only include, say, the scientific interfaces, it would be the relative path of the source files:

.. code-block:: shell

    gcovr -f qt/scientific_interfaces -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .

To include only API and Kernel sources:

.. code-block:: shell

    gcovr -f Framework/Kernel -f Framework/API -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .

Additionally, folders can be excluded using the -e flag, this is useful to filter out test coverage. Tests will typically have 100% coverage so tend to add noise.

.. code-block:: shell

    gcovr -r /path/to/src -e Framework/KernelTest -j <n_threads> --exclude-throw-branches --html --html-details -o html/<output>.html .


Python
======

Unit tests can also be run to generate coverage too, this requires us to run the test using the Coverage module.

IDE
###
(Recommended)

Your IDE may already have an option to run the test with coverage enabled if it can already run the test directly.

To setup unit tests for Pycharm see the :ref:`pycharm-ref` page.

CLI
###
(Advanced)

- Install Coverage:

  .. code-block :: shell

    pip3 install coverage

- Make a note of the directory or test file you'd like coverage information from. Pytest will search recursively from a given directory.

- cd to build directory and run your test with coverage as follows

  .. code-block :: shell

    # Where <path> is the directory or file to run
    PYTHONPATH=/path/to/build/bin python3 -m coverage run -m pytest <path>

- To limit coverage information to only project files add the `--source` flag:

  .. code-block :: shell

    # Where /path/to/src/ contains Mantid source code
    # Note this cannot use the home dir, i.e. ~/path/to/src
    PYTHONPATH=/path/to/build/bin python3 -m coverage run --source=</path/to/src/> -m pytest <path>

- Example: To run coverage for the entire project

  .. code-block :: shell

    PYTHONPATH=/path/to/build/bin python3 -m coverage run --source /mantid_src -m pytest /mantid_src

- Generate out a html report in the directory you ran tests from:

    .. code-block :: shell

      # This will generate htmlcov/index.html
      coverage html
