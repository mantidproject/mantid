
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm converts diffraction OffsetsWorkspaces to
:ref:`calibration table <DiffractionCalibrationWorkspace>`. It uses
the geometry of the OffsetsWorkspace to based the values of
:math:`DIFC` on. The effective :math:`DIFC` is calculated as

.. math:: DIFC = \frac{1}{1+offset}\frac{2m_N}{h} L_{tot} sin \theta

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ConvertDiffCal**

The file needed for this example are not present in our standard usage
data download due to its size. It be downloaded from
`pg3_mantid_det.cal
<http://198.74.56.37/ftp/external-data/MD5/e2b281817b76eadbc26a0a2617477e97>`_.

.. testcode:: ConvertDiffCalExample

   LoadCalFile(InstrumentName="PG3",
               CalFilename="pg3_mantid_det.cal",
               MakeGroupingWorkspace=False,
               MakeOffsetsWorkspace=True,
               MakeMaskWorkspace=False,
               WorkspaceName="PG3")
   PG3_cal=ConvertDiffCal(OffsetsWorkspace="PG3_offsets",
                          OutputWorkspace="PG3_cal")

.. categories::

.. sourcelink::
