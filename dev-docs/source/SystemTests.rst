.. _SystemTests:

============
System Tests
============

.. contents::
  :local:

Overview
########

System tests are high-level tests, which check that Mantid is able to
reproduce accepted, standardised results as part of its calculations,
when executing user stories. The system test suite is written against
Mantid's Python API.

As part of our nightly-build and nightly-test procedure, Mantid's system
tests are run as acceptance tests. The nightly-test jobs deploy a
packaged version of Mantid to the target OS, before executing the system
tests scripts on that environment.

Writing a Test
##############

The (Python) code for the system tests can be found in the git
repository at
`mantidproject/mantid <https://github.com/mantidproject/mantid>`__, under
the ``Testing/SystemTests`` directory.

System tests inherit from the :class:`systemtesting.MantidSystemTest` class.
The methods that need to be overridden are ``runTest(self)``, where the Python
code that runs the test should be placed, and ``validate(self)``, which should
simply return a pair of strings: the name of the final workspace that results
from the ``runTest`` method and the name of a nexus file that should be saved
in the ``ReferenceResults`` sub-directory in the repository. The test code
itself is likely to be the output of a *Save History* command, though it can
be any Python code. In the unlikely case of files being used during a system
test, implement the method ``requiredFiles`` which should return a list of
filenames without paths. The file to validate against should be included as
well. If any of those files are missing the test will be marked as skipped.

The tests should be added to the ``Testing/SystemTests/tests/framework``,
with the template result going in the ``reference`` sub-folder. It will
then be included in the suite of tests from the following night.

Alternatively, any tests relating to testing qt interfaces should be added to
the ``Testing/SystemTests/tests/qt`` directory.

Specifying Validation
---------------------

You may need to inform the System Test Suite about the format of that
the benchmark workspace you wish to validate against. By default, the
system tests assume that the second argument returned by the validate
tuple is the name of a nexus file to validate against. However you can
override the validateMethod on a test with any one of three options.

-  WorkspaceToNexus (Benchmark workspace is stored as a Nexus file)
   (default)
-  WorkspaceToWorkspace (Benchmark workspace is stored as a workspace)
-  ValidateAscii (Benchmark workspace is stored as an ascii file)

For example:

.. code-block:: python

    def validateMethod(self):
        return 'WorkspaceToNeXus'

No Workspace Validation
-----------------------

If the system test does not need comparison/validation against a
standard workpace, then this step can be skipped. Simply omitting the

.. code-block:: python

   def validate(self):
       pass

method from the system test is sufficient.

Skipping tests
--------------

Tests can be skipped based on arbitrary criteria by implementing the
``skipTests`` method and returning True if your criteria are met, and
False otherwise. Examples are the availability of a data file or of
certain Python modules (e.g. for the XML validation tests).

Target Platform Based on Free Memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some tests consume a large amount memory resources, and are therefore
best executed on hardware where enough memory is available. You can set
a minimum RAM specification by overriding requiredMemoryMB:

.. code-block:: python

   def requiredMemoryMB(self):
       return 2000

The above function limits the test to run on a machine where there is at
least 2GB of free memory.

Target Platform Based on Free Memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some tests require very large files that cannot be placed in the shared
repository. The ``requiredFiles()`` method returns a list of these files
so that they test can check that they are all available. If all files
are not available then the tests are skipped.

.. code-block:: python

   def requiredFiles(self):
       return ['a.nxs', 'b.nxs']

The above function limits the test to run on a machine that can find the
files 'a.nxs' & 'b.nxs'

Set the Tolerance
-----------------

You may specialise the tolerance used by ``CompareWorkspace`` in your
system test.

.. code-block:: python

   self.tolerance = 0.00000001

By default the tolerance is absolute. It can be changed to relative by another
flag in the :class:`systemtesting.MantidSystemTest` class.

.. code-block:: python

   self.tolerance_rel_err = True

Disable Some Checks
-------------------

You may disable some checks performed by the ``CompareWorkspaces``
algorithm by appending them to the disableChecking list, which, by
default, is empty.

.. code-block:: python

   # A list of things not to check when validating
   self.disableChecking = []

Assertions
----------

Additional assertions can be used as the basis for your own comparison
tests. The following assertions are already implemented in the base
class.

.. rstcheck: ignore-next-code-block
.. code-block:: python

   def assertTrue(self, value, msg=""):
   def assertEqual(self, value, expected, msg=""):
   def assertDelta(self, value, expected, delta, msg=""):
   def assertLessThan(self, value, expected, msg=""):
   def assertGreaterThan(self, value, expected, msg=""):

Running Tests Locally
#####################

CMake configures a script file called ``systemtest`` (``systemtest.bat``
on Windows) in the root of the build directory. This file is the driver
script to execute the system tests that runs the lower-level
``Testing/SystemTests/scripts/runSystemTests.py`` script but ensures
that the environment is set up correctly for that particular build and
that the required test data has been updated. The script accepts a
``-h`` option to print out the standard usage information.

Usage differs depending on whether you are using a single-configuration
generator with CMake, for example Makefiles/Ninja, or a
multi-configuration generator such as Visual Studio or Xcode.

Downloading the test data
-------------------------

The ``systemtest`` script will automatically attempt to download any missing
data files but will time-out after 2 minutes. The time out limit can be set in two
variables ``ExternalData_TIMEOUT_INACTIVITY`` and ``ExternalData_TIMEOUT_ABSOLUTE``.
If using CMake these will need to be added as new string entries (value is in seconds).

Visual Studio/Xcode
-------------------

The user must first open command-prompt from, the build directory. The
script requires the developer to select the configuration that will be
used to execute the tests, one of: *Release*, *Debug*, *RelWithDebInfo*
or 'MinSizeRelease''. Note that the script does not build the code so
the chosen configuration must have already been built. An example to
execute all of the tests for the release configuration would be (in the
command-prompt):

.. code-block:: sh

    > systemtest -C Release

Makefile-like Generators
------------------------

The script requires no additional arguments as the configuration is
fixed when running CMake, e.g.

.. code-block:: sh

   cd build
   systemtest

Selecting Tests to Run From IDE
-------------------------------

System tests can be run from the MSVC IDE using the ``SystemTests`` target,
which behaves in a similar way to unit test targets. One key advantage is
that it allows you to start Mantid in a debug environment rather than attach
to one midway through.

To select an individual test, or range of tests, go to the ``SystemTests``
properties, go to ```Command Arguments``` and append flags as appropriate.

For example, adding ``-R ISIS`` will run any tests which match the regular
expression ``ISIS``.

Debugging System Tests in Pycharm
---------------------------------

System tests can be debugged from Pycharm without finding and attaching to
the process, or using a remote debugger.

To do this, create a new Configuration (Run -> Edit Configurations), and add
a new Python configuration with the script path set to ``runSystemtests.py``
This is found in ``/Testing/SystemTests/Scripts/runSystemTests.py``

The parameters for the configuration can be set just like the command line
args when running the tests from the ``systemtest.bat``/``systemtest`` script, e.g pass
``-R="EnginX"`` to run all tests containing the string ``EnginX`` in their
name.

Note that running the system tests this way will not update the system test
data, so if your data need to be updated, the system tests should be called
via the normal method in the first case.

Do not use the multiprocessing ``-j`` flag in your configuration parameters
as this will render you unable to debug the system tests directly, as they
will no longer be running under the parent Python process.

N.B. Windows users do not need to specify the configuration with the ``-C``
flag as when using the ``systemtest.bat`` script, as this is not passed to
``runSystemtests.py`` and will result in an error.

Selecting Tests To Run
----------------------

The most important option on the script is the ``-R`` option. This
restricts the tests that will run to those that match the given regex,
e.g.

.. code-block:: sh

   cd build
   systemtest -R SNS
   # or for msvc/xcode
   systemtest -C <cfg> -R SNS

would run all of the tests whose name contains SNS.


Running the tests on multiple cores
-----------------------------------

Running the System Tests can be sped up by distributing the list of
tests across multiple cores. This is done in a similar way to ``ctest``
using the ``-j N`` option, where ``N`` is the number of cores you want
to use, e.g.

.. code-block:: sh

   ./systemtest -j 8

would run the tests on 8 cores.

Some tests write or delete in the same directories, using the same file
names, which causes issues when running in parallel. To resolve this,
a global list of test modules (= different Python files in the
``Testing/SystemTests/tests/framework`` directory) is first created.
Now we scan each test module line by line and list all the data files
that are used by that module. The possible ways files are being
specified are:
1. if the extensions ``.nxs``, ``.raw`` or ``.RAW`` are present
2. if there is a sequence of at least 4 digits inside a string
In case number 2, we have to search for strings starting with 4 digits,
i.e. "0123, or strings ending with 4 digits 0123".
This might over-count, meaning some sequences of 4 digits might not be
used for a file name specification, but it does not matter if it gets
identified as a filename as the probability of the same sequence being
present in another Python file is small, and it would therefore not lock
any other tests. A dict is created with an entry for each module name
that contains the list of files that this module requires.
An accompanying dict with an entry for each data file stores a lock
status for that particular datafile.

Finally, a scheduler spawns ``N`` threads who each start a loop and
gather a first test module from the master test list which is stored in
a shared dictionary, starting with the number in the module list equal
to the process id.

Each process then checks if all the data files required by the current
test module are available (i.e. have not been locked by another
thread). If all files are unlocked, the thread locks all these files
and proceeds with that test module. If not, it goes further down the
list until it finds a module whose files are all available.

Once it has completed the work in the current module, it unlocks the
data files and checks if the number of modules that remains to be
executed is greater than 0. If there is some work left to do, the
thread finds the next module that still has not been executed
(searches through the tests_lock array and finds the next element
that has a 0 value). This aims to have all threads end calculation
approximately at the same time.

Reducing the size of console output
-----------------------------------

The ``systemtests`` can be run in "quiet" mode using the ``-q`` or
``--quiet`` option. This will print only one line per test instead of
the full log.

.. code-block:: sh

   ./systemtest --quiet
   Updating testing data...
   [100%] Built target StandardTestData
   [100%] Built target SystemTestData
   Running tests...
   FrameworkManager-[Notice] Welcome to Mantid 3.13.20180820.2132
   FrameworkManager-[Notice] Please cite: http://dx.doi.org/10.1016/j.nima.2014.07.029 and this release: http://dx.doi.org/10.5286/Software/Mantid
   [  0%]   1/435 : DOSTest.DOSCastepTest ............................................... (success: 0.05s)
   [  0%]   2/435 : ISISIndirectBayesTest.JumpCETest .................................... (success: 0.06s)
   [  0%]   3/435 : ISISIndirectInelastic.IRISCalibration ............................... (success: 0.03s)
   [  0%]   4/435 : HFIRTransAPIv2.HFIRTrans1 ........................................... (success: 1.30s)
   [  1%]   5/435 : DOSTest.DOSIRActiveTest ............................................. (success: 0.04s)
   [  1%]   6/435 : ISISIndirectBayesTest.JumpFickTest .................................. (success: 0.06s)
   [  1%]   7/435 : AbinsTest.AbinsBinWidth ............................................. (success: 1.65s)
   [  1%]   8/435 : ISIS_PowderPearlTest.CreateCalTest .................................. (success: 1.65s)
   [  2%]   9/435 : ISISIndirectInelastic.IRISConvFit ................................... (success: 0.56s)
   [  2%]  10/435 : LiquidsReflectometryReductionWithBackgroundTest.BadDataTOFRangeTest . (success: 2.94s)
   [  2%]  11/435 : DOSTest.DOSPartialCrossSectionScaleTest ............................. (success: 0.23s)
   [  2%]  12/435 : ISISIndirectBayesTest.JumpHallRossTest .............................. (success: 0.07s)
   [  2%]  13/435 : ISISIndirectInelastic.IRISDiagnostics ............................... (success: 0.03s)
   [  3%]  14/435 : HFIRTransAPIv2.HFIRTrans2 ........................................... (success: 0.83s)
   [  3%]  15/435 : DOSTest.DOSPartialSummedContributionsCrossSectionScaleTest .......... (success: 0.15s)
   [  3%]  16/435 : ISISIndirectBayesTest.JumpTeixeiraTest .............................. (success: 0.07s)
   [  3%]  17/435 : ISISIndirectInelastic.IRISElwinAndMSDFit ............................ (success: 0.29s)
   [  4%]  18/435 : MagnetismReflectometryReductionTest.MRFilterCrossSectionsTest ....... (success: 5.30s)
   [  4%]  19/435 : DOSTest.DOSPartialSummedContributionsTest ........................... (success: 0.16s)

One can recover the full log when a test fails by using the ``--ouptut-on-failure`` option.

Running a cleanup run
---------------------

A cleanup run will go through all the tests and call the
``.cleanup()`` function for each test. It will not run the tests
(i.e. call the ``execute()`` function) themselves. This is achieved
by using the ``-c`` or ``--clean`` option, e.g.

.. code-block:: sh

   ./systemtest -c

This is useful if some old data is left over from a previous run,
where some tests were not cleanly exited.

Adding New Data & References Files
----------------------------------

The data is managed by CMake's external data system that is described by
:ref:`DataFilesForTesting`. Please see :ref:`DataFilesForTesting_AddingANewFile` for how to add new
files.

Best Practice
#############

-  Always check your test works locally before making it public.
-  User stories should come from the users themselves where possible.
-  Take care to set the tolerance to an acceptable level.
