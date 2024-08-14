.. _func-PearsonIV:

=========
PearsonIV
=========

.. index:: PearsonIV

Description
-----------

The PearsonIV peak shape is used to fit the prompt pulse in time-of-flight spectra.
It differs from the traditional definition of the PearsonIV distribution [1]_ in that the Mantid definition has the peak
centre (`Centre``) shifted so that it coincides with the peak maximum.

The function is defined as
.. math:: \frac{I}{\sigma}N\left[1 + \left(\frac{x - \lambda  - \nu\sigma/(2m)\right)^{2}}{\sigma}\right]^{-m}\exp\left(-\nu \arctan(\frac{x - \lambda - \nu\sigma/(2m)}{\sigma}) \right)

where:

- :math:`I` is the integrated intensity (area) of the peak (parameter name ``Intensity``)
- :math:`\lambda` is the peak centre (parameter name ``Centre``).
- :math:`\nu` is the parameter ``Skew``
- :math:`m` is the parameter ``Exponent`` (valid for :math:`m > 0.5`)
- :math:`\sigma` is the parameter ``Sigma`` (valid for :math:`\sigma > 0`)
- :math:`N = \frac{2^{2m-2}\left|\Gamma(m+i\nu/2)\right|^2}{\pi\sigma\Gamma(2m-1)}` is the normalisation


.. attributes::

.. properties::

References
----------

.. [1] Pearson, Karl. "X. Contributions to the mathematical theory of evolution.—II. Skew variation in homogeneous material." Philosophical Transactions of the Royal Society of London.(A.) 186 (1895): 343-414.

.. categories::

.. sourcelink::
