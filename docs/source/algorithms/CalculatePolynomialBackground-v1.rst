
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates backgrounds for the histograms in a workspace by fitting a polynomial to ranges given by *XRanges*. The fitting is done using :ref:`algm-Fit`. The backgrounds are returned as the *OutputWorkspace* which can be subsequently subtracted from the original workspace. The degree of the polynomial can be given by *Degree*. A zeroth degree polynomial would correspond to fitting a constant value. The *XRanges* property is a list of range start and range end value pairs in the X axis units. Overlapping ranges are merged. If no *XRanges* are given, full histograms are used.

The output background workspace contains errors as well. These are estimated for a histogram point at index :math:`i` by

.. math::
   E_{i} = \sum_{j} \left| \frac{\partial p}{\partial A_{j}}|_{x=x_{i}} \right| \Delta A_{j},

where :math:`p` is the fitted polynomial, :math:`A_{j}` the fitting coefficients and :math:`\Delta A_{j}` their estimated errors.

Usage
-----

**Example - CalculatePolynomialBackground**

.. testcode:: CalculatePolynomialBackgroundExample

   import numpy
   
   ws = CreateSampleWorkspace(Function='One Peak')
   # ws has a background of 0.3
   # There is a single peak around X = 10000 micro-s. Lets exclude it from
   # the background fitting using XRanges.
   bkg = CalculatePolynomialBackground(ws, XRanges=[0, 8000, 12000, 20000])
   ws_bkg_subtr = ws - bkg
   
   peakIndex = numpy.argmax(ws.extractY())
   Y = ws.readY(0)[peakIndex]
   E = ws.readE(0)[peakIndex]
   print('Peak height with background: {} +/- {:1.4f}'.format(Y, E))
   Y = ws_bkg_subtr.readY(0)[peakIndex]
   E = ws_bkg_subtr.readE(0)[peakIndex]
   print('Background subtracted peak height: {} +/- {:1.4f}'.format(Y, E))
   # In this case the increase in error is insignificant thanks to good
   # fitting statistics.

Output:

.. testoutput:: CalculatePolynomialBackgroundExample

   Peak height with background: 10.3 +/- 3.2094
   Background subtracted peak height: 10.0 +/- 3.2100

.. categories::

.. sourcelink::

