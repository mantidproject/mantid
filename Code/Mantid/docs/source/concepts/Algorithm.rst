.. _Algorithm:

Algorithm
=========

What are they?
--------------

Algorithms are the verbs of Mantid. They are the actors. If you want to
manipulate your data in any way it will be done through an algorithm.
Algorithms operate primarily on data in :ref:`workspaces <Workspace>`. They
will normally take one or more :ref:`workspaces <Workspace>` as an input,
perform some processing on them and provide an output as another
:ref:`workspace <Workspace>` (although it is possible to have multiple
outputs).

Categories, Name and Versions
-----------------------------

Each algorithm has a category, a name and a version. The name and
version of an algorithm when taken together have to be unique.

Category
~~~~~~~~

A category is a group of algorithms that have some connection in their
usage. This is primarily used to make the list of algorithms easier to
work with in graphical user interfaces. Example categories include,
DataHandling, Diffraction, Muon, Workflow and are currently
subcategories of Algorithms category.

Name
~~~~

The name of an algorithm is what is used to refer to it. This can be
different from the class name in C++, as for example if you had two
versions of the same algorithm they would have the same name, but would
have to have different class names (or at least be in different
namespaces).

Version
~~~~~~~

Mantid allows multiple versions of the same algorithm. These are
differentiated by using a single integer as a version number, where a
higher version number denotes a more recent version. This allows you to
normally use the most recent version of an algorithm but also to access
previous versions if you prefer.

Parameters
----------

Each algorithm will have one or more parameters, known within Mantid as
:ref:`properties <properties>`, that will control how it performs its
processing. These parameters specify both what the inputs and outputs of
the algorithm will be as well any other options that alter the
processing.

For examples of the parameters of an algorithm, look at the page for one
of the example algorithms below.

Usage
-----

.. code-block:: python

     # where p1,p2 & p3 are values for algorithm "Alg"'s properties
     mtd.execute("Alg","p1;p2;p3") # using parameter ordinal position
     #or
     mtd.execute("Alg","Property1=p1;Property2=p2;Property3=p3") #using parameter names
     #or
     alg = mtd.createAlgorithm("Alg") # explicitly setting each parameter, then executing
     alg.setPropertyValue("Property1","p1")
     alg.setPropertyValue("Property2","p2")
     alg.setPropertyValue("Property3","p3")
     alg.execute()

     # Properties of Algorithms can be read (but not written to) through a Python dictionary. So you may do:
     print alg["Property1"]
     # prints 'p1'
     print alg["Property2"]
     # prints 'p2', etc


Example Algorithms
------------------

-  :ref:`Plus <algm-Plus>`
   - An algorithm for adding data in two :ref:`workspaces <Workspace>`
   together
-  :ref:`Rebin <algm-Rebin>`
   - An algorithm for altering the binning of the data in a
   :ref:`workspace <Workspace>`.
-  :ref:`LoadRaw <algm-LoadRaw>`
   - An algorithm for loading the data from a RAW file into a
   :ref:`workspace <Workspace>`.
-  :ref:`GroupDetectors <algm-GroupDetectors>`
   - An algorithm for grouping two or more detectors into a larger
   'logical' detector.


Writing your own algorithm
--------------------------

A primer for this is `here <http://www.mantidproject.org/Writing_an_Algorithm>`__. Also look at the
examples in the :ref:`UserAlgorithms <UserAlgorithms>` directory of your
Mantid installation.



.. categories:: Concepts
