.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is useful for increasing the time resolution of spectra
whose bins have large numbers of counts which vary smoothly e.g. monitor
spectra.

The "params" property defines the new bin boundaries using the same
syntax as in :ref:`Rebin <algm-Rebin>`. That is, the first number is the first
bin boundary and the second number is the width of the bins. Bins are
created until the third number would be exceeded, the third number is
the x-value of the last bin. There can be further pairs of numbers, the
first in the pair will be the bin width and the last number the last bin
boundary.

The bin immediately before the specified boundaries :math:`x_2`,
:math:`x_3`, ... :math:`x_i` is likely to have a different width from
its neighbors because there can be no gaps between bins. Rebin ensures
that any of these space filling bins cannot be less than 25% or more
than 125% of the width that was specified.

To calculate the y-values the input spectra are approximated with a time
series where the value at the center of each bin is the mean count
rate over the bin. This series is interpolated by calculating cubic
splines that fit this series and evaluating the splines at the centers
of the requested bin. The splines have natural boundary conditions and
zero second derivative at the end points, they are calculated using the
`gsl <http://www.gnu.org/software/gsl/manual/html_node/Interpolation-Types.html>`__.

The errors on the count rates are estimated as a weighted mean of the
errors values for the nearest input bin centers. These weights are
inversely proportional to the distance of the output bin center to the
respective input bin data points.

Example Rebin param strings
###########################

The input rebin parameter string is identical to that used by :ref:`Rebin <rebin-example-strings>`.

Usage
-----

**Example - rebin with single bin width:**

.. testcode::

   # create histogram workspace
   dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10)
   dataY = [1,1,1,1,1,1,1,1,1] # or use dataY=[1]*9
   ws = CreateWorkspace(dataX, dataY)

   # rebin from min to max with size bin = 0.5
   ws = InterpolatingRebin(ws, 0.5)

   print("First 5 rebinned X values are: {}".format(ws.readX(0)[0:5]))
   print("First 5 rebinned Y values are: {}".format(ws.readY(0)[0:5]))

Output:

.. testoutput::

   First 5 rebinned X values are: [0.  0.5 1.  1.5 2. ]
   First 5 rebinned Y values are: [0.5 0.5 0.5 0.5 0.5]


For further examples with more complex parameter strings see :ref:`Rebin examples <rebin-usage>`.

.. categories::

.. sourcelink::
