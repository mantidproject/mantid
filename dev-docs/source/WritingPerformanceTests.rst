.. _WritingPerformanceTests:

=========================
Writing Performance Tests
=========================

.. contents::
  :local:

Overview
########

The point of Performance Tests is to track the performance of a piece of
code through time, so that it will be easy to pinpoint any change that
reduced the performance of a particular bit of code. Performance Tests
will be built and run by a build job, that will track the history of
timing vs revision. The Jenkins job that runs the performance tests
sends out an email when the performance of a test is lowered by more
than a threshold percentage (currently 25%), relative to the average of
a few previous tests.

How to Write C++ Performance Tests
##################################

An ideal performance test is neither too fast, nor too slow. The
precision of timing will be insufficient if the test runs in much less
than a second; tests that run for several minutes should also be
avoided.

C++ performance tests are written in the same way as unit tests, and in
the same file as the unit tests for a particular class, except that you
will add *Performance* to the end of the name of the test suite. For
example, in MyAlgorithmTest.h:

.. code-block:: c++

   class MyAlgorithmTest : public CxxTest::TestSuite {
      // Put in your usual, quick unit tests here
   };

   class MyAlgorithmTestPerformance : public CxxTest::TestSuite {
   public:
      MatrixWorkspace_sptr WS;
      int numpixels;

      void setUp() {
         // Put in any code needed to set up your test,
         // but that should NOT be counted in the execution time.
      }

      void tearDown() {
         // Clean-up code, also NOT counted in execution time.
      }

      void test_slow_performance() {
         // Put in a unit test that will be slow.
      }
   };

Only the presence/absence of the word Performance is used to determine
if it is a Performance test. As with unit tests, your suite name has to
match the file name.

You may include ASSERT's in the same way as a unit test to check that
results are meaningful, but that is unnecessary since these tests are
for performance, not function. Avoid adding so many assert's that it
would slow down your test.

Running Performance Tests
#########################

Performance tests targets are not added by default when running
cmake/make. To add them as ctest targets, enable them with the flag:

.. code-block:: sh

   cmake -DCXXTEST_ADD_PERFORMANCE=TRUE

After re-building, you can then run performance tests with the command:

.. code-block:: sh

   ctest [-C Release|Debug] -R Performance

And run regular unit tests, excluding the slow performance ones, with:

.. code-block:: sh

   ctest [-C Release|Debug] -E Performance

where the -C option is required for multi-configuration builds like
Visual Studio & XCode.

The resulting .XML files will contain the detailed timing (of just the
test portion without setUp/tearDown time).

Note that newly added performance tests will not be registered with
ctest until cmake has been re-run.

Running tests without ctest
---------------------------

The tests are still built into every test executable, even if you have
not set the flag. For example,

.. code-block:: sh

   AlgorithmsTest --help-tests

will list all the available tests. If you run

.. code-block:: sh

   AlgorithmsTest

alone, it will SKIP the Performance Tests. You have to give the name of
the specific test suite you want to run, e.g,

.. code-block:: sh

   AlgorithmsTest MyAlgorithmPerformanceTest

Best Practice Advice
####################

-  Performance tests are not System Tests. They should test the code at
   the same granularity as the unit test suite.
-  Performance tests are not Unit Tests. There is no need to perform
   lots of assertions on the test results.
-  Performance tests should perform enough work such that statistically
   significant performance differences can be measured.
-  The performance tests are executed often, so ideally they should
   typically take 0.2 - 2 seconds to run.
-  Always perform test set-up outside of the test method. That way your
   timings will only relate to the target code you wish to measure.

Avoiding Compiler Optimizations
###############################

A common issue with performance tests is when you want to compute some
value over a large number of iterations but ultimately you don't want
to actually use it in the end. We have included some handy utility functions
to avoid the compiler optimizing unused variables away. For more information
this conference talk is quite enlightening: https://youtu.be/nXaxk27zwlk?t=2441.

For example, we attempt to time the sum of a value after many calls to an operation:

.. code-block:: c++

   #include <CxxTest/BenchmarkUtil.h>

   class MyTestPerformance : public CxxTest::TestSuite {
   public:

      void test_slow_performance() {
        double value(0.0);
        for(size_t i = 0; i < 1000000) {
          value += my_op();
          CxxTest::doNotOptimize(&value);
        }
      }
   };

Without the ``CxxTest::doNotOptimize(&value);`` call the compiler will likely spot
that ``value`` is ultimately unused and the whole function will be optimized away
and the benchmark will become useless.

Jobs that monitor performance
#############################

There are currently no jobs that run the performance tests. See `here
<https://github.com/mantidproject/mantid/blob/main/buildconfig/Jenkins/performancetests>`__
for the build script that was used to run the tests on Jenkins.
Anyone attempting to resurrect the job will have to modify it
to work with the new Conda build system. There is a `set of python scripts
<https://github.com/mantidproject/mantid/tree/main/Testing/PerformanceTests>`__
for controlling and monitoring the performance tests.

The timing output of these jobs were typically monitored manually on a
weekly basis to pick up any notable performance degradation. Although
automatic checking is available within the python scripts, the level of
instability in the timings meant that it always produced way too many
false positives to be useful.
