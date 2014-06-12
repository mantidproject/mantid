.. algorithm::

.. summary::

.. alias::

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

The difference between Rebin and Regroup is that in the former the data
in the original bins may be divided by the new bin boundaries. In the
latter case, new bins are created only by combining whole bins. This is
true also for the ends of the regrouped array: if the bin boundaries are
990,1010,1030,1050,...,1210, then "params" = "1000,25,1200" yields a
workspace with bin boundaries 1010,1050,1090,1130,1170.

.. categories::
