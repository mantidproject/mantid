.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes in DEMAND detector scan data and converts it to Q-space, providing an optional adjustment of the detector
positions. `DetectorHeightOffset` adjusts all banks along the y-axis, and `DetectorDistanceOffset` adjusts banks
along the z-axis. Both parameters move the detector relative to the current detector position.

The conversion to Q-space is done using :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>`.

.. categories::

.. sourcelink::
