.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm subtracts a background from WAND or DEMAND single crystal data in detector space. The background is
measured at a single goniometer angle and is scaled to the integrated flux of each goniometer angle of the sample
before it is subtracted. The result, ``Sample - Background``, can then be converted to Q-sample with
:ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE>` and normalized with :ref:`MDNorm <algm-MDNorm>`.

``InputWorkspace`` is an MDHistoWorkspace with dimensions (y, x, goniometer angle), as returned by
:ref:`LoadWANDSCD <algm-LoadWANDSCD>` or by :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` with
``OutputType="Detector"``. ``InputBackground`` has the same pixel dimensions and a single goniometer angle. Both must
come from the same instrument.

The sample must not be normalized, because the background is in counts. The algorithm does not check this; load the
sample with ``NormalizeData=False`` in :ref:`LoadWANDSCD <algm-LoadWANDSCD>` or
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>`.

Scaling and subtraction
#######################

For each pixel and each goniometer angle :math:`\theta` of the sample, the output is

.. math::

   O(\theta) = S(\theta) - r(\theta)\,B, \qquad r(\theta) = \frac{F_S(\theta)}{F_B}

where :math:`S` is the sample, :math:`B` the background, and :math:`F_S(\theta)` and :math:`F_B` the integrated flux
of goniometer angle :math:`\theta` of the sample and of the background. ``NormalizedBy`` selects the flux:

- ``Monitor``: the monitor counts, from the ``monitor_count`` log for WAND and the ``monitor`` log for DEMAND.
- ``Time``: the duration, from the ``duration`` log for WAND and the ``time`` log for DEMAND.
- ``None``: no scaling, :math:`r(\theta) = 1`.

The uncertainties are propagated as for any difference of independent measurements:

.. math::

   \sigma_O^2(\theta) = \sigma_S^2(\theta) + r(\theta)^2\,\sigma_B^2

Negative values
###############

Where the sample contains only background, the subtraction can give a negative signal. When
``NullifyNegativeValues`` is ``True`` (the default), the signal of such bins is set to zero and their errors keep the
propagated value :math:`\sigma_O`: the sample and the scaled background are two independent measurements of the same
background, and :math:`\sigma_O` is the statistical uncertainty of their difference.

Output
######

When ``OutputWorkspace`` is the same workspace as ``InputWorkspace``, the subtraction is done in place and no copy of
the sample is made. Otherwise ``OutputWorkspace`` is a new workspace and ``InputWorkspace`` is left unchanged.

.. categories::

.. sourcelink::
