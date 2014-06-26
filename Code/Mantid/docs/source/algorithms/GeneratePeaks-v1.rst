.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Generate a workspace by summing over the peak functions. The peaks'
parameters are given in a `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_.

Peak Parameters
###############

Peak parameters must have the following columns, which are case
sensitive in input `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_

+------+--------------------+-------+
|Column|          Name      |Comment|
+======+====================+=======+
|  1   | spectrum           |       |
+------+--------------------+-------+
|  2   | centre             |       |
+------+--------------------+-------+
|  3   | height             |       |
+------+--------------------+-------+
|  4   | width              | FWHM  |
+------+--------------------+-------+
|  5   | backgroundintercept|  A0   |
+------+--------------------+-------+
|  6   | backgroundslope    |  A1   |
+------+--------------------+-------+
|  7   | A2                 |  A2   |
+------+--------------------+-------+
|  8   | chi2               |       |
+------+--------------------+-------+


The background is a quadratic function 

.. math:: \mbox{A0}+\mbox{A1}*x+\mbox{A2}*x^2

Output
######

| ``Output will include``
| ``1. pure peak``
| ``2. pure background (with specified range of FWHM (int))``
| ``3. peak + background``



Category:Algorithms

.. categories::
