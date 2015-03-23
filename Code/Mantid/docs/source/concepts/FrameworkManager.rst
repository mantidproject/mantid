.. _Framework Manager:

Framework Manager
=================

What is it?
-----------

The framework manager is the main class through which user code will
interact with the Mantid framework. It is the center piece of the
application programming interface.

The main purpose of the frameworkManager is to simplfy the interaction
with the various internal services of the Mantid framework. Of course
this does not prevent you from accessing those services directly should
you so wish.

What does it allow you to do?
-----------------------------

The frameworkManager allows you to create and execute algorithms as well
as retrieving workspaces, and deleteing them if they are no longer
required.

It is good practice to delete workspaces that are no longer required to
free up the memory used.

For the most up to date listing of the methods that Framework Manager
provides look at the `Doxygen code
documentation <http://doxygen.mantidproject.org/>`__. Select the classes
tab and look for FrameworkManagerImpl (Mantid::API).



.. categories:: Concepts