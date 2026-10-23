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
``OutputType="Detector"``, or a WorkspaceGroup of such workspaces, such as the output of
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` with ``OutputType="Detector"`` for several scans. With this
output type, :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` always returns several scans as a WorkspaceGroup.
Each member of a WorkspaceGroup is processed with its own flux logs.
``InputBackground`` has the same pixel dimensions and a single goniometer angle.
The sample and the background must come from the same instrument.

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
the sample is made; for a WorkspaceGroup, each member is modified in place. Otherwise ``OutputWorkspace`` is a new
workspace and ``InputWorkspace`` is left unchanged; for a WorkspaceGroup, ``OutputWorkspace`` is a new WorkspaceGroup
whose members are named ``<OutputWorkspace>_<input member name>``, as in
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>`.

The optional ``OutputBackgroundWorkspace`` holds the background scaled to every goniometer angle of the sample,
:math:`r(\theta)\,B`, with errors :math:`r(\theta)\,\sigma_B`, and the dimensions and sample logs of the sample. For a
WorkspaceGroup sample, it is a WorkspaceGroup whose members are named ``<OutputBackgroundWorkspace>_<input member name>``.
It is created only when a name is given.

A sample of several scans can occupy a lot of memory. Doing the subtraction in place and leaving
``OutputBackgroundWorkspace`` empty avoids creating any workspace of the size of the sample. The members of a
WorkspaceGroup are processed one after the other, and the background is subtracted one goniometer angle at a time, so
the additional memory used is that of one copy of the signal of one member.

.. categories::

.. sourcelink::
