.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes in DEMAND detector scan data from files or workspaces and converts them to Q-space, providing an optional
adjustment of the detector positions. `DetectorHeightOffset` adjusts all banks along the y-axis, and
`DetectorDistanceOffset` adjusts banks along the z-axis. Both parameters move the detector relative to the current
detector position.

The output can be either an :ref:`MDEventWorkspace <MDWorkspace>` or a
:ref:`MDHistoWorkspace <MDHistoWorkspace>`. For a `MDHistoWorkspace,`
the conversion to Q-space is done using :ref:`ConvertWANDSCDtoQ
<algm-ConvertWANDSCDtoQ>` while :ref:`ConvertHFIRSCDtoMDE
<algm-ConvertHFIRSCDtoMDE>` is used for outputting an
`MDEventWorkspace.`

If multiple data file are included then the output will be a
:ref:`WorkspaceGroup <WorkspaceGroup>` containing all the
:ref:`MDWorkspaces <MDWorkspace>`.

Normalisation
-------------

During loading a few different types of normalisation can be applied,
vanadium, time or monitor and motor step size. `VanadiumFile` or
`VanadiumWorkspace` is applied by dividing the data detector
pixel-by-pixel. The `NormaliseBy` option `time` or `monitor` uses that
log values from the data (and vanadium if used) input and is applied
to each scan axis step. The `ScaleByMotorStep` scales the entire data
by the step size of either the `omega` or `chi` axis, this is only
using when converting to Q-sample and allows the comparison of peak
intensities found with :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`
to be directly compared between scans measured with different step
sizes.

Grouping
--------

A grouping option is available group pixels by either 2x2 or 4x4 which reduces memory
usage and improves the performance of subsequent reduction steps. The default is the original detetor pixelation.

See :ref:`HB3AIntegratePeaks <algm-HB3AIntegratePeaks>` for complete examples of the HB3A workflow.

.. categories::

.. sourcelink::
