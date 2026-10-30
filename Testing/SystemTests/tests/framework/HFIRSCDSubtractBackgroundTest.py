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

Scaling by the flux logs is checked on WAND data with many goniometer angles, against the flux ratio computed from the
logs. An ungrouped WAND background, the mean of 100 runs, is also subtracted from a 4x4 sample with many goniometer
angles, through a GroupingWorkspace built from the group ID formula. An ungrouped DEMAND background, the mean of the
frames of a scan without a Bragg peak, is subtracted from the same scan grouped 4x4 and split into a WorkspaceGroup of
two members.
"""

import numpy as np
import systemtesting
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.simpleapi import (
    CloneMDWorkspace,
    CreateWorkspace,
    DeleteWorkspace,
    GroupWorkspaces,
    HB3AAdjustSampleNorm,
    HFIRSCDSubtractBackground,
    LoadEmptyInstrument,
    LoadMD,
    LoadWANDSCD,
    SliceMDHisto,
    mtd,
)

# Sample logs holding the flux of each goniometer angle of WAND data
WAND_FLUX_LOGS = {"Time": "duration", "Monitor": "monitor_count"}
# Sample logs holding the flux of each goniometer angle of DEMAND data
DEMAND_FLUX_LOGS = {"Time": "time", "Monitor": "monitor"}


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


def slice_angles(name, start, stop, output, flux_logs=WAND_FLUX_LOGS):
    """
    MDHistoWorkspace <output> holding goniometer angles start to stop - 1 of the MDHistoWorkspace <name>, with the
    flux logs cut to these angles: a single value for one angle, an array otherwise.

    SliceMDHisto shares the ExperimentInfo of its input, so the slice is cloned before its logs are changed.

    :param name: name of an MDHistoWorkspace with dimensions (y, x, goniometer angle) and the flux logs.
    :param start: index of the first goniometer angle kept.
    :param stop: index after the last goniometer angle kept.
    :param output: name of the new MDHistoWorkspace.
    :param flux_logs: WAND_FLUX_LOGS or DEMAND_FLUX_LOGS.
    """
    data = mtd[name]
    n_y, n_x = data.getDimension(0).getNBins(), data.getDimension(1).getNBins()
    SliceMDHisto(InputWorkspace=name, Start=f"0,0,{start}", End=f"{n_y},{n_x},{stop}", OutputWorkspace="__slice")
    CloneMDWorkspace(InputWorkspace="__slice", OutputWorkspace=output)
    DeleteWorkspace("__slice")
    run = mtd[output].getExperimentInfo(0).mutableRun()
    for log in flux_logs.values():
        values = np.asarray(data.getExperimentInfo(0).run().getProperty(log).value)[start:stop]
        run.addProperty(log, float(values[0]) if len(values) == 1 else values.tolist(), True)


def flux(name, normalized_by, flux_logs=WAND_FLUX_LOGS):
    """
    Flux of each goniometer angle of an MDHistoWorkspace, read from its sample logs.

    :param name: name of an MDHistoWorkspace with the flux logs.
    :param normalized_by: "Time" or "Monitor".
    :param flux_logs: WAND_FLUX_LOGS or DEMAND_FLUX_LOGS.
    :returns: numpy array with one value per goniometer angle.
    """
    return np.atleast_1d(mtd[name].getExperimentInfo(0).run().getProperty(flux_logs[normalized_by]).value).astype(float)


def wand_grouping(pixels, output):
    """
    GroupingWorkspace <output> grouping the WAND pixels in squares of pixels x pixels, as LoadWANDSCD does with
    Grouping="2x2" or "4x4", built from the group ID formula rather than by LoadWANDSCD.

    Workspace index w = y + 512 x, of the pixel in row y of detector column x, holds detector ID w and belongs to group
    (x // pixels) (512 // pixels) + y // pixels + 1.

    :param pixels: number of pixels on each side of a group, 2 or 4.
    :param output: name of the new GroupingWorkspace.
    """
    n_y, n_x = 512, 480 * 8
    grouping = WorkspaceFactory.create("GroupingWorkspace", n_y * n_x, 1, 1)
    index = np.arange(n_y * n_x)
    group_ids = (index // n_y // pixels) * (n_y // pixels) + (index % n_y) // pixels + 1.0
    for i, group_id in enumerate(group_ids):
        grouping.mutableY(i)[0] = group_id
        grouping.getSpectrum(i).setDetectorID(i)
    AnalysisDataService.addOrReplace(output, grouping)


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


class HFIRSCDSubtractBackgroundWANDFluxTest(systemtesting.MantidSystemTest):
    """
    Scaling by the WAND flux logs: one goniometer angle of HB2C_WANDSCD_data.nxs, used as the background, is subtracted
    from the first 100 angles of the same file, and the result is compared with S - r B, where r is the ratio of the
    flux of each angle of the sample to the flux of the background.
    """

    def requiredFiles(self):
        return ["HB2C_WANDSCD_data.nxs"]

    def requiredMemoryMB(self):
        return 11000

    def runTest(self):
        LoadMD("HB2C_WANDSCD_data.nxs", LoadHistory=False, OutputWorkspace="data")
        slice_angles("data", 0, 100, "sample")
        slice_angles("data", 650, 651, "background")
        DeleteWorkspace("data")
        sample, background = mtd["sample"], mtd["background"]
        for normalized_by in WAND_FLUX_LOGS:
            ratio = flux("sample", normalized_by) / flux("background", normalized_by)
            assert np.ptp(ratio) > 0, f"the {normalized_by} flux ratio is the same for every angle"
            expected = sample.getSignalArray() - ratio * background.getSignalArray()
            expected_error_squared = sample.getErrorSquaredArray() + ratio**2 * background.getErrorSquaredArray()
            assert (expected < 0).any(), "no bin is negative after the subtraction"
            for nullify in (False, True):
                output = HFIRSCDSubtractBackground(
                    InputWorkspace="sample",
                    InputBackground="background",
                    NormalizedBy=normalized_by,
                    NullifyNegativeValues=nullify,
                    OutputWorkspace="output",
                )
                np.testing.assert_allclose(output.getSignalArray(), np.maximum(expected, 0) if nullify else expected, atol=1e-9)
                np.testing.assert_allclose(output.getErrorSquaredArray(), expected_error_squared)

    def cleanup(self):
        mtd.clear()


class HFIRSCDSubtractBackgroundWANDMeanBackgroundTest(systemtesting.MantidSystemTest):
    """
    Subtraction of an ungrouped WAND background, grouped 4x4 by InputGroupingWorkspace, from a sample with many
    goniometer angles.

    HB2C_WANDSCD_background.nxs is the mean count rate of each ungrouped pixel over runs 26640-26739 of IPTS-7776, in
    counts for the duration of run 26640, with the flux logs of that run. HB2C_WANDSCD_data.nxs holds the same runs, as
    its first 100 angles, grouped 4x4. The expected grouped background is computed from these 100 angles, independently
    of the background file and of the grouping done by HFIRSCDSubtractBackground. The sample is the next 100 angles,
    runs 26740-26839, which are not part of the background. The GroupingWorkspace is built from the group ID formula, and
    the 'detectorID' log of the sample, which HB2C_WANDSCD_data.nxs lacks, from the formula of LoadWANDSCD.
    """

    def requiredFiles(self):
        return ["HB2C_WANDSCD_data.nxs", "HB2C_WANDSCD_background.nxs"]

    def requiredMemoryMB(self):
        return 11000

    def runTest(self):
        LoadMD("HB2C_WANDSCD_data.nxs", LoadHistory=False, OutputWorkspace="data")
        # Mean count rate of the 4x4 groups over the background runs, the first 100 angles, in counts for the duration
        # of the first run
        duration = flux("data", "Time")[:100]
        scale = duration[0] / duration.sum()
        data = mtd["data"]
        background = scale * data.getSignalArray()[:, :, :100].sum(axis=2, keepdims=True)
        background_error_squared = scale**2 * data.getErrorSquaredArray()[:, :, :100].sum(axis=2, keepdims=True)
        background_flux = {normalized_by: flux("data", normalized_by)[:1] for normalized_by in WAND_FLUX_LOGS}
        del data
        slice_angles("data", 100, 200, "sample")
        DeleteWorkspace("data")
        # HB2C_WANDSCD_data.nxs predates the 'detectorID' log. Add the one LoadWANDSCD writes with Grouping="4x4": the
        # first detector ID of the group of each pixel, the pixels being ordered with the first dimension varying fastest.
        pixel = np.arange(128 * 960)
        detector_ids = 4 * (pixel % 128) + 4 * 512 * (pixel // 128)
        mtd["sample"].getExperimentInfo(0).mutableRun().addProperty("detectorID", detector_ids.tolist(), True)
        LoadMD("HB2C_WANDSCD_background.nxs", LoadHistory=False, OutputWorkspace="background")
        wand_grouping(4, "grouping")
        for normalized_by in WAND_FLUX_LOGS:
            np.testing.assert_allclose(flux("background", normalized_by), background_flux[normalized_by])
        # The subtraction is done in place, to save memory. Keep the sample to restore it for the next normalization.
        sample = mtd["sample"]
        sample_signal = sample.getSignalArray().copy()
        sample_error_squared = sample.getErrorSquaredArray().copy()
        for normalized_by in WAND_FLUX_LOGS:
            ratio = flux("sample", normalized_by) / flux("background", normalized_by)
            expected = sample_signal - ratio * background
            assert (expected < 0).any(), "no bin is negative after the subtraction"
            output = HFIRSCDSubtractBackground(
                InputWorkspace="sample",
                InputBackground="background",
                InputGroupingWorkspace="grouping",
                NormalizedBy=normalized_by,
                NullifyNegativeValues=False,
                OutputWorkspace="sample",
            )
            np.testing.assert_allclose(output.getSignalArray(), expected, atol=1e-9)
            np.testing.assert_allclose(output.getErrorSquaredArray(), sample_error_squared + ratio**2 * background_error_squared)
            output.setSignalArray(sample_signal)
            output.setErrorSquaredArray(sample_error_squared)

    def cleanup(self):
        mtd.clear()


class HFIRSCDSubtractBackgroundDEMANDMeanBackgroundTest(systemtesting.MantidSystemTest):
    """
    Subtraction of an ungrouped DEMAND background, grouped 4x4 by InputGroupingWorkspace, from a WorkspaceGroup sample.

    HB3A_exp0724_scan0182.nxs is a phi scan of 123 goniometer angles at 2theta = 28 deg. The background is the mean
    count rate per monitor count of each ungrouped pixel over the frames without a Bragg peak, those whose count rate
    is below 1.1 times the median count rate of the scan, in counts for the monitor of the first of these frames, with
    the flux logs of that frame. The sample is the scan grouped 4x4 by HB3AAdjustSampleNorm, split into two members,
    angles 0-61 and 62-122, as two scans at the same 2theta would be. The expected grouped background is the sum of
    the background over each 4x4 group, computed independently of the grouping done by HFIRSCDSubtractBackground.
    """

    def requiredFiles(self):
        return ["HB3A_exp0724_scan0182.nxs"]

    def requiredMemoryMB(self):
        return 10000

    def runTest(self):
        LoadMD("HB3A_exp0724_scan0182.nxs", LoadHistory=False, OutputWorkspace="scan0182")
        common = dict(InputWorkspaces="scan0182", OutputType="Detector", NormalizeData=False)
        # The grouped scan first, before the ungrouped one exists, to lower the peak memory
        HB3AAdjustSampleNorm(Grouping="4x4", OutputWorkspace="scan_4x4", OutputGroupingWorkspace="grouping", **common)
        frames = HB3AAdjustSampleNorm(OutputWorkspace="frames", **common)
        DeleteWorkspace("scan0182")
        # Mean count rate per monitor count over the frames without a Bragg peak, in counts for the monitor of the
        # first of these frames
        monitor = flux("frames", "Monitor", DEMAND_FLUX_LOGS)
        rate = frames.getSignalArray().sum(axis=(0, 1)) / monitor
        quiet = np.flatnonzero(rate < 1.1 * np.median(rate))
        first = int(quiet[0])
        slice_angles("frames", first, first + 1, "background", DEMAND_FLUX_LOGS)
        scale = monitor[first] / monitor[quiet].sum()
        background = mtd["background"]
        background.setSignalArray(scale * frames.getSignalArray()[:, :, quiet].sum(axis=2, keepdims=True))
        background.setErrorSquaredArray(scale**2 * frames.getErrorSquaredArray()[:, :, quiet].sum(axis=2, keepdims=True))
        del frames
        DeleteWorkspace("frames")
        # Sum over each 4x4 group, the first two dimensions of the ungrouped pixels being split into groups of 4
        n_0, n_1, _ = background.getSignalArray().shape
        grouped_background, grouped_error_squared = (
            array.reshape(n_0 // 4, 4, n_1 // 4, 4, 1).sum(axis=(1, 3))
            for array in (background.getSignalArray(), background.getErrorSquaredArray())
        )
        slice_angles("scan_4x4", 0, 62, "sample_0", DEMAND_FLUX_LOGS)
        slice_angles("scan_4x4", 62, 123, "sample_1", DEMAND_FLUX_LOGS)
        DeleteWorkspace("scan_4x4")
        GroupWorkspaces(InputWorkspaces="sample_0,sample_1", OutputWorkspace="sample")
        members = mtd["sample"].getNames()
        # The subtraction is done in place, to save memory. Keep the sample to restore it for the next normalization.
        sample_signal = {member: mtd[member].getSignalArray().copy() for member in members}
        sample_error_squared = {member: mtd[member].getErrorSquaredArray().copy() for member in members}
        for normalized_by in DEMAND_FLUX_LOGS:
            ratio = {
                member: flux(member, normalized_by, DEMAND_FLUX_LOGS) / flux("background", normalized_by, DEMAND_FLUX_LOGS)
                for member in members
            }
            expected = {member: sample_signal[member] - ratio[member] * grouped_background for member in members}
            assert all((expected[member] < 0).any() for member in members), "no bin is negative after the subtraction"
            HFIRSCDSubtractBackground(
                InputWorkspace="sample",
                InputBackground="background",
                InputGroupingWorkspace="grouping",
                NormalizedBy=normalized_by,
                NullifyNegativeValues=False,
                OutputWorkspace="sample",
            )
            for member in members:
                output = mtd[member]
                np.testing.assert_allclose(output.getSignalArray(), expected[member], atol=1e-9)
                np.testing.assert_allclose(
                    output.getErrorSquaredArray(), sample_error_squared[member] + ratio[member] ** 2 * grouped_error_squared
                )
                output.setSignalArray(sample_signal[member])
                output.setErrorSquaredArray(sample_error_squared[member])

    def cleanup(self):
        mtd.clear()
