.. _Plugin:

Plugin
======

What is it?
-----------

Mantid is designed to be extensible by anyone who can write a simple
library in C++. There are several areas that have been built so that new
version and instances can be added just by copying a library with the
new code into the plugin directory before starting mantid. This
eliminates the need to recompile the main Mantid code or any user
interfaces derived from it when you want to extend it by, for example,
adding a new algorithm.

What is a plugin?
-----------------

A plugin is a library of one or more classes that include the
functionality that you need. Within the outputs of the Mantid project
Several of the libraries we deliver are created as plugins. Examples
are:

-  MantidAlgorithms - Contains the general :ref:`algorithms <Algorithm>`
-  MantidDataHandling - Contains the basic data loading and saving
   :ref:`algorithms <Algorithm>`
-  MantidNexus - Contains the :ref:`algorithms <Algorithm>` for handling
   nexus files
-  MantidDataObjects - Contains the definitions of the standard
   :ref:`workspaces <Workspace>`

How can you extend Mantid?
--------------------------

The following areas have been designed to be easily extensible through
using plugins. Each one contains more details in case you wish to create
one of your own.

-  :ref:`Algorithm <Algorithm>`
-  :ref:`Workspace <Workspace>`
-  Unit

How do you create a plugin?
---------------------------

There is nothing special about the library you build in order for it to
be used as a plugin, as long as it contains one or more algorithms,
workspaces or units (they can be mixed) they will automatically be
registered and available for use.

How does it work?
-----------------

Each of the extensible units within Mantid shares a base class that all
further objects of that type inherit from. For example all algorithms
must inherit from the Algorithm base class. This allows all uses of
those objects to work through the interface of the base class, and the
user (or other code) does not need to know what the algorithm actually
is, just that it is an algorithm.

In addition each of the extensible units has a macro that adds some code
that automatically registers the class with the appropriate :ref:`dynamic
factory <Dynamic Factory>`. This code executes immediately when the
library is loaded and is what makes you new objects available for use.
All of these macros start DECLARE and, for example, the one for
algorithms is:

-  ``DECLARE_ALGORITHM(classname)`` (or ``namespace::classname`` if the
   declaration is not enclosed in the algorithm's namespace)



.. categories:: Concepts
