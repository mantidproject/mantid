.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to replicate a n dimensional :ref:`MDHistoWorkspace <MDHistoWorkspace>` in a stack to make a n+1 dimensional :ref:`MDHistoWorkspace <MDHistoWorkspace>`. A *ShapeWorkspace* provides the shape, extents, names and units of the final *OutputWorkspace*, which is also a :ref:`MDHistoWorkspace <MDHistoWorkspace>`.

Validation
######################################

The *InputWorkspace* and *ShapeWorkspace*, are expected to match over the initial n dimensions of the *InputWorkspace*. Names and Units are not checked, and are automatically taken from the *ShapeWorkspace*. However, Extents (Min, Max), and number of bins in each dimension are checked in order. The algorithm will fail to continue if these are not seen to match. 


Limitations 
#####################

The current implementation of this algorithm will not handle dimensions that have been integrated out (have 1 bin) as though the dimensionality of the workspace is n-1.

Usage
-----

**Example - Replicating a slice:**

.. testcode:: ExampleSimple

   print "Hello World"

Output:

.. testoutput:: ExampleSimple

   Hello World

.. categories::

.. sourcelink::
