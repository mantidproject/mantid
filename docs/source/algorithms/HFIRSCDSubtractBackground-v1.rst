.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm subtracts a background from WAND or DEMAND single crystal data in detector space.
The background is scaled to the integrated flux of each goniometer angle of the sample
before it is subtracted. The result, ``Sample - Background``, can then be converted to Q-sample with
:ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE>` and normalized with :ref:`MDNorm <algm-MDNorm>`.

``InputWorkspace`` is an MDHistoWorkspace with dimensions (y, x, goniometer angle), as returned by
:ref:`LoadWANDSCD <algm-LoadWANDSCD>` or by :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` with
``OutputType="Detector"``, or a WorkspaceGroup of such workspaces, such as the output of
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` with ``OutputType="Detector"`` for several scans. With this
output type, :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` always returns several scans as a WorkspaceGroup.
Each member of a WorkspaceGroup is processed with its own flux logs.
The sample and the background must come from the same instrument.

Background
##########

``InputBackground`` is in counts and takes one of two forms:

- An MDHistoWorkspace with dimensions (y, x, goniometer angle) and a single goniometer angle, such as a background run
  loaded with :ref:`LoadWANDSCD <algm-LoadWANDSCD>` or :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>`.
  Without ``InputGroupingWorkspace``, it has the pixel dimensions of the sample.
- A Workspace2D with one value per spectrum, from the instrument of the sample. Its spectra are matched to the pixels
  of the sample through their detector IDs, using the ``detectorID`` sample log of the sample, which
  :ref:`LoadWANDSCD <algm-LoadWANDSCD>` and :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` write. Without
  ``InputGroupingWorkspace``, it has one spectrum per pixel of the sample. Its flux is read from its own sample logs,
  which hold a single value.

When the sample was loaded with pixel grouping (``Grouping="2x2"`` or ``"4x4"``) and the background was not, provide
the GroupingWorkspace that the loader returns in ``OutputGroupingWorkspace`` as ``InputGroupingWorkspace``. The
intensities and errors squared of the background are then summed within each group before the subtraction. The group
of each pixel or spectrum of the background is that of its detector ID, read from the ``detectorID`` log of an
MDHistoWorkspace background or from the spectra of a Workspace2D background.

For clarity, ``InputGroupingWorkspace`` is to be used only with an ungrouped background. The algorithm rejects a call
that passes both a grouped background and ``InputGroupingWorkspace``, whatever the grouping of the background: the
same as that of the sample, finer, or coarser. When the sample and the background were loaded with the same grouping,
subtract them without ``InputGroupingWorkspace``.

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

Workflow
########

#. Load the sample in counts (``NormalizeData=False``) with :ref:`LoadWANDSCD <algm-LoadWANDSCD>` or
   :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` (``OutputType="Detector"``), with pixel grouping if
   wanted, and keep the GroupingWorkspace of a grouped sample (``OutputGroupingWorkspace``). For WAND, also keep the
   vanadium normalization (``OutputNormalizationWorkspace``).
#. Load the background.
#. Convert ``Sample - Background`` and the normalization to Q-sample with
   :ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE>`, directly for WAND, or through
   :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` with ``OutputType="Q-sample events"`` for DEMAND.
#. Normalize with :ref:`MDNorm <algm-MDNorm>`, giving the converted normalization as
   ``MonoSCDNormalizationWorkspace``.

Usage
-----

The examples read data from the HFIR archive and are not run when the documentation is built.

**Example - WAND, 4x4 grouped sample and ungrouped background**

The background is the mean count rate of each pixel over the first 100 runs of the scan, in counts for the duration
of the first run, with the flux logs of that run. An ungrouped WAND pixel records about 0.01 counts per run, so the
median over the runs, as taken by
:ref:`HFIRGoniometerIndependentBackground <algm-HFIRGoniometerIndependentBackground>`, is zero. Unlike the median, the
mean includes the Bragg peaks that cross a pixel during these runs. The sample is one run every 10 of the same 100 runs.

.. code-block:: python

   import numpy as np
   from mantid.simpleapi import *

   # Background: mean count rate of runs 26640-26739, ungrouped,
   # in counts for the duration of the first run
   runs = LoadWANDSCD(
       IPTS=7776,
       RunNumbers="26640-26739",
       OutputWorkspace="background_runs"
   )
   SliceMDHisto(
       InputWorkspace="background_runs",
       Start="0,0,0",
       End="512,3840,1",
       OutputWorkspace="background_slice"
   )
   # SliceMDHisto shares the sample logs of its input,
   # so the slice is cloned before its flux logs are changed
   background = CloneMDWorkspace(
       InputWorkspace="background_slice",
       OutputWorkspace="background"
   )
   logs = runs.getExperimentInfo(0).run()
   duration = np.asarray(logs.getProperty("duration").value)
   scale = duration[0] / duration.sum()
   counts = runs.getSignalArray().sum(axis=2, keepdims=True)
   errors_squared = runs.getErrorSquaredArray().sum(axis=2, keepdims=True)
   background.setSignalArray(scale * counts)
   background.setErrorSquaredArray(scale**2 * errors_squared)
   run = background.getExperimentInfo(0).mutableRun()
   for log in ("duration", "monitor_count"):
       run.addProperty(log, float(np.asarray(logs.getProperty(log).value)[0]), True)
   DeleteWorkspaces(["background_runs", "background_slice"])

   # Sample in counts, grouped 4x4, with the vanadium normalization and the GroupingWorkspace
   LoadWANDSCD(
       IPTS=7776,
       RunNumbers=",".join(str(run) for run in range(26640, 26740, 10)),
       VanadiumIPTS=7776,
       VanadiumRunNumber=26509,
       NormalizeData=False,
       Grouping="4x4",
       OutputWorkspace="sample",
       OutputNormalizationWorkspace="norm",
       OutputGroupingWorkspace="grouping"
   )

   # Subtract in place; the ungrouped background is grouped 4x4 by InputGroupingWorkspace
   HFIRSCDSubtractBackground(
       InputWorkspace="sample",
       InputBackground="background",
       InputGroupingWorkspace="grouping",
       NormalizedBy="Time",
       OutputWorkspace="sample"
   )

   # Convert to Q-sample and normalize; LoadWANDSCD has set the goniometer of each angle
   for name in ("sample", "norm"):
       ConvertHFIRSCDtoMDE(
           InputWorkspace=name,
           Wavelength=1.488,
           OutputWorkspace=f"{name}_md"
       )
   # Horizontal plane, Q_sample_y between -0.5 and 0.5, over the Q range of these runs
   MDNorm(
       InputWorkspace="sample_md",
       MonoSCDNormalizationWorkspace="norm_md",
       RLU=False,
       Dimension0Binning="0,0.05,5",
       Dimension1Binning="-0.5,0.5",
       Dimension2Binning="-7,0.05,1",
       OutputWorkspace="result",
       OutputDataWorkspace="result_data",
       OutputNormalizationWorkspace="result_norm"
   )

**Example - DEMAND, WorkspaceGroup sample**

Scans 0187 and 0188 of experiment 724, loaded together, are a WorkspaceGroup of two members, each with its own
monitor counts. The background is the mean count rate per monitor count of each pixel over the 6 frames of scan 0188,
which show no Bragg peak, in counts for the monitor of its first frame. As for WAND, a pixel records too few counts per
frame for a median.

.. code-block:: python

   import numpy as np
   from mantid.simpleapi import *

   directory = "/HFIR/HB3A/IPTS-24655/shared/autoreduce"

   # Background: mean count rate per monitor count of scan 0188, ungrouped,
   # in counts for the monitor of its first frame
   frames = HB3AAdjustSampleNorm(
       Filename=f"{directory}/HB3A_exp0724_scan0188.nxs",
       OutputType="Detector",
       NormalizeData=False,
       OutputWorkspace="background_frames"
   )
   SliceMDHisto(
       InputWorkspace="background_frames",
       Start="0,0,0",
       End="1536,512,1",
       OutputWorkspace="background_slice"
   )
   # SliceMDHisto shares the sample logs of its input,
   # so the slice is cloned before its flux logs are changed
   background = CloneMDWorkspace(
       InputWorkspace="background_slice",
       OutputWorkspace="background"
   )
   logs = frames.getExperimentInfo(0).run()
   monitor = np.asarray(logs.getProperty("monitor").value)
   scale = monitor[0] / monitor.sum()
   counts = frames.getSignalArray().sum(axis=2, keepdims=True)
   errors_squared = frames.getErrorSquaredArray().sum(axis=2, keepdims=True)
   background.setSignalArray(scale * counts)
   background.setErrorSquaredArray(scale**2 * errors_squared)
   run = background.getExperimentInfo(0).mutableRun()
   for log in ("time", "monitor"):
       run.addProperty(log, float(np.asarray(logs.getProperty(log).value)[0]), True)
   DeleteWorkspaces(["background_frames", "background_slice"])

   sample_files = ",".join(f"{directory}/HB3A_exp0724_scan{scan:04d}.nxs" for scan in (187, 188))
   HB3AAdjustSampleNorm(
       Filename=sample_files,
       OutputType="Detector",
       NormalizeData=False,
       OutputWorkspace="sample"
   )

   # Subtract in place, member by member
   HFIRSCDSubtractBackground(
       InputWorkspace="sample",
       InputBackground="background",
       NormalizedBy="Monitor",
       OutputWorkspace="sample"
   )

   # Convert to Q-sample, merging the members, with the vanadium normalization
   HB3AAdjustSampleNorm(
       InputWorkspaces=",".join(mtd["sample"].getNames()),
       VanadiumFile="/HFIR/HB3A/IPTS-24612/shared/autoreduce/HB3A_exp0722_scan0220.nxs",
       NormalizeData=False,
       OutputType="Q-sample events",
       MergeInputs=True,
       OutputWorkspace="sample_md",
       OutputNormalizationWorkspace="norm_md"
   )
   # Q_sample_x between -1 and 1, over the Q range of these scans
   MDNorm(
       InputWorkspace="sample_md",
       MonoSCDNormalizationWorkspace="norm_md",
       RLU=False,
       Dimension0Binning="-1,1",
       Dimension1Binning="-4,0.05,1",
       Dimension2Binning="-5,0.05,-2",
       OutputWorkspace="result",
       OutputDataWorkspace="result_data",
       OutputNormalizationWorkspace="result_norm"
   )

.. categories::

.. sourcelink::
