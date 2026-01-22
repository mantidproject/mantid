# Obtaining a Benchmark for Mantid Fitting

::: {.contents local=""}
:::

There are several scenarios in which obtaining a benchmark for the
accuracy and runtime of Mantid minimizers might be useful:

- From the perspective of a **Scientist**, who wants to know the best
  Mantid minimizer to use for fitting their model to their data.
- From the perspective of a **Software Developer**, who wants to
  optimize their code to improve its accuracy and runtime.

This page will explain how to benchmark the accuracy and runtime of
different fitting minimizers in Mantid for different fitting problems.
To do this, we will use a package called
[FitBenchmarking](https://fitbenchmarking.readthedocs.io/en/stable/)
which is a cross-platform open source tool for comparing different
minimizers and fitting frameworks.

Before you continue, make sure you have a recent version of python
installed, and access to git bash (which is the recommended terminal to
use).

## Benchmarking Mantid from Source

This section will show you how to benchmark Mantid using a source code
directory. This is useful for software developers who want to make quick
changes to the code to see how they affect the fit accuracy.

1.  Open a git bash terminal and cd to the desired location.
2.  Activate your mantid conda environment.
3.  Pip install the FitBenchmarking package by following the
    [FitBenchmarking installation
    instructions](https://fitbenchmarking.readthedocs.io/en/stable/users/install_instructions/fitbenchmarking.html).
4.  Follow the instructions in the
    `Running a Benchmark <running-a-benchmark-ref>` section below. Note
    that you *must* specify the `PYTHONPATH` in the last command, where
    the `<config>` is only required on Windows:

``` sh
PYTHONPATH=/path/to/build/bin/<config> fitbenchmarking -o fitting_options.ini -p examples/benchmark_problems/Muon
```

## Benchmarking Mantid from Install

This section will show you how to benchmark Mantid using an installed
version. This is useful when you want a more realistic benchmark for fit
runtimes.

### Initial Setup

1.  Download and install the desired version of MantidWorkbench from the
    [releases page](https://github.com/mantidproject/mantid/releases).
    Note its installation path, hereby denoted as `[INSTALL_PATH]`.
2.  Open a git bash terminal and cd to the desired location.
3.  Pip install the virtual environment package as described in the
    ['Installing virtualenv'
    section](https://packaging.python.org/en/latest/guides/installing-using-pip-and-virtual-environments/#installing-virtualenv).

### Creating your Benchmarking Environment

1.  From the same git bash terminal, run the following command:

If using Windows:

``` sh
py -m virtualenv -p=[INSTALL_PATH]\bin\python.exe benchmark-env
```

This will create a virtual environment based on the python executable
provided with your Mantid installation. For Windows, the location of
this executable is `C:\MantidInstall\bin\python.exe`.

If using Ubuntu:

``` sh
virtualenv --python=python3 benchmark-env
```

In this example, `benchmark-env` is the name given to your virtual
environment.

2.  Activate your environment. This is explained in the ['Activating a
    virtual environment'
    section](https://packaging.python.org/en/latest/guides/installing-using-pip-and-virtual-environments/#activating-a-virtual-environment).
3.  It might be necessary to upgrade and install a few packages before
    installing FitBenchmarking if working from a Linux system:

``` sh
python3 -m pip install --upgrade pip
python3 -m pip install --upgrade pillow
sudo apt-get install libglu1-mesa
```

4.  Install the FitBenchmarking package by following the
    [FitBenchmarking installation
    instructions](https://fitbenchmarking.readthedocs.io/en/stable/users/install_instructions/fitbenchmarking.html).
    Note that installing FitBenchmarking from source using the editable
    flag `-e` proved to be the most stable installation prior to the
    release of FitBenchmarking v0.2.

Your environment should now be ready for performing a benchmark of
Mantid minimizers.

## Running a Benchmark

The process for how to run a benchmark is explained extensively in the
[FitBenchmarking
documentation](https://fitbenchmarking.readthedocs.io/en/stable/users/index.html),
and so I recommend you give it a read. This section will give a basic
example of how to perform a simple benchmark of three Mantid minimizers.

1.  Create a file called `fitting_options.ini` with the following
    contents

``` text
[FITTING]

software: mantid

num_runs: 1

[MINIMIZERS]

mantid: Levenberg-Marquardt
        Levenberg-MarquardtMD
        Simplex

[PLOTTING]

make_plots: yes

[LOGGING]

external_output: log_only
```

2.  Download the examples folder from the [FitBenchmarking github
    repo](https://github.com/fitbenchmarking/fitbenchmarking) by git
    cloning the repository. Alternatively, you can define your own
    fitting problems.
3.  From your activated virtual environment, run the following command.
    This will run the Muon fitting problems assuming you have the same
    directory structure as seen on the Fitbenchmarking repo.

``` sh
fitbenchmarking -o fitting_options.ini -p examples/benchmark_problems/Muon
```

If benchmarking from source, you must also specify the `PYTHONPATH`. The
`PYTHONPATH` needs to point to your build/bin/\<config\> folder where
the `<config>` is only required on Windows and should be replaced by the
config of your build e.g. build/bin/Release.

``` sh
PYTHONPATH=/path/to/build/bin/<config> fitbenchmarking -o fitting_options.ini -p examples/benchmark_problems/Muon
```

When the benchmark is complete, it should open a browser which contains
the results. You should read the FitBenchmarking documentation if you
need help with how to interpret these results. The results will also be
stored in your current folder location.

## Tips

- Make sure your git bash terminal is open in the correct location and
  has the virtual environment activated when running your benchmark.
- Each time your run the benchmark, the old results will be overwritten
  unless you change the directory you run the `fitbenchmarking` command
  from. In later versions of FitBenchmarking (\>v1.5) there will be an
  option to specify the results directory on the command line or via the
  `.ini` file.
- Be aware that an 'Unexpected Exception' can sometimes occur when
  running the fitbenchmarking command after installing it from source
  without the editable flag `-e`.
