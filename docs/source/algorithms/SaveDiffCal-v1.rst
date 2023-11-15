
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves a :ref:`diffraction calibration workspace <DiffractionCalibrationWorkspace>`, ``MaskWorkspace``, and ``GroupingWorkspace`` to a HDF5 file.

The hierarchy of the HDF5 file is as follows:

| calibration
|   detid
|   dasid (only if present in CalibrationWorkspace)
|   difc
|   difa (only if not all values are zero)
|   tzero (only if not all values are zero)
|   group
|   use ("0" if detector is not successfully calibrated, "1" otherwise)
|   offset (only if present in CalibrationWorkspace)
|   instrument
|     name
|     instrument_source (absolute path to the instrument definition file)

This can be used in an alternate mode without a ``CalibrationWorkspace`` to write out the grouping workspace with the ``difc`` values all set to zero.

.. categories::

.. sourcelink::
