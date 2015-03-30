
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Provides smoothing of :ref:`MDHistoWorkspace <MDHistoWorkspace>` in n-dimensions. The WidthVector relates to the number of pixels to include in the width for each dimension. *WidthVector* **must contain entries that are odd numbers**.

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

