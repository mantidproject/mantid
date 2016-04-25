
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning:: This is an early, experimental version of the algorithm.

The input and output workspaces are workspace groups where every
element is an image workspace. The input workspace must have one image
workspace per projection from a tomography imaging experiment. The
output workspace will have one image workspace for every slice of the
output reconstructed volume.

Methods supported: FBP (following the Tomopy implementation [TomoPy2014]).


References:

.. [TomoPy2014] Gursoy D, De Carlo F, Xiao X,
  Jacobsen C. (2014). TomoPy: a framework for the analysis of
  synchrotron tomographic data. J. Synchrotron Rad. 21. 1188-1193
  doi:10.1107/S1600577514013939


Usage
-----

**Example - LoadReconstructProjections**

.. testcode:: python

    # Load an image
    wsg_name = 'images'
    wsg = LoadFITS(Filename='FITS_small_01.fits', LoadAsRectImg=1, OutputWorkspace=wsg_name)

.. testcleanup:: LoadReconstructProjections

    import os

    DeleteWorkspace(wsg_name)

Output:

.. testoutput:: LoadReconstructProjections

.. categories::

.. sourcelink::
