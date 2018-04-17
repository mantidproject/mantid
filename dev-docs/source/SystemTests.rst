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

The (python) code for the system tests can be found in the git
repository at
`mantidproject/mantid <http://github.com/mantidproject/mantid>`__, under
the ``Testing/SystemTests`` directory.

Like their 'stress' equivalents (`stress testing <Stress_Tests>`__),
system tests inherit from the stresstesting.MantidStressTest class. The
methods that need to be overridden are ``runTest(self)``, where the
python code that runs the test should be placed, and ``validate(self)``,
which should simply return a pair of strings: the name of the final
workspace that results from the ``runTest`` method and the name of a
nexus file that should be saved in the ReferenceResults sub-directory in
the repository. The test code itself is likely to be the output of a
*Save History* command, though it can be any python code. In the
unlikely case of files being used during a system test, implement the
method ``requiredFiles`` which should return a list of filenames without
paths. The file to validate against should be included as well. If any
of those files are missing the test will be marked as skipped.

The tests should be added to the ``Testing/SystemTests/tests/analysis``,
with the template result going in the ``reference`` sub-folder. It will
then be included in the suite of tests from the following night.

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

   def validate(self):

method from the system test is sufficient.

Skipping tests
--------------

Tests can be skipped based on arbitrary criteria by implementing the
``skipTests`` method and returning True if your criteria are met, and
False otherwise. Examples are the availability of a data file or of
certain python modules (e.g. for the XML validation tests).

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

   self.tolerance = 0.00000001

Disable Some Checks
-------------------

You may disable some checks performed by the ``CompareWorkspaces``
algorithm by appending them to the disableChecking list, which, by
default, is empty.

.. code-block:: python

   # A list of things not to check when validating
   self.disableChecking = []

Assertions
----------

Additional assertions can be used as the basis for your own comparison
tests. The following assertions are already implemented in the base
class.

.. code-block:: python

   def assertTrue(self, value, msg=""):
   def assertEqual(self, value, expected, msg=""):
   def assertDelta(self, value, expected, delta, msg=""):
   def assertLessThan(self, value, expected, msg=""):
   def assertGreaterThan(self, value, expected, msg=""):

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

Adding New Data & References Files
----------------------------------

The data is managed by CMake's external data system that is described by
`Data_Files_in_Mantid <Data_Files_in_Mantid>`__. Please see `Adding a
new file <Data_Files_in_Mantid#Adding_A_New_File>`__ for how to add new
files.

Best Practice
#############

-  Always check your test works locally before making it public.
-  User stories should come from the users themselves where possible.
-  Take care to set the tolerance to an acceptable level.
