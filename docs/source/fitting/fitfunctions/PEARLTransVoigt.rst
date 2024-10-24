.. _func-PEARLTransVoigt:

===============
PEARLTransVoigt
===============

.. index:: PEARLTransVoigt

Description
-----------

The Voigt profile is used in spectroscopy when a given line shape is neither a Gaussian nor a Lorentzian but rather is a
convolution of the two. This function is described in Igor Technical Note 26 [1]_ and is based on a computational method
by Humlícek [2]_. Its relative accuracy is better than 0.0001 and most of the time is much better. This function, as
coded, is currently only to be used on PEARL data in the Transfit algorithm.

.. attributes::

.. properties::

References
----------

.. [1] Hutchinson, L. `(1991) IGOR Technical Note 26 <https://wavemetrics.net/Downloads/FTP_Archive/IgorPro/Technical_Notes/index.html>`__
.. [2] Humlícek, J. `(1982) Optimized Computation of the Voigt and Complex Probability Functions. Journal of Quantitative Spectroscopy and Radiative Transfer, 27, 437-444. <http://dx.doi.org/10.1016/0022-4073(82)90078-4>`__
.. categories::

.. sourcelink::
