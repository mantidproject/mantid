.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the backgrounds for the histograms in a workspace. The backgrounds can be returned as-is, or directly subtracted from the input workspace depending on *OutputMode*.

There are three modes of operation: **Linear Fit** fits a line to the range specified by *StartX* and *EndX* and uses the mid-point as the background. **Mean** calculates the mean in same range. Finally, **Moving Average** calculates a rolling average with cyclic boundary conditions over the histograms of the input workspace. Width of the averaging window can be specified by *AveragingWindowWidth*.

The error of the background is only calculated when **Mean** or **Moving Average** is used. It is the errors in all the bins in the background region or averaging window, summed in quadrature and divided by the number of bins. This background error value is added in quadrature to the errors in each bin of the input workspace if **Subtract Background** is specified, otherwise returned in the background workspace.

If *NullifyNegativeValues* is true, the background is set to the corresponding y value for the bins/points which would become negative when the background is subtracted. In these cases, the errors are set to either the background value or to the original error, whichever is greater.

.. note::
   Generally, using **Subtract Background** directly or subtracting the returned background manually later produces the same end result. This is not true, however, for the errors if *NullifyNegativeValues* is set. In this case **Subtract Background** will set the errors for the otherwise negative y values either to the background value or to the original error, whereas manual subtraction will add the background errors to the original ones in quadrature.

.. note::
   Care should be taken when subtracting the returned background from other workspaces than the input workspace if *NullifyNegativeValues* is set. For backgrounds corresponding to the y value which would be zeroed, this algorithm returns the original y values instead of the actual background.

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

   print('Values with subtracted background: {}'.format(np.around(output.readY(0))))

Output:

.. testoutput:: ExSubLinFit

   Values with subtracted background: [2. 0. 0. 0. 6. 0.]

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

   # Note how some bins in the output workspace will be different from
   # 3 (even negative!). By default, NullifyNegativeValues will be set
   # to true, and subtracting the output from the input workspace will
   # set these bins to zero.
   print('Calculated Mean background: {}'.format(np.around(output.readY(0))))
   subtracted = input - output
   print('Background subtracted: {}'.format(np.around(subtracted.readY(0))))

Output:

.. testoutput:: ExReturnMean

   Calculated Mean background: [ 3.  3.  2.  3. -3.]
   Background subtracted: [0. 1. 0. 0. 0.]

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
