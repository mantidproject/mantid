.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a set of peaks at least three of which have been assigned Miller
indices, this algorithm will find the UB matrix, that best maps the
integer (h,k,l) values to the corresponding Q vectors. The set of
indexed peaks must include three linearly independent Q vectors. The
(h,k,l) values from the peaks are first rounded to form integer (h,k,l)
values. The algorithm then forms a possibly over-determined linear
system of equations representing the mapping from (h,k,l) to Q for each
indexed peak. The system of linear equations is then solved in the least
squares sense, using QR factorization.

.. categories::
