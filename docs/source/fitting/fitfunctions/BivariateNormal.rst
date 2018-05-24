.. _func-BivariateNormal:

===============
BivariateNormal
===============

.. index:: BivariateNormal

Description
-----------

Provides a peak shape function interface for a peak on one time slice of
a Rectangular detector.

.. math:: V=\mathrm{Background} + \mathrm{Intensity}\times\mathrm{Normal}( \mu_x, \mu_y,\sigma_x,\sigma_y)

The Normal(..) is the Normal probability density function. Its integral
over all x(col) and y(row) values is one. This means that Intensity is
the total intensity with background removed.

.. attributes::

There is only one Attribute: **CalcVariances**. This attribute is
boolean.

If true, the variances are calculated from the data, given the means,
variances and covariance. Otherwise they will become parameters and fit.

CalcVariances = true gives better/more stable results for peaks interior
to the Rectangular Detector. For peaks close to the edge, CalcVariances
should be false.

.. properties::

#. Background - The background of the peak
#. Intensity - The intensity of data for the peak on this time slice
#. Mcol - The col(x) of the center of the peak
#. Mrow - The row(y) of the center of the peak on this slice
#. ------- If CalcVariances is false, the following 3 parameters are
   also fit---------
#. SScol -The variance of the column(x) values in the peak for this time
   slice
#. SSrow - The variance of the row(y) values in the peak for this time
   slice
#. SSrc - The covariance of the row(x) and column(y) values in the peak
   for this time slice

Usage
~~~~~

The workspace can be "any" MatrixWorkspace where

#. dataY(1) is the column(x) values for the pixels to be considered
#. dataY(2) is the row(y) values for the pixels to be considered
#. dataY(0)is the experimental data at the corresponding row and column
   for a panel and time slice( or merged time slices or...)

The data can have missing row and column values and need not represent a
square or contiguous subregion of a panel

The values for out in function1D are, for each pixel, the difference of
V(see formula) and dataY(0).

.. categories::

.. sourcelink::
