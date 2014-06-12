.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This does a least squares fit between indexed peaks and Q values for a
set of runs producing an overall leastSquare orientation matrix.

Get estimates of the standard deviations of the parameters, by
approximating chisq by a quadratic polynomial through three points and
finding the change in the parameter that would cause a change of 1 in
chisq. (See Bevington, 2nd ed., pg 147, eqn: 8.13 ) In this version, we
calculate a sequence of approximations for each parameter, with delta
ranging over 10 orders of magnitude and keep the value in the sequence
with the smallest relative change.

.. categories::
