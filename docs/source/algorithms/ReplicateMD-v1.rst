
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates a higher dimensional dataset by replicating along an additional axis. The syntax is similar to that used by `Horace <http://horace.isis.rl.ac.uk/Reshaping_etc#replicate>`__.

The *ShapeWorkspace* input defines the shape of the *OutputWorkspace*, but not the contents. The *DataWorkspace* provides the data contents in the lower dimensionality cut, which will be replicated over. This algorithm operates on :ref:`MDHistoWorkspace <MDHistoWorkspace>` inputs and provides a :ref:`MDHistoWorkspace <MDHistoWorkspace>` as an output.

The *ShapeWorkspace* and *DataWorkspace* can have any number of extra integrated trailing dimensions. For example the *ShapeWorkspace* can have shape ``[10, 5, 1, 1]``
(where each number in the list is a number of bins in the corresponding dimension) and the *DataWorkspace* can have either ``[10, 1, 1, 1]`` or ``[1, 5, 1, 1]``.
But the following arrangement will cause a run-time error: shape ``[10, 1, 5, 1]``, data ``[1, 1, 5, 1]``. In such case the dimensions can be re-arranged using
:ref:`algm-TransposeMD` algorithm.

Usage
-----

**Example - ReplicateMD 1D to 2D**

.. testcode:: ReplicateMDExample1D

   import numpy as np
   data = CreateMDHistoWorkspace(2, SignalInput=np.arange(20*30), ErrorInput=np.arange(20*30), NumberOfEvents=np.arange(20*30), Extents=[-10, 10, -1,1], NumberOfBins=[20, 30], Names='E,Qx', Units='MeV,A^-1')
   shape = CreateMDHistoWorkspace(3, SignalInput=np.tile([1], 20*30*10), ErrorInput=np.tile([1], 20*30*10), NumberOfEvents=np.tile([1], 20*30*10), Extents=[-1,1, -10, 10, -10,10], NumberOfBins=[30,20,10], Names='Qx,E,Qy', Units='A^-1, MeV, A^-1')

   replicated = ReplicateMD(ShapeWorkspace=shape, DataWorkspace=data)

   print('Num dims: {}'.format(replicated.getNumDims()))
   print('Num points: {}'.format(replicated.getNPoints()))

Output:

.. testoutput:: ReplicateMDExample1D

   Num dims: 3
   Num points: 6000

.. categories::

.. sourcelink::
