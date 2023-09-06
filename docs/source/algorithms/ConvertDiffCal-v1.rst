
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Generates a :ref:`calibration table <DiffractionCalibrationWorkspace>` based on
d-spacing detector offsets provided in an OffsetsWorkspace. Optionally updates
detector calibrations from an existing calibration file. Each detector
offset in the OffsetsWorkspace is processed as follows:

If the detector is masked, its :math:`DIFC` will not be calculated or updated.

If a PreviousCalibration entry exists for the detector, the detector entry's
:math:`DIFC` will be updated as follows and reflected in the final calibration table:

.. math:: DIFC_{new} = \frac{DIFC_{old}}{1+offset}

If a prior calibration entry does not exist for the detector, calculate the :math:`DIFC`
as follows using geometry of the experiment:

.. math:: DIFC = \frac{1}{1+offset}\frac{2m_N}{h} L_{tot} sin \theta

If a detector is present in the PreviousCalibration table but is not found in the
OffsetsWorkspace, it will be propagated to the output calibration table unchanged.

If `Signed` offset mode is specified, the :math:`DIFC` will be calculated or updated with
the following equations:

Update existing calibration:

.. math:: DIFC = DIFC_{old} \cdot (1+|BinWidth|)^{-offset}

Calculate :math:`DIFC` from geometry of the experiment:

.. math:: DIFC = \frac{m_n}{h} \cdot (L1 + L2) 2 \sin(\theta) \cdot (1+|BinWidth|)^{-offset}

The calculations for signed mode is appropriate for full-pattern cross-correlation with logarithmically binned data

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ConvertDiffCal**

The file needed for this example are not present in our standard usage
data download due to its size. It be downloaded from
`pg3_mantid_det.cal
<https://testdata.mantidproject.org/ftp/external-data/MD5/e2b281817b76eadbc26a0a2617477e97>`_.

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
