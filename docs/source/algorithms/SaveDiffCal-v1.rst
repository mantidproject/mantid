
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

| calibration
|   detid
|   dasid (only if present in CalibrationWorkspace)
|   difa
|   tzero
|   group
|   use ("0" if detector is not successfully calibrated, "1" otherwise)
|   offset (only if present in CalibrationWorkspace)
|   instrument
|     name
|     instrument_source (absolute path to the instrument definition file)

.. categories::

.. sourcelink::
