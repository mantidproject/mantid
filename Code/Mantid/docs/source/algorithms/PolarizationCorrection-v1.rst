.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs wavelength polarization correction on a TOF reflectometer spectrometer.

Algorithm is based on the the paper Fredrikze, H, et al. "Calibration of a polarized neutron reflectometer" Physica B 297 (2001).

Polarizer and Analyzer efficiencies are calculated and used to perform an intensity correction on the input workspace. The input workspace(s) are in units of wavelength
inverse angstroms.

In the ideal case :math:`P_{p} = P_{a} = A_{p} = A_{a} = 1`

:math:`\rho = \frac{P_{a}}{P_{p}}` where rho is bounded by, but inclusive of 0 and 1.
Since this ratio is wavelength dependent, rho is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\rho(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`

:math:`\alpha = \frac{A_{a}}{A_{p}}` where alpha is bounded by, but inclusive of 0 and 1.
Since this ratio is wavelength dependent, alpha is a polynomial, which is expressed as a function of wavelength. For example:
:math:`\alpha(\lambda) =\sum\limits_{i=0}^{i=2} K_{i}\centerdot\lambda^i`, can be provided as :math:`K_{0}, K_{1}, K_{2}`

.. categories::
