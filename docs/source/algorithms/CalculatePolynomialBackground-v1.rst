
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates backgrounds for the histograms in a workspace by fitting a polynomial to ranges given by *XRanges*. The fitting is done using :ref:`algm-Fit`. The backgrounds are returned as the *OutputWorkspace* which can be subsequently subtracted from the original workspace. The degree of the polynomial can be given by *Degree*. A zeroth degree polynomial would correspond to fitting a constant value. The *XRanges* property is a list of range start and range end value pairs in the X axis units. Overlapping ranges are merged. If no *XRanges* are given, full histograms are used.

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
   print('Peak height with background: {}'.format(Y))
   Y = ws_bkg_subtr.readY(0)[peakIndex]
   print('Background subtracted peak height: {}'.format(Y))

Output:

.. testoutput:: CalculatePolynomialBackgroundExample

   Peak height with background: 10.3
   Background subtracted peak height: 10.0

.. categories::

.. sourcelink::

