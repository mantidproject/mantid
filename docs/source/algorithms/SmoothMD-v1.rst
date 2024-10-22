
.. algorithm::

.. summary::

.. relatedalgorithms::

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

WidthVector and Functions
-------------------------

The WidthVector property defines the width of the smoothing function in each dimension in units of pixels. If "Hat" is chosen as the smoothing function then the WidthVector must only contain odd integer values, such that the filter will have a central pixel. For the "Gaussian" option the width is defined as the full width at half maximum (FWHM). Where the Gaussian function in 1D is defined as

.. math:: G(x) = \frac{1}{\sqrt{2\pi} \sigma} e^{-\frac{x^2}{2\sigma^2}} ,

the FWHM is given by

.. math:: \text{FWHM} = 2 \sqrt{2\text{ln}2}\sigma .

The Gaussian filter uses values which are integrated over the width of the pixel and is truncated at the point where the value of the pixel falls to less than 0.02 of the central pixel.


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

   print('Smoothed has {0} points'.format(smoothed.getNPoints()))

Output:

.. testoutput:: SmoothMDExample

   Smoothed has 2500 points

.. categories::

.. sourcelink::
