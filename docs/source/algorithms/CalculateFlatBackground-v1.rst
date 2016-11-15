.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a list of spectra and for each spectrum 
calculates an average count rate in a region, usually a region when 
there are only background neutrons. This count rate is then 
subtracted from the counts in all the spectrum's bins. However, no 
bin will take a negative value as bins with count rates less than 
the background are set to zero (and their error is set to the 
backgound value).

The average background count rate is estimated in one of three ways. 
When *Mode* is set to "Mean" it is the sum of the values in the bins 
in the background region divided by the width of the X range. 
Selecting "Linear Fit" sets the background value to the height in 
the centre of the background region of a line of best fit through 
that region. The "Averaging Window" *Mode* in turn calculates a 
moving window average over each spectrum and takes the minimum as 
the background. The values of the bins half-*AveragingWindowWidth* 
from the beginning and end of a spectrum are taken as-is.

The error on the background value is only calculated when "Mean" is
used. It is the errors in all the bins in the background region summed
in quadrature divided by the number of bins. This background error value
is added in quadrature to the errors in each bin.

Usage
-----

**Example - Subtracting background using Linear Fit (using a distribution):**

.. testcode:: ExSubLinFit

   import numpy as np

   y = [3, 1, 1, 1, 7, -3]
   x = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
   input = CreateWorkspace(x, y, Distribution=True)

   output = CalculateFlatBackground('input',
                                    StartX=2,
                                    EndX=4,
                                    Mode='Linear Fit',
                                    OutputMode='Subtract Background')

   print 'Values with subtracted background:', np.around(output.readY(0))

Output:

.. testoutput:: ExSubLinFit

   Values with subtracted background: [ 2.  0.  0.  0.  6.  0.]

**Example - Returning background using Mean (using a histogram):**

.. testcode:: ExReturnMean

   import numpy as np

   y = [3, 4, 2, 3, -3]
   x = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
   input = CreateWorkspace(x, y)

   output = CalculateFlatBackground('input',
                                    StartX=1,
                                    EndX=3,
                                    Mode='Mean',
                                    OutputMode='Return Background')

   print 'Calculated Mean background:', np.around(output.readY(0))

Output:

.. testoutput:: ExReturnMean

   Calculated Mean background: [ 3.  3.  3.  3.  3.]

**Example - Returning background using Moving Average (using a histogram):**

.. testcode:: ExReturnMovingAverage

   import numpy as np
   from scipy.constants import pi

   def spectrum(x):
       # A fancy triple-peak-shaped spectrum
       z = x / 10.0 - 0.5
       return np.sin(5.5 * (z + 0.1) * pi) + 2.0 * np.exp(-((z / 0.1)**2)) + 1.0

   # Equidistant x grid. Represents bin boundaries
   x = np.arange(0.5, 9.1, 0.2)
   # y is a bin shorter than x and has to be evaluated at bin centres.
   y = spectrum(x[:-1] + 0.5 * (x[1] - x[0]))
   input = CreateWorkspace(x, y)

   output = CalculateFlatBackground('input',
                                    AveragingWindowWidth=3,
                                    Mode='Moving Average',
                                    OutputMode='Return Background')

   print('Background using moving window average: {0:.4}'.format(output.readY(0)[0]))
   print('True minimum: {0:.4}'.format(np.amin(input.readY(0))))

Output:

.. testoutput:: ExReturnMovingAverage

   Background using moving window average: 0.09483
   True minimum: 0.04894


.. categories::

.. sourcelink::
