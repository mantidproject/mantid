# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    AnalysisDataService,
    CloneWorkspace,
    ConjoinWorkspaces,
    CropWorkspaceRagged,
    DeleteWorkspace,
    MatchSpectra,
    Rebin,
    SumSpectra,
)
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, WorkspaceProperty, WorkspaceGroup, ADSValidator
from mantid.dataobjects import Workspace2D
from mantid.kernel import Direction, FloatArrayProperty, StringArrayProperty
import numpy as np


class MatchAndMergeWorkspaces(DataProcessorAlgorithm):
    def name(self):
        return "MatchAndMergeWorkspaces"

    def category(self):
        return "Workflow\\Diffraction"

    def seeAlso(self):
        return []

    def summary(self):
        return "Merges a group workspace using weighting from a set of range limits for each workspace."

    def checkGroups(self):
        return False

    def version(self):
        return 1

    def validateInputs(self):
        # given workspaces must exist
        # and must be public of ExperimentInfo
        issues = dict()
        ws_list = self.getProperty("InputWorkspaces").value
        spectra_count = 0
        for name_in_list in ws_list:
            ws_in_list = AnalysisDataService.retrieve(name_in_list)
            if isinstance(ws_in_list, Workspace2D):
                spectra_count += ws_in_list.getNumberHistograms()
            if isinstance(ws_in_list, WorkspaceGroup):
                for ws_in_group in ws_in_list:
                    spectra_count += ws_in_group.getNumberHistograms()

        x_min = self.getProperty("XMin").value
        if not x_min.size == spectra_count:
            issues["XMin"] = "XMin entries does not match size of workspace group"

        x_max = self.getProperty("XMax").value
        if not x_max.size == spectra_count:
            issues["XMax"] = "XMax entries does not match size of workspace group"

        return issues

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspaces", direction=Direction.Input, validator=ADSValidator()),
            doc="List of workspaces or group workspace containing workspaces to be merged.",
        )
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The merged workspace.")
        self.declareProperty(FloatArrayProperty("XMin", [], direction=Direction.Input), doc="Array of minimum X values for each workspace.")
        self.declareProperty(FloatArrayProperty("XMax", [], direction=Direction.Input), doc="Array of maximum X values for each workspace.")
        self.declareProperty("CalculateScale", True, doc="Calculate scale factor when matching spectra.")
        self.declareProperty("CalculateOffset", True, doc="Calculate vertical shift when matching spectra.")

    def PyExec(self):
        ws_list = self.getProperty("InputWorkspaces").value
        x_min = self.getProperty("XMin").value
        x_max = self.getProperty("XMax").value
        scale_bool = self.getProperty("CalculateScale").value
        offset_bool = self.getProperty("CalculateOffset").value
        flattened_list = self.unwrap_groups(ws_list)
        # If some banks are to be excluded this line removes them from flattened_list, x_min and x_max
        flattened_list, x_min, x_max = self.exclude_banks(flattened_list, x_min, x_max)
        largest_range_spectrum, rebin_param = self.get_common_bin_range_and_largest_spectra(flattened_list)
        CloneWorkspace(InputWorkspace=flattened_list[0], OutputWorkspace="ws_conjoined")
        Rebin(InputWorkspace="ws_conjoined", OutputWorkspace="ws_conjoined", Params=rebin_param)
        for ws in flattened_list[1:]:
            temp = CloneWorkspace(InputWorkspace=ws)
            temp = Rebin(InputWorkspace=temp, Params=rebin_param)
            ConjoinWorkspaces(InputWorkspace1="ws_conjoined", InputWorkspace2=temp, CheckOverlapping=False, CheckMatchingBins=False)
        ws_conjoined = AnalysisDataService.retrieve("ws_conjoined")
        ref_spec = ws_conjoined.getSpectrum(largest_range_spectrum).getSpectrumNo()
        ws_conjoined, offset, scale, chisq = MatchSpectra(
            InputWorkspace=ws_conjoined, ReferenceSpectrum=ref_spec, CalculateScale=scale_bool, CalculateOffset=offset_bool
        )
        x_min, x_max, bin_width = self.fit_x_lims_to_match_histogram_bins(ws_conjoined, x_min, x_max)

        ws_conjoined = CropWorkspaceRagged(InputWorkspace=ws_conjoined, XMin=x_min, XMax=x_max)
        ws_conjoined = Rebin(InputWorkspace=ws_conjoined, Params=[min(x_min), bin_width, max(x_max)])
        merged_ws = SumSpectra(InputWorkspace=ws_conjoined, WeightedSum=True, MultiplyBySpectra=False, StoreInADS=False)
        DeleteWorkspace(ws_conjoined)
        self.setProperty("OutputWorkspace", merged_ws)

    @staticmethod
    def unwrap_groups(inputs):
        output = []
        for name_in_list in inputs:
            ws_in_list = AnalysisDataService.retrieve(name_in_list)
            if isinstance(ws_in_list, Workspace2D):
                output.append(ws_in_list)
            if isinstance(ws_in_list, WorkspaceGroup):
                for ws_in_group in ws_in_list:
                    output.append(ws_in_group)
        return output

    @staticmethod
    def get_common_bin_range_and_largest_spectra(ws_list):
        x_min = np.inf
        x_max = -np.inf
        x_num = -np.inf
        ws_max_range = 0
        largest_range_spectrum = 0
        for i in range(len(ws_list)):
            for j in range(ws_list[i].getNumberHistograms()):
                x_data = ws_list[i].dataX(j)
                x_min = min(np.min(x_data), x_min)
                x_max = max(np.max(x_data), x_max)
                x_num = max(x_data.size, x_num)
                ws_range = np.max(x_data) - np.min(x_data)
                if ws_range > ws_max_range:
                    largest_range_spectrum = i
                    ws_max_range = ws_range
        if x_min.any() == -np.inf or x_min.any() == np.inf:
            raise AttributeError("Workspace x range contains an infinite value.")
        return largest_range_spectrum, [x_min, (x_max - x_min) / x_num, x_max]

    @staticmethod
    def fit_x_lims_to_match_histogram_bins(ws_conjoined, x_min, x_max):
        bin_width = np.inf
        for i in range(x_min.size):
            pdf_x_array = ws_conjoined.readX(i)
            x_min[i] = pdf_x_array[np.amin(np.where(pdf_x_array >= x_min[i]))]
            x_max[i] = pdf_x_array[np.amax(np.where(pdf_x_array <= x_max[i]))]
            bin_width = min(pdf_x_array[1] - pdf_x_array[0], bin_width)
        if x_min.any() == -np.inf or x_min.any() == np.inf:
            raise AttributeError("Limits contains an infinite value.")
        return x_min, x_max, bin_width

    @staticmethod
    def exclude_banks(flat_list, x_min, x_max):
        index_to_remove = []
        for i in range(x_min.size):
            if x_min[i] == -1 and x_max[i] == -1:
                index_to_remove.append(i)
            else:
                if x_min[i] == -1 or x_max[i] == -1:
                    raise RuntimeError(
                        "The banks to be excluded in q_lims do not match. Please check "
                        "that -1 has been added in the correct place on both lists."
                    )
        revised_wslist = np.delete(flat_list, index_to_remove)
        x_min = np.delete(x_min, index_to_remove)
        x_max = np.delete(x_max, index_to_remove)
        if len(x_min) < 1 or len(x_max) < 1:
            raise RuntimeError("You have excluded all banks. Merging requires at least one bank.")
        return revised_wslist, x_min, x_max


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MatchAndMergeWorkspaces)
