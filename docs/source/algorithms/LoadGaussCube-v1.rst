
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

.. code-block:: python

    from mantid.simpleapi import *

    ws = LoadGaussCube(Filename="gauss_cube_example.cube", Names=['[H,0,0]','[0,K,0]','[0,0,L]'], Frames='HKL,HKL,HKL', Units='rlu,rlu,rlu')

.. categories::

.. sourcelink::

