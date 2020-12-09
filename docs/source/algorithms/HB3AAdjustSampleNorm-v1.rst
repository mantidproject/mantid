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

The output can be either an `MDEventWorkspace` or a `MDHistoWorkspace.` For a `MDHistoWorkspace,` the conversion to
Q-space is done using :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` while
:ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE>` is used for outputting an `MDEventWorkspace.`

.. categories::

.. sourcelink::
