.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm written using the paper referenced below which has a very good 
description. 

This algorithm estimates the background level and separates the background 
from signal data in a Poisson-distributed data set by statistical analysis. 
For each iteration, the bins/points with the highest intensity value are 
eliminated from the data set and the sample mean and the unbiased variance 
estimator are calculated. Convergence is reached when the absolute 
difference between the sample mean and the sample variance of the data set 
is within k standard deviations of the variance, the default value of k 
being 1. The k value is called ``SigmaConstant`` in the algorithm input.

References
----------
**Objective algorithm to separate signal from noise in a Poisson-distributed pixel data set**
by T. |Straaso|, D. Mueter, H. O. |Sorensen| and J. Als-Nielsen Strass
`J. Appl. Cryst. (2013). 46, 663-671 <http://dx.doi.org/10.1107/S0021889813006511>`__

.. |Straaso| unicode:: Straas U+00F8 ..
   :ltrim:
.. |Sorensen| unicode:: S U+00F8 rensen ..
   :trim:

.. categories::

.. sourcelink::
