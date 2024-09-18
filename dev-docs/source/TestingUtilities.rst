.. _TestingUtilities:

=================
Testing Utilities
=================

.. contents::
  :local:

Summary
#######

This page will provide developers with details of testing utilities, such as helper files, which are
useful in creating unit tests.


Helper Functions
################

C++
---

The following helper files have been found in the
`Mantid/Framework/TestHelpers <http://github.com/mantidproject/mantid/tree/main/Framework/TestHelpers>`__
package:

-  `BinaryOperationMDTestHelper <http://doxygen.mantidproject.org/d1/d4f/namespaceBinaryOperationMDTestHelper.html>`__
-  ComponentCreationHelper
   `ComponentCreationHelper <http://doxygen.mantidproject.org/d8/d8d/namespaceComponentCreationHelper.html>`__
   This creates instrument components that can then be used in a unit test.
-  ICatTestHelper
-  `MDEventsTestHelper <http://doxygen.mantidproject.org/d5/d75/namespaceMantid_1_1MDEvents_1_1MDEventsTestHelper.html>`__
-  `SANSInstrumentCreationHelper <http://doxygen.mantidproject.org/d9/dbf/classSANSInstrumentCreationHelper.html>`__
-  `ScopedFileHelper <http://doxygen.mantidproject.org/d7/d7f/classScopedFileHelper_1_1ScopedFile.html#details>`__
   This creates a file that is automatically deleted when no longer needed.
-  `WorkspaceCreationHelper <http://doxygen.mantidproject.org/d1/db6/namespaceWorkspaceCreationHelper.html>`__
   This creates simple workspaces that can be used in a unit test. One of these workspaces has a full instrument.

Python
------

There are some ``testhelpers`` which are only available in Python, they can
be found in the ``testhelpers``-package.

-  ``make_decorator`` - A function that returns a decorator for an
   algorithm without executing it.
-  ``TemporaryFileHelper`` - A class that creates named temporary files
   and deletes them automatically when the object is deleted. Basically
   a thin wrapper around `NamedTemporaryFile <https://docs.python.org/2/library/tempfile.html>`__
   from the tempfile package.
- ``mtd.unique_name`` and ``mtd.unique_hidden_name`` - functions that allow you to create a workspace name that will be
   unique to avoid colisions with other workspaces. This is useful for testing and temporary workspaces.