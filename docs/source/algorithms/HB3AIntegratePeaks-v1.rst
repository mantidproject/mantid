.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses a :ref:`MDEventWorkspace <MDWorkspace>` and `PeaksWorkspace` to integrate the peaks provided in the peaks
workspace. A :ref:`LorentzCorrection <algm-LorentzCorrection>` can be optionally applied to the integrated peaks with
the `ApplyLorentz` option.

The output peaks workspace can be written to `OutputFile` in either SHELX format with direction cosines using
:ref:`SaveHKL <algm-SaveHKL>` or in the Fullprof format using :ref:`SaveReflections <algm-SaveReflections>`.

The input to this algorithm is intended as part of the DEMAND data reduction workflow, using
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` and :ref:`HB3AFindPeaks <algm-HB3AFindPeaks>` which can be seen
below in the example usage.

Usage
-----

**Example - DEMAND Workflow**

.. code-block:: python

    # Input detector scan data to use. Can be a list of files, or workspaces - see HB3AAdjustSampleNorm for details
    data = "HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs"

    # Converts to MDEventWorkspace and merges multiple files into one workspace.
    #  If needed, the sample position can be adjusted by a height and distance - see the HB3AAdjustSampleNorm docs
    merged = HB3AAdjustSampleNorm(Filename=data, MergeInputs=True)

    # Finds peaks on the combined workspace, optimizing the UB matrix for the given cell type parameters
    #  lattice parameters can be optionally specified to calculate the UB matrix (see docs for HB3AFindPeaks)
    peaks = HB3AFindPeaks(InputWorkspace=merged,
                          CellType="Orthorhombic",
                          Centering="F")

    # Integrate the peaks from the optimized peaks workspace above
    integrated_peaks = HB3AIntegratePeaks(InputWorkspace=merged, PeaksWorkspace=peaks,
                                          PeakRadius=0.25,
                                          OutputFile="./integrated_peaks.hkl")

.. categories::

.. sourcelink::
