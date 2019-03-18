.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the background in a workspace using the spectrum ranges in :literal:`InputWorkspaceIndexSet`. If no spectrum ranges are given the whole input workspace is used.

The background can be calculated using two methods. **Per detector average** which groups the background spectrum together and divides it by the total number of spectra. 
This is done using :ref:`algm-GroupDetectors`. **Polynomial** uses :ref:`algm-Transpose` so the spectrum numbers 
are in the X (horizontal) axis and TOF channels are the vertical axis. Then the background is calculated by fitting a polynomial of the given degree to each TOF using the background spectra given in :literal:`InputWorkspaceIndexSet`. 
This is done using :ref:`algm-CalculatePolynomialBackground`. The value of CostFunction is passed to :ref:`algm-CalculatePolynomialBackground` as-is. The default option is ‘Least squares’ which uses the histogram errors as weights. 
This might not be desirable, e.g. when there are bins with zero counts and zero errors. An ‘Unweighted least squares’ option is available to deal with such cases. 
Once this has been done the workspace is then transposed again so it is in the same form as the input workspace.

Once the background has been calculated using one of these methods it is then subtracted from the input workspace.

Usage
-----

**Example - Subtracting background using Per detector average:**

.. testcode:: ExPerDetAve

   import numpy

   dataX = [1, 2, 3, 4, 5]
   background = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2]
   peak = [5, 5, 5, 5, 5]
   dataY = background + peak + background 
   #workspace has a background of 2 and a peak of 5 in the 2nd index
   ws = CreateWorkspace(dataX, dataY, NSpec = 5)

   ws_bkg_subtr = ReflectometryBackgroundSubtraction(ws, InputWorkspaceIndexSet = "0,1,3,4", BackgroundCalculationMethod = "Per Detector Average")

   Y = ws.readY(2)[0]
   print('Peak height with background: {}'.format(Y))
   Y = ws_bkg_subtr.readY(2)[0]
   print('Background subtracted peak height: {}'.format(Y))

Output:

.. testoutput:: ExPerDetAve

   Peak height with background: 5.0
   Background subtracted peak height: 3.0 

**Example - Subtracting background using Polynomial:**
.. testcode:: ExPoly

   import numpy

   #create a workspace with a polynomial background of degree 2 and a peak of 5 in the 5th spectra
   dataX = [1]
   polynomial = [1, 8, 13, 16, 17, 16, 13, 8, 1]
   peak = [0, 0, 0, 0, 5, 0, 0, 0, 0]
   dataY = [a + b for a, b in zip(polynomial, peak)]
   ws = CreateWorkspace(dataX, dataY, NSpec = 9)

   ws_bkg_subtr = ReflectometryBackgroundSubtraction(ws, InputWorkspaceIndexType='SpectrumNumber', InputWorkspaceIndexSet = "1-4,6-9", BackgroundCalculationMethod = "Polynomial", DegreeOfPolynomial = 2)

   Y = ws.readY(4)[0]
   print('Peak height with background: {}'.format(Y))
   Y = ws_bkg_subtr.readY(4)[0]
   print('Background subtracted peak height: {}'.format(Y))

Output:

.. testoutput:: ExPoly
   Peak height with background: 22.0
   Background subtracted peak height: 5.0
.. categories::

.. sourcelink::
