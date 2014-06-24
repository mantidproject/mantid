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

Peak parameters must have the following parameters, which are case
sensitive in input `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_

| ``1. spectrum``
| ``2. centre``
| ``3. height``
| ``4. width (FWHM)``
| ``5. backgroundintercept (a0)``
| ``6. backgroundslope (a1)``
| ``7. A2``
| ``8. chi2``

Output
######

| ``Output will include``
| ``1. pure peak``
| ``2. pure background (with specified range of FWHM (int))``
| ``3. peak + background``

.. raw:: mediawiki

   {{AlgorithmLinks|GeneratePeaks}}

Category:Algorithms

.. categories::
