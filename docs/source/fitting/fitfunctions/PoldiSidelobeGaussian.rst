.. _func-PoldiSidelobeGaussian:

=====================
PoldiSidelobeGaussian
=====================

.. index:: PoldiSidelobeGaussian

Description
-----------

A Gaussian core minus a broader, shallower Gaussian sidelobe, used to fit the 1D autocorrelation
spectrum produced by :ref:`algm-PoldiAutoCorrelation`.  The impulse response of the POLDI
correlation method is not a plain Gaussian: as described on that algorithm's page, each Bragg peak
is surrounded by a dip whose depth is proportional to the peak height and whose width is set by the
chopper speed.  Fitting such a peak with a plain Gaussian biases the refined width, and hence the
intensity, low.  :ref:`algm-PoldiFitPeaks1D` accounts for the same dip by adding a quadratic
background under each peak; this function instead builds it into the peak shape, so that a single
sidelobe parameter is shared by every reflection in a Pawley refinement.

The function is defined as

.. math:: \frac{I}{\sigma\sqrt{2\pi}}\left[\exp\left(-\frac{(x-x_0)^2}{2\sigma^2}\right) - f\exp\left(-\frac{(x-x_0)^2}{2\sigma_2^2}\right)\right]

where:

- :math:`I` is the area of the Gaussian **core** (parameter name ``Intensity``)
- :math:`x_0` is the peak centre (parameter name ``Centre``)
- :math:`\sigma` is the standard deviation of the core (parameter name ``Sigma``)
- :math:`f` is the sidelobe depth as a fraction of the core amplitude (parameter name ``SidelobeFraction``)
- :math:`\sigma_2` is the standard deviation of the sidelobe (parameter name ``SidelobeSigma``)

Setting :math:`f = 0` reduces the function exactly to a :ref:`func-Gaussian`.

Notes on the intensity and width
################################

``Intensity`` is the area of the core alone, not the integral of the whole function.  For typical
sidelobe parameters :math:`f\sigma_2/\sigma > 1`, so the net integral is negative; the core area is
the quantity of interest for downstream analysis.  The framework's ``intensity()`` method
integrates the function numerically and so returns that negative net integral - read the
``Intensity`` parameter directly instead.

Similarly ``fwhm()`` returns the FWHM of the core; the sidelobe widens the apparent FWHM slightly.

Usage with POLDI
################

:math:`\sigma_2` is an instrument quantity, inversely proportional to the chopper speed, and is
normally held fixed.  ``PoldiSidelobeProfile`` in ``Engineering.pawley_utils`` computes it from the
chopper speed and the ``sidelobe_width_coeff`` instrument parameter, and refines :math:`f`, which
varies with the number of detector groups used in the autocorrelation and with the background.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
