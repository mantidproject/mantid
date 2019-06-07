.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs azimuthal averaging for a 2D SANS data set by going through
each wavelength bin of each detector pixel, determining its Q-value, and adding its amplitude
:math:`I` to the appropriate Q bin. The algorithm works both for monochromatic and TOF SANS data.

Note that the algorithm performs averaging in 2 steps.
First, for each wavelength slice of the input it performs azimuthal averaging in radial rings.
This results in a stack of I(Q)s, one for each input wavelength.
As a second step, all the I(Q)s corresponding to different wavelengths are averaged again.
Note that for the TOF data, this is mathematically not equivalent to performing averaging with one step; that is averaging everything into a single I(Q) histogram.
The latter is done in :ref:`Q1D <algm-Q1D>` algorithm.
See the :ref:`Rebin <algm-Rebin>` documentation for details about choosing the ``OutputBinning`` parameter.

Usage
-----

This algorithm is used as a part of the `SANSReduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`_ and :ref:`SANSILLIntegration <algm-SANSILLIntegration>`.
However, it can also be used directly, provided that the input data is already corrected for all the effects.

I/O
---

The input to this algorithm must be a histogram with common wavelength bins for each pixel.
The output will be a distribution though.

Additional options
------------------

NPixelDivision
##############

For greater precision, each detector
pixel can be sub-divided in sub-pixels by setting the ``NPixelDivision``
parameters. This will split each pixel to a grid of ``NPixelDivision`` * ``NPixelDivision`` pixels.
``PixelSizeX`` and ``PixelSizeY`` inputs will be used only if pixels are to be split.

ErrorWeighting
##############

Each pixel has a weight of 1 by default, but the weight of
each pixel can be set to :math:`1/\sigma^2 I` by setting the
``ErrorWeighting`` parameter to True. This will effectively transmute the average to a weighted average, where the weight is inversely proportional to the absolute statistical uncertainty on the intensity.
This option will potentially produce very different results, so care must be taken when analysing the data with this option.
In the reduction workflows this option is deprecated.

Wedges
######

For unisotropic scatterer, the *I(Q)* can be calculated for different angular sectors (or wedges).
This can be done in two ways: symmetric wedges (default) or asymmetric wedges.

Symmetric
~~~~~~~~~

The figure below illustrates an example for symmetric wedges. Each wedge in this case represents two back-to-back sectors.
The wedges output group will have two workspaces: one for the red region, one for the blue region.

.. figure:: /images/wedge_symm.png
  :align: center
  :width: 600

Asymmetric
~~~~~~~~~~

An example for asymmetric wedges is shown below. The output will have four workspaces, one per each sector of different color.

.. figure:: /images/wedge_asymm.png
  :align: center
  :width: 600

Masked Bins
###########

Bins masked in the input workspace will not enter the calculation.

AccountForGravity
#################

If enabled, this will correct for the gravity effect by analytical calculation of the drop during the time-of-flight from sample to detector.

.. categories::

.. sourcelink::
