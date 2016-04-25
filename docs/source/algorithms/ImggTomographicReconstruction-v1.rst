
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------


The input and output workspaces are workspaces. The input workspace must have one image workspace per projection. The output workspace will have one image workspace for every slice of the output reconstructed volume.


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
