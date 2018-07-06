.. _UnitTestGoodPractice:

=======================
Unit Test Good Practice
=======================

.. contents::
  :local:

General Guidance
################

What to test
------------

Simply put you should test.

-  Every public member of a class.
-  That the class can be cast to any of the interfaces or base classes
   it inherits from.
-  Any private or protected members.

   -  That aren’t directly covered by a public method test.
   -  That do any significant processing.

For each method you are testing you should include tests for the
following:

-  To confirm that the methods meet the requirements associated with
   them. Thus the test should verify that the function does what it is
   supposed to do.
-  To confirm the expected behaviour for boundary and special values.
-  To confirm that exceptions are thrown when expected.

How to test private or protected members of a class
---------------------------------------------------

Testing the internals of a class can be considered harmful as it exposes
the internals of the class, which can arguably be freely changed (so
long as it does not affect the function of the public interface).
However there are cases where the internals of a class need unit tests
either due to complexity or tracking down specific bugs.

In the circumstance where the implementation within the private/protected
methods of a class is sufficiently complex, such to require dedicated unit
tests, this code should be moved into a separate class(es).

Protected
~~~~~~~~~

Within the test library you can add a new testable class that inherits
from the class you need to test. This class can simply expose any
protected methods as testable public methods.

Private
~~~~~~~

There is no ideal way to test a private member of a class as they are
intentionally hidden from the class interface. There are two options to
consider in preference order:

#. Change the protection level to protected and follow the approach
   above.
#. Declare the test class as a friend, which can access private members.

Good practices for writing tests
--------------------------------

The following are good practices for writing your unit tests. Many of
them are standard good coding practices. You will notice that in several
situations they can clash with each other, in this case common sense
needs to be applied.

-  Unit tests should test one method only. This allows you to easily
   identify what failed if the test fails.
-  Unit tests should not be coupled together, therefore one unit test
   **CANNOT** rely on another unit test having completed first.

These two often clash, in which case it is often better to compromise on
the first, and in fact we have relaxed this rule for Mantid (see below).

-  Units tests should use realistic data
-  Unit tests should use small and simple data sets.

Again these can often conflict.

-  Each test class should be named after the class it is testing (e.g.
   tests for the ``AlgorithmFactory`` should go in a ``AlgorithmFactoryTest``
   class).
-  Each test within a test class should use a descriptive test name,
   prefixed with test (tests for the ``CreateAlgorithm`` method would be
   included in ``testCreateAlgorithm``). If there are specific tests for
   failure situations then these should be added to the end (e.g.
   ``testCreateAlgorithmNoAlgorithmException``). THE AIM IS THAT FROM THE
   TEST METHOD NAME ALONE, YOU SHOULD BE ABLE TO IDENTIFY THE PROBLEM.

Other More General Points
~~~~~~~~~~~~~~~~~~~~~~~~~

-  Tests should be **fast**, ideally really fast - certainly not more
   than a few seconds. Unit tests test functionality, performance tests
   can be used to check stress and timings.
-  Untestable code is a code-smell, if you can't get the code under test
   it probably needs refactoring.
-  Weight your testing to be destructive rather than demonstrative.
   Destructive tests have a higher efficacy for finding bugs.

Mantid-specific Guidelines
##########################

-  As noted above, you can assume that individual tests within a cxxtest
   suite will be run in order.
-  There must be **no relative paths** (or, more obviously, absolute
   ones) used in tests as with CMake the code can be build anywhere with
   respect to the source tree. Make use of the datasearch.directories
   property (which CMake configures to hold correct paths for a given
   build).
-  Ideally, test suites should not have a constructor. If one is
   required, the following boiler-plate code **must** be inserted in the
   test class:

   .. code-block:: c++

      static NameOfTest *createSuite() { return new NameOfTest(); }
      static void destroySuite(NameOfTest *suite) { delete suite; }

   where ``NameOfTest`` is the name of the test class. Without this, the
   class is turned into a static meaning that the constructor is run at
   initialisation even if (via an argument) you are not going to run that
   particular test suite. Also, this can cause problems if running tests in
   parallel.

-  Be cautious in use of the ``setUp()``and ``tearDown()`` methods. Be aware
   that if you use these in your suite they will be run before/after
   **every single** individual test. That's fine if it's the behaviour
   you really need, but we have found that to be rare - use the
   constructor or set things up within the test.
-  To avoid clashes, use unique names for workspaces that will go into
   the [Analysis Data Service], perhaps by prepending the name of the
   test suite. Even better, don't put workspaces into the ADS in the
   first place: for example, an InputWorkspace property can be set via
   pointer instead of by name.
-  Clean up the ADS at (or before) the end of the test suite.

Using files in Unit tests
-------------------------

Files for unit tests bloat our repository and slow down the testing
process. Therefore unless the prime purpose of the algorithms is to load
or save a file then you should not use a file in your unit tests.

How do I get a workspace filled with data?
    Firstly you want to think about how much data you really need, unit
    tests need to be fast so you don't want too much data.
    Secondly you should use and extend helper classes (like
    `1 <https://github.com/mantidproject/mantid/blob/master/Framework/TestHelpers/inc/MantidTestHelpers/WorkspaceCreationHelper.h>`__)
    to provide the workspaces for you. Keep things as generic as you can
    and it will help you and others for other tests.
    More details of this will be provided at `Testing Utilities <TestingUtilities>`__.
I want a workspace with a valid instrument definition and Spectra-detector map
    As above use or extend a method in one of the `helper classes <TestingUtilities>`__
    that actually creates a minimal workspace for you in code - it will
    only hurt the first time but everyone will benefit.
    Loading instrument XML files in debug **really** hurts performance;
    avoid this like the plague.
What if it **really** needs a file
    First justify your reasoning with the PM or Lead developer
    Ensure the file is as small as possible. Perhaps edit the file to
    only contain 2 spectra
    Note: this is not the same as just loading 2 spectra from a large
    file.
    Do not use a relative path to a file
    Used the `Scoped
    File <https://github.com/mantidproject/mantid/blob/master/Framework/TestHelpers/inc/MantidTestHelpers/ScopedFileHelper.h>`__
    helper, to ensure that resources are cleaned-up in an exception safe
    manner.

Mocking
#######

Mocking is a very powerful tool that allows you to simulate components
in your unit environment and check how your code operates within this
environment. Mocking allows you to avoid creating Fake objects of any
kind, and results in fast executing code with a very high test coverage.
See `Mocking <Mocking>`__ in Mantid to find out what it is and how it
works.

.. figure:: images/Mocking.png
   :alt: Object under test using Mocking to isolate the testing.|400px

   Object under test using Mocking to isolate the testing.|400px
