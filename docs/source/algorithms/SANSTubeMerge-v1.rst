.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm merges separate ISIS Sans2d front and rear detector calibration files that have previously been created from :ref:`algm-SANSTubeCalibration`.
A single workspace is created with an empty instrument containing the calibrated positions of both sets of detector pixels.
The final workspace will be saved out as a Nexus file to the location specified in the ``OutputFile`` property, if one has been provided.

.. categories::

.. sourcelink::
