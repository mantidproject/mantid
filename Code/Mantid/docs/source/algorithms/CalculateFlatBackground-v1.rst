.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a list of spectra and for each spectrum calculates
an average count rate in the given region, usually a region when there
are only background neutrons. This count rate is then subtracted from
the counts in all the spectrum's bins. However, no bin will take a
negative value as bins with count rates less than the background are set
to zero (and their error is set to the backgound value).

The average background count rate is estimated in one of two ways. When
*Mode* is set to "Mean" it is the sum of the values in the bins in the
background region divided by the width of the X range. Selecting "Linear
Fit" sets the background value to the height in the centre of the
background region of a line of best fit through that region.

The error on the background value is only calculated when "Mean" is
used. It is the errors in all the bins in the background region summed
in quadrature divided by the number of bins. This background error value
is added in quadrature to the errors in each bin.

Usage
-----

**Example - Subtracting background using Linear Fit:**

.. testcode:: ExSubLinFit

   import numpy as np

   y = [3, 1, 1, 1, 7, -3]
   x = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5]
   input = CreateWorkspace(x,y)

   output = CalculateFlatBackground('input',
                                    StartX=2,
                                    EndX=4,
                                    Mode='Linear Fit',
                                    OutputMode='Subtract Background')

   print 'Values with subtracted background:', np.around(output.readY(0))

Output:

.. testoutput:: ExSubLinFit

   Values with subtracted background: [ 2.  0.  0.  0.  6.  0.]

**Example - Returning background using Mean:**

.. testcode:: ExReturnMean

   import numpy as np

   y = [3, 4, 2, 3, -3]
   x = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
   input = CreateWorkspace(x,y)

   output = CalculateFlatBackground('input',
                                    StartX=1,
                                    EndX=3,
                                    Mode='Mean',
                                    OutputMode='Return Background')

   print 'Calculated Mean background:', np.around(output.readY(0))

Output:

.. testoutput:: ExReturnMean

   Calculated Mean background: [ 3.  3.  3.  3.  3.]

.. categories::
