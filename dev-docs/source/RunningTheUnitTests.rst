.. _RunningTheUnitTests:

======================
Running the Unit Tests
======================

.. contents::
  :local:

Overview
########

We use `CTest <http://www.cmake.org/cmake/help/ctest-2-8-docs.html>`__
for building and running our unit tests. This wraps the underlying
`cxxtest <cxxtest>`__ or other (e.g. pyunit) test code.

CMake/CTest: Command Line using a makefile generator
####################################################

The unit tests are currently excluded from the 'all' target. To build
the all the tests, including the dependent framework code (in all these
examples in parallel using 8 cores):

.. code-block:: sh

   make -j8 AllTests

To build only one package of tests (and its dependencies):

.. code-block:: sh

   make -j8 KernelTest

To run all the tests:

.. code-block:: sh

   ctest -j8

To build and run all the tests in one shot:

.. code-block:: sh

   make -j8 check

To run a specific test or set of tests (will run all those that match
the search string):

.. code-block:: sh

   ctest -R KernelTest_TimerTest

So to run all tests in a suite (using a search string):

.. code-block:: sh

   ctest -j8 -R KernelTest

To exclude things from your tests (matches the string as with the -R
option) - useful for those building the performance tests:

.. code-block:: sh

   ctest -j8 -E Performance

Useful CTest Options
####################

``ctest -h`` gives the full list of command line arguments. Some useful
ones to note are:

-  ``--output-on-failure``: displays the log and any stderr output that
   is otherwise hidden by CTest.
-  ``--schedule-random``: run the tests in a random order. Useful to
   weed out accidental dependencies
-  ``--repeat-until-fail``\ : require each test to run times without
   failing in order to pass. Useful to try and find random failures

Running Unit Tests Manually
###########################

Starting in your build folder (e.g. Mantid/Code/debug):

-  Running an entire test suite (e.g. KernelTest):

   .. code-block:: sh

      ctest -j8 -R KernelTest
      bin/KernelTest

-  Running a specific test class.

   .. code-block:: sh

      ctest -R MyTestClassName
      bin/KernelTest MyTestClassName

-  Running a specific test.

   .. code-block:: sh

      bin/KernelTest MyTestClassName MySingleTestName``

   -  Not possible with ctest.

Visual Studio/ XCode note
#########################

In Visual Studio the user can alter the properties of the subset of
tests (inside the unitTest directory (e.g. AlgorithmTest). In the
properties box it is possible to specify a specific test to run by
typing its name in the TargetName box. Then to execute the test, right
click the subset of tests and select debug and then start new instance.

To run the tests under one of these environments then you will need to
open a command window and change to the build directory. Once there you
can run the tests by selecting the configuration;

.. code-block:: sh

   ctest -C Debug -j4

This runs all tests in Debug mode (note that this will NOT build any
outdated libraries). To select a subset use the ``-R`` option:

.. code-block:: sh

   ctest -C Release -R Kernel -j4

   (-R Kernel), with 4 cores (-j4), in Release mode (-C Release).

Debugging unit tests
####################

See the instructions `here <DebuggingUnitTests>`__
