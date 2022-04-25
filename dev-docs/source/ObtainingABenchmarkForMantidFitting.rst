.. _ObtainingABenchmarkForMantidFitting:

Obtaining a Benchmark for Mantid Fitting
========================================

.. contents::
  :local:

There are several scenarios in which obtaining a benchmark for the accuracy and runtime of Mantid minimizers might be useful:

- From the perspective of a **Scientist**, who wants to know the best Mantid minimizer to use for fitting their model to their data.
- From the perspective of a **Software Developer**, who wants to optimize their code to improve its accuracy and runtime.

This page will explain how to set up a python virtual environment that can be used to benchmark the accuracy and runtime of different fitting minimizers in Mantid for different fitting problems. To do this, we will use a package called `FitBenchmarking <https://fitbenchmarking.readthedocs.io/en/stable/>`_ which is a cross-platform open source tool for comparing different minimizers and fitting frameworks.

Initial Setup
#############

Some initial setup is required on your system before we can start setting up an environment for benchmarking Mantid minimizers. Before you start this process, make sure you have a recent version of python installed and access to a terminal or command prompt.

1. Download and install the desired version of Mantid from the `downloads <https://download.mantidproject.org/>`_ page (if not already installed). Note its installation path, hereby denoted as ``[INSTALL_PATH]``.
2. Open a terminal or command prompt and cd to the desired location.
3. Pip install the virtual environment package as described in the 'Installing virtualenv' section found `here <https://packaging.python.org/en/latest/guides/installing-using-pip-and-virtual-environments/#installing-virtualenv>`_.

Creating your Benchmarking Environment
######################################

1. From the same terminal or command prompt, run the following command:

.. code-block:: sh

  py -m virtualenv -p=[INSTALL_PATH]\bin\python.exe benchmark-env

This will create a virtual environment based on the python executable provided with your Mantid installation. For Windows, the location of this executable is ``C:\MantidInstall\bin\python.exe``. In this example, ``benchmark-env`` is the name given to your virtual environment.

2. Activate your environment. This is explained in the 'Activating a virtual environment' section found `here <https://packaging.python.org/en/latest/guides/installing-using-pip-and-virtual-environments/#activating-a-virtual-environment>`_.
3. Install the FitBenchmarking package by following the instructions found `here <https://fitbenchmarking.readthedocs.io/en/stable/users/install_instructions/fitbenchmarking.html>`_.

Your environment should now be ready for performing a benchmark of Mantid minimizers.

Running a Benchmark
###################

The process for how to run a benchmark is explained extensively in the `FitBenchmarking documentation <https://fitbenchmarking.readthedocs.io/en/stable/users/index.html>`_, and so I recommend you give it a read. This section will give a basic example of how to perform a simple benchmark of three Mantid minimizers.

1. Create a file called ``fitting_options.ini`` with the following contents

.. code-block:: text

  [FITTING]

  software: mantid

  num_runs: 1
  max_runtime: 600

  [MINIMIZERS]

  mantid: Levenberg-Marquardt
          Levenberg-MarquardtMD
          Simplex

  [PLOTTING]

  make_plots: yes

  [OUTPUT]

  results_dir: benchmark_results/

  [LOGGING]

  external_output: log_only


2. Download the examples folder from the `FitBenchmarking github repo <https://github.com/fitbenchmarking/fitbenchmarking>`_. Alternatively, you can define your own fitting problems.
3. From your activated virtual environment, run the following command. This will run the Muon fitting problems assuming you have the same directory structure as seen on the Fitbenchmarking repo.

.. code-block:: sh

  fitbenchmarking -o fitting_options.ini -p examples/benchmark_problems/Muon

When the benchmark is complete, it should open a browser which contains the results. You should read the FitBenchmarking documentation if you need help with how to interpret these results.

Tips
####

* Make sure your terminal or command prompt is open in the correct location and has the virtual environment activated when running your benchmark.
* It might be necessary to pip install some addition packages if you see errors when running a benchmark: ``pip install 'h5py>=2.10.0,<3' && pip install 'pyyaml>=5.4.1'``.
* Each time your run the benchmark, the old results will be overwritten unless you change the ``results_dir`` option in the ``*ini`` file.
* To do a benchmark of the changes made in a Pull Request, you can tell Jenkins to build an unstable package, and then you can install this locally. When creating your benchmark environment, you would then use the python.exe found in the Mantid unstable install directory.
