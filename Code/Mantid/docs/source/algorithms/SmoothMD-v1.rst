
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Provides smoothing of :ref:`MDHistoWorkspace <MDHistoWorkspace>` in n-dimensions. The WidthVector relates to the number of pixels to include in the width for each dimension. *WidthVector* **must contain entries that are odd numbers**.

A *InputNormalizationWorkspace* may optionally be provided. Such workspaces must have exactly the same shape as the *InputWorkspace*. Where the signal values from this workspace are zero, the corresponding smoothed value will be NaN. Any un-smoothed values from the *InputWorkspace* corresponding to zero in the *InputNormalizationWorkspace* will be ignored during neighbour calculations, so effectively omitted from the smoothing altogether.
Note that the NormalizationWorkspace is not changed, and needs to be smoothed as well, using the same parameters and *InputNormalizationWorkspace* as the original data.

.. figure:: /images/PreSmooth.png
   :alt: PreSmooth.png
   :width: 400px
   :align: center
   
   No smoothing
   
.. figure:: /images/Smoothed.png
   :alt: PreSmooth.png
   :width: 400px
   :align: center
   
   Smooth with WidthVector=5


Usage
-----

**Example - SmoothMD**

.. testcode:: SmoothMDExample

   ws = CreateMDWorkspace(Dimensions=2, Extents=[-10,10,-10,10], Names='A,B', Units='U,U')
   FakeMDEventData(InputWorkspace=ws, PeakParams='100000,-5,0,1')
   FakeMDEventData(InputWorkspace=ws, PeakParams='100000,5,0,1')
   histogram = BinMD(InputWorkspace=ws, AlignedDim0='A,-10,10,50', AlignedDim1='B,-10,10,50', OutputExtents='-10,10,-10,10,-10,10', OutputBins='10,10,10')
   # plotSlice(histogram)
   smoothed = SmoothMD(InputWorkspace=histogram, WidthVector=5, Function='Hat')
   # plotSlice(smoothed)

   print 'Smoothed has %i points' % smoothed.getNPoints()

Output:

.. testoutput:: SmoothMDExample

   Smoothed has 2500 points

.. categories::

