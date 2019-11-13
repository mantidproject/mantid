.. _Poisson:

=====================
Poisson Cost Function
=====================

.. index:: Poisson

Description
-----------

The Poisson cost function is designed to be applied to data, which has a low number of counts.
It is used to measure the discrepancy between the observed and fitted value and this is minimised to find the
best fit. Also known as the Poisson deviance or Poisson log-linear model, it uses the following function:

.. math:: 2\sum_{i}^{N} \{ y_{i} \textup{ log}\left ( \frac{y_{i}}{\hat{\mu}_{i}} \right ) - (y_{i} - \hat{\mu}_{i}) \} \textup{ for } \hat{\mu}_{i} > 0

.. math:: 2\sum_{i}^{N}  y_{i} \textup{ for } \hat{\mu}_{i} = 0

where,

:math:`y_{i}` is the number of events predicted by the model to be in the i-th bin

and :math:`\hat{\mu}_{i}` is the observed number of events in the i-th bin.

The first term is equal to binomial deviance which represents twice the sum of the observed values time the log of the observed over fitted.
The second is the difference between the observed and fitted values which is usually zero or very small.

For Poisson-distributed data using cost functions, such as least squares, can lead to biased results.
This is because it is designed for Gaussian-distributed data and Poisson distributed data is only well approximated by a Gaussian
when the number fo events is large which can be difficult to achieve in practice. This cost
function uses a more appropriate measure which is based on the maximum likelihood estimator for Poisson distribution.

Please note this should not be used with data where the y-axis is not counts.

Example
-------

Given a workspace with low counts, a fit can be done in a script as follows:
``Fit(Function=f,InputWorkspace=workspace,Output="outputName",CreateOutput=True,CostFunction="Poisson",Minimizer="Levenberg-MarquardtMD")``

To demonstrate the difference between using the Poisson cost function and Least Squares cost function the plot below show the value of a given
parameter over a series of fits with low count data. The Poisson model gives a much more consistent result and means no special treatment is required for zero counts

.. image:: /images/Poisson.png
   :alt: Poisson cost function compared with least squares cost function

References
----------

[1]  `Rodríguez, G. (2007). Lecture Notes on Generalized Linear Models <https://data.princeton.edu/wws509/notes/c4.pdf>`_.


