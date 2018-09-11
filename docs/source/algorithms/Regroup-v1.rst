.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Regroups data with new bin boundaries to ensure that bins have minimum
width determined by the parameter :math:`\Delta x_i\,`, but ensuring the
bin boundaries are always coincident with original bin boundaries. The
'params' property defines new boundaries in intervals
:math:`x_i-x_{i+1}\,`. Positive :math:`\Delta x_i\,` define constant
minimum bin width, whilst negative ones create logarithmic binning
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`

The difference between :ref:`algm-Rebin` and Regroup is that in the former the data
in the original bins may be divided by the new bin boundaries. In the
latter case, new bins are created only by combining whole bins. This is
true also for the ends of the regrouped array: if the bin boundaries are
990,1010,1030,1050,...,1210, then "params" = "1000,25,1200" yields a
workspace with bin boundaries 1010,1050,1090,1130,1170.

Usage
-----

.. testcode::

  import numpy as np

  # Create a workspace
  ws = CreateSampleWorkspace()

  # Regroup the bins such that no bin has width smaller than 300
  rgws = Regroup(ws, [0,300,20000])

  # Check the result
  print('Bin width in ws   is {}'.format(ws.readX(0)[1] - ws.readX(0)[0]))
  print('Bin width in rgws is {}'.format(rgws.readX(0)[1] - rgws.readX(0)[0]))

  # Using numpy array calculations check that all  bins in the regrouped workspace
  # are wider than 300

  # Get the x vector of the first spectrum
  x = rgws.readX(0)

  # Construct an array containing the differences of the consecutive elements in array x
  # x[1:] is a slice of x having all elements of x excluding the first one
  # x[:-1] is a slice of x having all elements of x excluding the last one
  widths = x[1:] - x[:-1]

  # Check that all elements in widths are greater than 300
  print(np.all(widths >= 300))

Output
######

.. testoutput::

  Bin width in ws   is 200.0
  Bin width in rgws is 400.0
  True

.. categories::

.. sourcelink::
