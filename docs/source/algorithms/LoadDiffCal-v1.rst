
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

.. categories::

.. sourcelink::
