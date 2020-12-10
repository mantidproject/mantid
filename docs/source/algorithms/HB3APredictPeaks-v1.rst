.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

PredictPeaks peaks for the given `MDWorkspace` either using the UB
matrix on the input workspace or the UB matrix from a provided
`PeaksWorkspace`.

The input to this algorithm is intended to be DEMAND data that has
been processed from :ref:`HB3AAdjustSampleNorm
<algm-HB3AAdjustSampleNorm>`. It correctly takes into account the
wavelength and goniometer rotations for the given workspace.

See :ref:`HB3AIntegratePeaks <algm-HB3AIntegratePeaks>` for complete
examples of the HB3A workflow.

.. categories::

.. sourcelink::
