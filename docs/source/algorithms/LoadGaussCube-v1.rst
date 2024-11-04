
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to load Gaussian Cube files (for volumetric data) into a 3D :ref:`MDHistoWorkspace <MDHistoWorkspace>`.
The file is assumed to follow the XYZ order for flattening (i.e. the outer loop is over X coordinate and inner loop
is over Z).

Usage
-----

**Example - LoadGaussCube**

.. testcode:: exampleLoadGaussCube

    from mantid.simpleapi import *

    ws = LoadGaussCube(Filename="gauss_cube_example.cube", Names=['[H,0,0]','[0,K,0]','[0,0,L]'], Frames='HKL,HKL,HKL', Units='rlu,rlu,rlu')

    print(f"ws has {ws.getNumDims()} dimensions")
    for idim in range(ws.getNumDims()):
        dim = ws.getDimension(0)
        print(f"dimension {idim} has {dim.getNBins()} bins between {dim.getMinimum()} and {dim.getMaximum()}")

**Output:**

.. testoutput:: exampleLoadGaussCube

    ws has 3 dimensions
    dimension 0 has 3 bins between -10.0 and 10.0
    dimension 1 has 3 bins between -10.0 and 10.0
    dimension 2 has 3 bins between -10.0 and 10.0


.. categories::

.. sourcelink::
