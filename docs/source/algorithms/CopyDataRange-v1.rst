.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes a continuous block of data from an input workspace and places this data into a destination 
workspace, replacing the data in the destination workspace at which the insertion is specified.  The block of 
data to be used is specified by entering spectra indices and x indices within the input workspace into the algorithm.
The insertion location is then specified by an InsertionYIndex and InsertionXIndex within the destination workspace.

Note that this algorithm will replace not only the Y values, but also the E values within the destination workspace. The 
original input workspace remains unchanged.

.. categories::

.. sourcelink::
