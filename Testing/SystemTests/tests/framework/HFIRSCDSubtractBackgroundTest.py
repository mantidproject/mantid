# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
System tests of HFIRSCDSubtractBackground on data loaded with LoadWANDSCD and HB3AAdjustSampleNorm.

Grouping a background through HFIRSCDSubtractBackground must reproduce the grouping done by the loader. The sample is a
file loaded with 4x4 grouping, and the background is the same file loaded without grouping, as an MDHistoWorkspace or
as a Workspace2D. Both files have a single goniometer angle. With NormalizedBy=None, the scaled background must then
equal the grouped sample, and the subtraction must give zero, with errors squared doubled.
"""

import numpy as np
import systemtesting
from mantid.simpleapi import (
    CreateWorkspace,
    DeleteWorkspace,
    HB3AAdjustSampleNorm,
    HFIRSCDSubtractBackground,
    LoadEmptyInstrument,
    LoadWANDSCD,
    mtd,
)


def create_workspace2d(name):
    """
    Workspace2D <name>_2d holding, in the spectrum of each detector, the counts of that detector in the ungrouped
    single-angle MDHistoWorkspace <name>.

    The pixel of each detector is found through the 'detectorID' log of <name>, whose entry k is the detector ID of
    pixel k, the pixels being ordered with the first dimension varying fastest.

    :param name: name of an ungrouped MDHistoWorkspace with one goniometer angle, loaded with LoadWANDSCD or
        HB3AAdjustSampleNorm.
    """
    data = mtd[name]
    detector_ids = np.asarray(data.getExperimentInfo(0).run().getProperty("detectorID").value)
    instrument = LoadEmptyInstrument(InstrumentName=data.getExperimentInfo(0).getInstrumentName(), OutputWorkspace="__instrument")
    n_spectra = instrument.getNumberHistograms()
    # LoadEmptyInstrument gives detector ID d to spectrum d for WAND and to spectrum d - 1 for DEMAND
    indices = detector_ids - instrument.getSpectrum(0).getDetectorIDs()[0]
    signal, error_squared = np.zeros(n_spectra), np.zeros(n_spectra)
    signal[indices] = data.getSignalArray()[:, :, 0].ravel(order="F")
    error_squared[indices] = data.getErrorSquaredArray()[:, :, 0].ravel(order="F")
    CreateWorkspace(
        DataX=np.tile([0.0, 1.0], n_spectra),
        DataY=signal,
        DataE=np.sqrt(error_squared),
        NSpec=n_spectra,
        ParentWorkspace=instrument,
        OutputWorkspace=f"{name}_2d",
    )
    DeleteWorkspace(instrument)


def assert_reproduces(sample, background, grouping=None):
    """
    Subtract background from sample with NormalizedBy=None, and check that the scaled background equals the sample
    and that the subtraction gives zero, with the errors squared of the sample doubled.

    :param sample: name of the sample, which has the pixel layout expected for the background after grouping.
    :param background: name of the background, an MDHistoWorkspace or a Workspace2D.
    :param grouping: name of the GroupingWorkspace, or None.
    :raises AssertionError: if a check fails.
    """
    kwargs = {"InputGroupingWorkspace": grouping} if grouping else {}
    output, scaled_background = HFIRSCDSubtractBackground(
        InputWorkspace=sample,
        InputBackground=background,
        NullifyNegativeValues=False,
        OutputWorkspace="output",
        OutputBackgroundWorkspace="scaled_background",
        **kwargs,
    )
    expected = mtd[sample]
    assert expected.getSignalArray().sum() > 0, f"{sample} has no counts"
    np.testing.assert_allclose(scaled_background.getSignalArray(), expected.getSignalArray())
    np.testing.assert_allclose(scaled_background.getErrorSquaredArray(), expected.getErrorSquaredArray())
    np.testing.assert_allclose(output.getSignalArray(), 0.0, atol=1e-9)
    np.testing.assert_allclose(output.getErrorSquaredArray(), 2 * expected.getErrorSquaredArray())


class HFIRSCDSubtractBackgroundWANDGroupingTest(systemtesting.MantidSystemTest):
    """Grouping of WAND backgrounds, MDHistoWorkspace and Workspace2D, reproduces the 4x4 grouping of LoadWANDSCD."""

    def requiredFiles(self):
        return ["HB2C_7000.nxs.h5"]

    def runTest(self):
        LoadWANDSCD("HB2C_7000.nxs.h5", OutputWorkspace="wand")
        LoadWANDSCD("HB2C_7000.nxs.h5", Grouping="4x4", OutputWorkspace="wand_4x4", OutputGroupingWorkspace="wand_grouping")
        create_workspace2d("wand")
        assert_reproduces("wand_4x4", "wand", "wand_grouping")
        assert_reproduces("wand_4x4", "wand_2d", "wand_grouping")
        assert_reproduces("wand", "wand_2d")

    def cleanup(self):
        mtd.clear()


class HFIRSCDSubtractBackgroundDEMANDGroupingTest(systemtesting.MantidSystemTest):
    """
    Grouping of DEMAND backgrounds, MDHistoWorkspace and Workspace2D, reproduces the 4x4 grouping of
    HB3AAdjustSampleNorm.
    """

    def requiredFiles(self):
        return ["HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        common = dict(Filename="HB3A_exp0722_scan0220.nxs", OutputType="Detector", NormalizeData=False)
        HB3AAdjustSampleNorm(OutputWorkspace="demand", **common)
        HB3AAdjustSampleNorm(Grouping="4x4", OutputWorkspace="demand_4x4", OutputGroupingWorkspace="demand_grouping", **common)
        create_workspace2d("demand")
        assert_reproduces("demand_4x4", "demand", "demand_grouping")
        assert_reproduces("demand_4x4", "demand_2d", "demand_grouping")
        assert_reproduces("demand", "demand_2d")

    def cleanup(self):
        mtd.clear()
