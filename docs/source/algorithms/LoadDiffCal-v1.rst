
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates up to three workspaces from a hdf5 file:

* :ref:`diffraction calibration workspace
  <DiffractionCalibrationWorkspace>` contains information on
  converting from time-of-flight to d-spacing

* ``MaskWorkspace`` contains which pixels to use (by deleting the
  unused ones)

* ``GroupingWorkspace`` describes which pixels get added together by
  :ref:`algm-DiffractionFocussing`

Specifying an Instrument
------------------------
If the instrument is supplied via ``InputWorkspace``, ``InstrumentName``, or ``InstrumentFilename``, it will be included in the :obj:`mantid.dataobjects.GroupingWorkspace` and :obj:`mantid.dataobjects.MaskWorkspace`.
This is especially useful for debugging these in the instrument view.

.. categories::

.. sourcelink::
