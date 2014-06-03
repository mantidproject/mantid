.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Takes an input TOF spectrum and converts it to Y-space using the
:ref:`algm-ConvertToYSpace` algorithm. The result is then
fitted using the ComptonPeakProfile function using the given mass to
produce an estimate of the peak area. The input data is normalised by
this value.

The algorithm has 4 outputs:

-  the input data normalised by the fitted peak area;
-  the input data (without normalisation) converted Y-space;
-  the fitted peak in Y-space;
-  the input data converted to Y and then symmetrised about Y=0.

If the sum option is requested then all input spectra are rebinned, in
steps of 0.5 :math:`A^-1`, to a common Y grid and then summed to give a
single spectrum.

.. categories::
