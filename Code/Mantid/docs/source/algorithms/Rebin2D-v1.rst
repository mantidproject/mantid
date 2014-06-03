.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The bin parameters are used to form an output grid. A positive
:math:`\Delta x_i\,` makes constant width bins, whilst negative ones
create logarithmic binning using the formula
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`. The overlap of the polygons
formed from the old and new grids is tested to compute the required
signal weight for the each of the new bins on the workspace. The errors
are summed in quadrature.

Requirements
------------

The algorithms currently requires the second axis on the workspace to be
a numerical axis so :ref:`_algm-ConvertSpectrumAxis` may
need to run first.

.. categories::
