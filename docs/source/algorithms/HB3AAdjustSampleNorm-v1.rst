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

The output can be either an :py:obj:`MDEventWorkspace <mantid.api.IMDWorkspace>` or a
:py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>`. For a `MDHistoWorkspace,`
the conversion to Q-space is done using :ref:`ConvertWANDSCDtoQ
<algm-ConvertWANDSCDtoQ>` while :ref:`ConvertHFIRSCDtoMDE
<algm-ConvertHFIRSCDtoMDE>` is used for outputting an
`MDEventWorkspace.`

If multiple data file are included then the output will be a
:py:obj:`WorkspaceGroup <mantid.api.WorkspaceGroup>` containing all the
:py:obj:`MDWorkspace <mantid.api.IMDWorkspace>`.

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

When ``OutputGroupingWorkspace`` is specified alongside a ``2x2`` or ``4x4`` grouping, the algorithm
produces an additional ``Workspace2D`` with one spectrum per detector. The Y value of each spectrum
holds the 1-indexed group ID that the corresponding detector belongs to. Since HB3A detector IDs
start at 1 and map directly to workspace indices (``workspace_index = det_id - 1``), the group ID
for detector ``d`` can be read as ``grouping_ws.readY(d - 1)[0]``. A ``Workspace2D`` is used instead
of a ``GroupingWorkspace`` for performance reasons: constructing a ``GroupingWorkspace`` from the
HB3A instrument requires building the detector-ID-to-workspace-index map, which takes tens of
seconds for the ~786k detectors across HB3A's three 512×512 panels.

See :ref:`HB3AIntegratePeaks <algm-HB3AIntegratePeaks>` for complete examples of the HB3A workflow.

.. categories::

.. sourcelink::
