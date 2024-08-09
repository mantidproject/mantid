.. _RunningTheUnitTests:

======================
Running the Unit Tests
======================

.. contents::
  :local:

Overview
########

We use `CTest <https://cmake.org/cmake/help/latest/manual/ctest.1.html>`__
for building and running our unit tests. This wraps the underlying
`cxxtest <cxxtest>`__ or other (e.g. pyunit) test code.

CMake/CTest: Command Line
#########################

The unit tests are currently excluded from the 'all' target. To build
all of the tests, including the dependent framework code run:

.. code-block:: sh

   cmake --build . --target AllTests

To build only one package of tests (and its dependencies):

.. code-block:: sh

   cmake --build . --target KernelTest

To run all the tests:

.. code-block:: sh

   ctest -j8

To build and run all the tests in one shot:

.. code-block:: sh

   cmake --build . --target check

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

Running Unit Tests Directly
###########################

Starting in your build folder:

-  Running an entire test suite (e.g. `KernelTest`):

   .. code-block:: sh

      ./bin/KernelTest

-  Running a specific test class.

   .. code-block:: sh

      ./bin/KernelTest MyTestClassName

-  Running a specific test from a CxxTest test class (not possible via CTest).

   .. code-block:: sh

      ./bin/KernelTest MyTestClassName MySingleTestName

- Running a specific test from a Python ``unittest`` test class (not possible
  via CTest).

  .. code-block:: sh

     PYTHONPATH=/path/to/build/bin python3 /path/to/src/Framework/PythonInterface/test/python/plugins/algorithms/MeanTest.py MeanTest.test_mean

Running Unit Tests With Visual Studio and ctest
###############################################

Open the Mantid solution in Visual Studio. To run a subset of tests (for example ``UnitTests/AlgorithmsTest``);

-  In the Solution Explorer, right click the project for the tests (in this case ``UnitTests/AlgorithmsTest``) and select Properties.

-  In the Debugging tab of Properties change the Command Arguments box to the name of the test, for example "AddPeakTest".

-  Right click the directory again and select Debug->Start new instance.

-  Once the build has finished, open a file browser and navigate to the mantid build directory, run the command-prompt.bat file to open a command prompt and run

   .. code-block:: sh

     ctest -C Debug -V -R AddPeakTest

   For this example, there should be several lines of output ending with the time taken and the line

   .. code-block:: sh

     100% tests passed, 0 tests failed out of 1

   Omitting the ``-R AddPeakTest`` option runs all the tests, but note that any tests which were not built according to the above instructions will fail. Adding the ``-V`` increases the output verbosity.


Running Unit Tests With Visual Studio
#####################################

The unit tests can be run from within Visual Studio, following steps 1-3 above, with the addition in step 2 of;

-  Add the name of the test to the Target Name field in the General tab of Properties. Then add a breakpoint somewhere in the test header file.


Debugging unit tests
####################

See the instructions `here <DebuggingUnitTests>`__
