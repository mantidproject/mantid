.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Masks bins in a workspace. Masked bins should properly be regarded as
having been completely removed from the workspace. Bins falling within
the range given (even partially) are masked, i.e. their data and error
values are set to zero and the bin is added to the list of masked bins.
This range is masked for all spectra in the workspace (though the
workspace does not have to have common X values in all spectra).

At present, although the zeroing of data will obviously be 'seen' by all
downstream algorithms. Only
:ref:`algm-DiffractionFocussing` (version 2) and
:ref:`algm-Q1D` have been modified to take account of masking. Several
algorithms (e.g. :ref:`algm-Rebin`, :ref:`algm-CropWorkspace`)
have been modified to properly propagate the masking.


Usage
-----

**Masking of a small workspace**

.. testcode:: exMaskBinsSimple

   # Create workspace with 10 bins of width 10
   ws = CreateSampleWorkspace(BankPixelWidth=1, Xmax=100, BinWidth=10)

   # Mask a range of X-values
   wsMasked = MaskBins(ws,XMin=16,XMax=32)

   # Show Y values in workspaces
   print("Before masking: {}".format(ws.readY(0)))
   print("After masking: {}".format(wsMasked.readY(0)))


Output:

.. testoutput:: exMaskBinsSimple

   Before masking: [ 0.3  0.3  0.3  0.3  0.3 10.3  0.3  0.3  0.3  0.3]
   After masking: [ 0.3  0.   0.   0.   0.3 10.3  0.3  0.3  0.3  0.3]


Related Algorithms
------------------

RemoveBins
##########

:ref:`algm-RemoveBins` can work in several ways, if the bins are at
the edges of the workspace they will be removed, and that will in many
ways act like Masking the bins. If the bins are in the middle of the
workspace then the effect depends on the type of interpolation, but
importantly these bins will continue to influence future algorithms as
opposed to masked bins. For example, with no interpolation
:ref:`algm-RemoveBins` sets the bin values to 0. This 0 values will
be included in the summing performed in DiffractionFocussing, pushing
down the values in that region. MaskBins is more clever. While if you
look at the data, it will appear that it has simply set the values to 0.
It has also set a series of flags inside that mark those bins to not be
included in further claculations. This means that when you Focus the
data these values are simply missed out of the summing that is
performed.

.. categories::

.. sourcelink::
