
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves a :ref:`diffraction calibration workspace
<DiffractionCalibrationWorkspace>`, ``MaskWorkspace``, and
``GroupingWorkspace`` to a HDF5 file.

The hierarchy of the HDF5 file is as follows:

calibration
+__ _detid
+__ _dasid (only if present in CalibrationWorkspace)
+__ _difc
+__ _difa
+__ _tzero
+__ _group
+__ _use ("0" if detector is not successfully calibrated, "1" otherwise)
+__ _offset (only if present in CalibrationWorkspace)
+__ _instrument
    +-- name
    +-- instrument_source (absolute path to the instrument definition file)

.. categories::

.. sourcelink::
