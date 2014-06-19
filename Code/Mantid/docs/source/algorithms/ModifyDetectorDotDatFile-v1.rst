.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Modifies an ISIS detector dot data file, so that the detector positions
are as in the given workspace. This algorithm can be used to transfer a
calibration done via the :ref:`algm-ApplyCalibration`
algorithm to an ISIS detector dot dat file by selecting a workspace that
has been modified by ApplyCalibration.

A typical ISIS dot data file has a format like this:

| ``DETECTOR.DAT generated by CREATE_DETECTOR_FILE``
| `` 286729      14``
| `` det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x       ...       ``
| ``     11   0.000  -3.25800     1   180.00000     0.00000     0.00000     0.00000     0.00000     0.00000    ... ``
| ``     21   0.000  -1.50400     1   180.00000     0.00000     0.00000     0.00000     0.00000     0.00000    ...``
| ``   ....``
| ``1110001   5.300   2.88936     3    52.28653  -140.67224     0.02540     0.02540     0.00283     0.02750   ... ``
| ``1110002   5.300   2.88794     3    52.26477  -140.72720     0.02540     0.02540     0.00283     0.02750   ...``
| ``   ....``

Each row corresponds to a detector whose type is indicated in the
``code`` column. The algorithm will only modify values in colums ``l2``,
``theta`` and ``phi`` and only if the value in the ``code`` column is 3,
which indicates a PSD gas tube. For more details about the detector dot
data file see
`LoadDetectorInfo#File\_format <LoadDetectorInfo#File_format>`__.

.. categories::
