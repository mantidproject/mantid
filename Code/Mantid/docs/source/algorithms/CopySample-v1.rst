.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm copies some/all the sample information from one workspace
to another.For MD workspaces, if no input sample number is specified, or
not found, it will copy the first sample. For MD workspaces, if no
output sample number is specified (or negative), it will copy to all
samples. The following information can be copied:

-  Name
-  Material
-  Sample environment
-  Shape
-  Oriented lattice

One can copy the orientation matrix only. To do this, select both
CopyLattice and CopyOrientationOnly. If only CopyOrientationOnly is
true, the algorithm will throw an error.

.. categories::
