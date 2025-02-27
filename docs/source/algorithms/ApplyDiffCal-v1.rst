.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads diffractometer constants (DIFA, DIFC, TZERO) from a calibration data source and
stores them in the instrument parameter map of the `InstrumentWorkspace`. The constants
are used in time of flight diffraction instruments to convert from time of flight to
d spacing and vice versa.

This algorithm either reads the constants from the
`CalibrationWorkspace`, reads them from `CalibrationFile` using :ref:`LoadDiffCal
<algm-LoadDiffCal>`, or uses :ref:`ConvertDiffCal<algm-ConvertDiffCal>` to generate
them from the `OffsetsWorkspace`.

This algorithm allows unit conversions between time of flight and d spacing using
calibrated diffractometer constants to be performed using the
:ref:`ConvertUnits <algm-ConvertUnits>` algorithm. :ref:`ConvertUnits <algm-ConvertUnits>`
reads the constants from the instrument parameter map.

When used together with :ref:`ConvertUnits <algm-ConvertUnits>` this algorithm provides a way of
converting in both directions between time of flight and d spacing.

The values of the diffractometer constants that are stored in the instrument parameter map
can be viewed on the Show Detectors screen of a workspace.

.. categories::

.. sourcelink::
