.. _func-MagneticOrderParameter:

========================
Magnetic Order Parameter
========================

.. index:: MagneticOrderParameter

Description
-----------

A Magentic Order Parameter is defined as:

.. math:: y = A_0(1-(\frac{x}{Tc})^{\alpha})^{\beta}

where

-  :math:`A_0` - the amplitude
-  :math:`\alpha` - the balance parameter
-  :math:`\beta` - the critical exponent
-  :math:`Tc` - the critical temperature

The CriticalTemp (:math:`Tc`) needs to be set to the value at which the frequency hits 0. In the figure example below this is around 0.4 and so CritialTemp was set to 0.4 to achieve a fit.

.. figure:: /images/magorder_plot.png
   :alt: magorder_plot.png


Examples
--------

This has been used for Muon precession frequencies when examining the magentic order in copper pyrazine dinitrate[1].


.. attributes::

.. properties::

References
----------
[1] Lancaster, T., Blundell, S.J., Brooks, M.L., Baker, P.J., Pratt, F.L., Manson, J.L., Landee, C.P. and Baines, C. (2006) Magnetic order in the quasi-one-dimensional spin-1/2 molecular chain compound in copper pyrazine dinitrate, Phys. Rev. B, Vol 73 Issue 2, 020410 `doi: 10.1103/PhysRevB.73.020410 <https://doi.org/10.1103/PhysRevB.73.020410>`_.


.. categories::

.. sourcelink::
