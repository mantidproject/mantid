.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm written using this paper: J. Appl. Cryst. (2013). 46, 663-671

Objective algorithm to separate signal from noise in a
Poisson-distributed pixel data set

T. Straaso/, D. Mueter, H. O. So/rensen and J. Als-Nielsen

Synopsis: A method is described for the estimation of background level
and separation of background pixels from signal pixels in a
Poisson-distributed data set by statistical analysis. For each
iteration, the pixel with the highest intensity value is eliminated from
the data set and the sample mean and the unbiased variance estimator are
calculated. Convergence is reached when the absolute difference between
the sample mean and the sample variance of the data set is within k
standard deviations of the variance, the default value of k being 1. The
k value is called SigmaConstant in the algorithm input.

.. categories::
