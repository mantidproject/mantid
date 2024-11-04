# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
from mantid.kernel import logger, Direction, IntArrayProperty, StringArrayProperty
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, FitPeaks, LoadEventNexus, LRScalingFactors
import functools
import numpy as np
from typing import List, Tuple
import datetime
from math import ceil

THI_TOLERANCE = 0.002


class CompareTwoNXSDataForSFcalculator(object):
    """
    will return -1, 0 or 1 according to the position of the nexusToPosition in relation to the
    nexusToCompareWith based on the following criteria
    #1: number of attenuators (ascending order)
    #2: lambda requested (descending order)
    #3: S2W (ascending order)
    #4: S2H (descending order)
    #5 if everything up to this point is identical, return 0
    """

    nexusToCompareWithRun = None
    nexusToPositionRun = None
    resultComparison = 0

    def __init__(self, nxsdataToCompareWith, nxsdataToPosition):
        self.nexusToCompareWithRun = nxsdataToCompareWith.getRun()
        self.nexusToPositionRun = nxsdataToPosition.getRun()

        compare = self.compareParameter("LambdaRequest", "descending")
        if compare != 0:
            self.resultComparison = compare
            return

        compare = self.compareParameter("thi", "descending", tolerance=THI_TOLERANCE)
        if compare != 0:
            self.resultComparison = compare
            return

        compare = self.compareParameter("vAtt", "ascending")
        if compare != 0:
            self.resultComparison = compare
            return

        pcharge1 = self.nexusToCompareWithRun.getProperty("gd_prtn_chrg").value / nxsdataToCompareWith.getNEvents()
        pcharge2 = self.nexusToPositionRun.getProperty("gd_prtn_chrg").value / nxsdataToPosition.getNEvents()

        self.resultComparison = -1 if pcharge1 < pcharge2 else 1

    def compareParameter(self, param, order, tolerance=0.0):
        """
        Compare parameters for the two runs
        :param string param: name of the parameter to compare
        :param string order: ascending or descending
        :param float tolerance: tolerance to apply to the comparison [optional]
        """
        _nexusToCompareWithRun = self.nexusToCompareWithRun
        _nexusToPositionRun = self.nexusToPositionRun

        _paramNexusToCompareWith = float(_nexusToCompareWithRun.getProperty(param).value[0])
        _paramNexusToPosition = float(_nexusToPositionRun.getProperty(param).value[0])

        if abs(_paramNexusToPosition - _paramNexusToCompareWith) <= tolerance:
            return 0

        if order == "ascending":
            resultLessThan = -1
            resultMoreThan = 1
        else:
            resultLessThan = 1
            resultMoreThan = -1

        if _paramNexusToPosition < _paramNexusToCompareWith:
            return resultLessThan
        elif _paramNexusToPosition > _paramNexusToCompareWith:
            return resultMoreThan
        else:
            return 0

    def result(self):
        return self.resultComparison


def sorter_function(r1, r2):
    """
    Sorter function used by with the 'sorted' call to sort the direct beams.
    """
    return CompareTwoNXSDataForSFcalculator(r2, r1).result()


class LRDirectBeamSort(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRDirectBeamSort"

    def version(self):
        return 1

    def summary(self):
        return "Sort a set of direct beams for the purpose of calculating scaling factors."

    def PyInit(self):
        self.declareProperty(
            IntArrayProperty("RunList", [], direction=Direction.Input),
            "List of run numbers (integers) to be sorted - takes precedence over WorkspaceList",
        )
        self.declareProperty(StringArrayProperty("WorkspaceList", [], direction=Direction.Input), "List of workspace names to be sorted")
        self.declareProperty(
            "UseLowResCut", False, direction=Direction.Input, doc="If True, an x-direction cut will be determined and used"
        )
        self.declareProperty("ComputeScalingFactors", True, direction=Direction.Input, doc="If True, the scaling factors will be computed")
        self.declareProperty("TOFSteps", 200.0, doc="TOF bin width")
        self.declareProperty("WavelengthOffset", 0.0, doc="Wavelength offset used for TOF range determination")
        self.declareProperty("IncidentMedium", "Air", doc="Name of the incident medium")
        self.declareProperty("OrderDirectBeamsByRunNumber", False, "Force the sequence of direct beam files to be ordered by run number")
        self.declareProperty(
            FileProperty("ScalingFactorFile", "", action=FileAction.OptionalSave, extensions=["cfg"]), "Scaling factor file to be created"
        )
        self.declareProperty(IntArrayProperty("OrderedRunList", [], direction=Direction.Output), "Ordered list of run numbers")
        self.declareProperty(
            StringArrayProperty("OrderedNameList", [], direction=Direction.Output),
            "Ordered list of workspace names corresponding to the run list",
        )
        self.declareProperty("SlitTolerance", 0.02, doc="Tolerance for matching slit positions")

    def PyExec(self):
        compute = self.getProperty("ComputeScalingFactors").value
        lr_data = []
        run_list = self.getProperty("RunList").value
        if len(run_list) > 0:
            for run in run_list:
                workspace = LoadEventNexus(Filename="REF_L_%s" % run, OutputWorkspace="__data_file_%s" % run, MetaDataOnly=not compute)
                lr_data.append(workspace)
        else:
            ws_list = self.getProperty("WorkspaceList").value
            for ws in ws_list:
                lr_data.append(mtd[ws])

        sort_by_runs = self.getProperty("OrderDirectBeamsByRunNumber").value
        if sort_by_runs is True:
            lr_data_sorted = sorted(lr_data, key=lambda r: r.getRunNumber())
        else:
            lr_data_sorted = sorted(lr_data, key=functools.cmp_to_key(sorter_function))

        # Set the output properties
        run_numbers = [r.getRunNumber() for r in lr_data_sorted]
        ws_names = [str(r) for r in lr_data_sorted]
        self.setProperty("OrderedRunList", run_numbers)
        self.setProperty("OrderedNameList", ws_names)

        # Compute the scaling factors if requested
        if compute:
            sf_file = self.getProperty("ScalingFactorFile").value
            if len(sf_file) == 0:
                logger.error("Scaling factors were requested but no output file was set")
            else:
                self._compute_scaling_factors(lr_data_sorted)

    def _compute_scaling_factors(self, lr_data_sorted):
        """
        If we need to compute the scaling factors, group the runs by their wavelength request
        @param lr_data_sorted: ordered list of workspaces
        """
        group_list = []
        current_group = []
        _current_wl = None
        _current_thi = None
        for r in lr_data_sorted:
            wl_ = r.getRun().getProperty("LambdaRequest").value[0]
            thi = r.getRun().getProperty("thi").value[0]

            if _current_thi is None or abs(thi - _current_thi) > THI_TOLERANCE or not _current_wl == wl_:
                # New group
                _current_wl = wl_
                _current_thi = thi
                if len(current_group) > 0:
                    group_list.append(current_group)
                current_group = []

            current_group.append(r)

        # Add in the last group
        group_list.append(current_group)

        tof_steps = self.getProperty("TOFSteps").value
        scaling_file = self.getProperty("ScalingFactorFile").value
        # use_low_res_cut = self.getProperty("UseLowResCut").value
        incident_medium = self.getProperty("IncidentMedium").value
        summary = ""
        for g in group_list:
            if len(g) == 0:
                continue

            direct_beam_runs = []
            peak_ranges = []
            x_ranges = []
            bck_ranges = []

            for run in g:
                peak, low_res = self._find_peak(run)  # , use_low_res_cut)

                att = run.getRun().getProperty("vAtt").value[0] - 1
                wl = run.getRun().getProperty("LambdaRequest").value[0]
                thi = run.getRun().getProperty("thi").value[0]
                direct_beam_runs.append(run.getRunNumber())
                peak_ranges.append(int(peak[0]))
                peak_ranges.append(int(peak[1]))
                x_ranges.append(int(low_res[0]))
                x_ranges.append(int(low_res[1]))
                bck_ranges.append(int(peak[0]) - 3)
                bck_ranges.append(int(peak[1]) + 3)

                summary += "%10s wl=%5s thi=%5s att=%s %5s,%5s %5s,%5s\n" % (
                    run.getRunNumber(),
                    wl,
                    thi,
                    att,
                    peak[0],
                    peak[1],
                    low_res[0],
                    low_res[1],
                )

            # Determine TOF range from first file
            sample = g[0].getInstrument().getSample()
            source = g[0].getInstrument().getSource()
            source_sample_distance = sample.getDistance(source)
            detector = g[0].getDetector(0)
            sample_detector_distance = detector.getPos().getZ()
            source_detector_distance = source_sample_distance + sample_detector_distance
            h = 6.626e-34  # m^2 kg s^-1
            m = 1.675e-27  # kg
            wl = g[0].getRun().getProperty("LambdaRequest").value[0]
            chopper_speed = g[0].getRun().getProperty("SpeedRequest1").value[0]
            wl_offset = self.getProperty("WavelengthOffset").value
            tof_min = source_detector_distance / h * m * (wl + wl_offset * 60.0 / chopper_speed - 1.7 * 60.0 / chopper_speed) * 1e-4
            tof_max = source_detector_distance / h * m * (wl + wl_offset * 60.0 / chopper_speed + 1.7 * 60.0 / chopper_speed) * 1e-4
            tof_range = [tof_min, tof_max]

            summary += "      TOF: %s\n\n" % tof_range

            # Compute the scaling factors
            logger.notice("Computing scaling factors for %s" % str(direct_beam_runs))
            slit_tolerance = self.getProperty("SlitTolerance").value
            LRScalingFactors(
                DirectBeamRuns=direct_beam_runs,
                TOFRange=tof_range,
                TOFSteps=tof_steps,
                SignalPeakPixelRange=peak_ranges,
                SignalBackgroundPixelRange=bck_ranges,
                LowResolutionPixelRange=x_ranges,
                IncidentMedium=incident_medium,
                SlitTolerance=slit_tolerance,
                ScalingFactorFile=scaling_file,
            )

        # log output summary
        logger.notice(summary)

    @staticmethod
    def _find_peak(ws, crop=25, factor=1.0) -> Tuple[List[int], List[int]]:
        """Find peak by Mantid FindPeaks with Gaussian peak in the counts
        summed from detector pixels on the same row.

        Assumption
        1. The maximum count is belonged to the real peak

        Parameters
        ----------
        ws: MatrixWorkspace
            workspace to find peak
        crop: int
            number of pixels to crop out at the edge of detector
        factor: float
            multiplier factor to extend from peak width to peak range

        Returns
        -------
        tuple
            peak range, low resolution range

        """
        # Sum detector counts into 1D
        y = ws.extractY()
        y = np.reshape(y, (256, 304, y.shape[1]))
        p_vs_t = np.sum(y, axis=0)
        signal = np.sum(p_vs_t, axis=1)

        # Max index as the "observed" peak center
        max_index = np.argmax(signal)

        # Fit peak by Gaussian
        # create workspace
        now = datetime.datetime.now()
        ws_name = f"REL{now.hour:02}{now.minute:02}{now.second:02}{now.microsecond:04}.dat"
        CreateWorkspace(DataX=np.arange(len(signal)), DataY=signal, DataE=np.sqrt(signal), OutputWorkspace=ws_name)

        # prepare fitting
        model_ws_name = f"{ws_name}_model"
        param_ws_name = f"{ws_name}_parameter"
        peak_ws_name = f"{ws_name}_peaks"

        FitPeaks(
            InputWorkspace=ws_name,
            OutputWorkspace=peak_ws_name,
            PeakCenters=f"{max_index}",
            FitWindowBoundaryList=f"{crop},{signal.shape[0]-crop}",
            HighBackground=False,
            ConstrainPeakPositions=False,
            FittedPeaksWorkspace=model_ws_name,
            OutputPeakParametersWorkspace=param_ws_name,
            RawPeakParameters=False,
        )

        # Retrieve value
        peak_width = mtd[param_ws_name].cell(0, 3)
        peak_center = mtd[param_ws_name].cell(0, 2)

        info_str = f"{ws}: Max = {max_index}, Peak center = {peak_center}, Width = {peak_width}"
        logger.notice(info_str)

        # Form output
        peak = [int(peak_center - factor * peak_width), int(ceil(peak_center + factor * peak_width))]

        # Delete workspaces
        for ws_name in [peak_ws_name, model_ws_name, param_ws_name]:
            DeleteWorkspace(ws_name)

        return peak, [0, 255]


AlgorithmFactory.subscribe(LRDirectBeamSort)
