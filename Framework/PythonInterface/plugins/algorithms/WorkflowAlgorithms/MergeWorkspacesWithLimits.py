# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (ConjoinWorkspaces, CropWorkspaceRagged, DeleteWorkspace, MatchSpectra, Rebin, SumSpectra)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, WorkspaceProperty, WorkspaceGroupProperty)
from mantid.kernel import (Direction, FloatArrayProperty)
import numpy as np


class MergeWorkspacesWithLimits(DataProcessorAlgorithm):

    def name(self):
        return 'MergeWorkspacesWithLimits'

    def category(self):
        return 'Workflow\\Diffraction'

    def seeAlso(self):
        return []

    def summary(self):
        return 'Merges a group workspace using weighting from a set of range limits for each workspace.'

    def checkGroups(self):
        return False

    def version(self):
        return 1

    def validateInputs(self):
        # given workspaces must exist
        # and must be public of ExperimentInfo
        issues = dict()
        ws_group = self.getProperty('InputWorkspaces').value
        x_min = self.getProperty('XMin').value
        x_max = self.getProperty('XMax').value
        if not x_min.size == ws_group.size():
            issues['x_min'] = 'x_min entries does not match size of workspace group'
        if not x_max.size == ws_group.size():
            issues['x_max'] = 'x_max entries does not match size of workspace group'
        return issues

    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty('InputWorkspaces', '', direction=Direction.Input),
                             doc='The group workspace containing workspaces to be merged.')
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The merged workspace')
        self.declareProperty(FloatArrayProperty('XMin', [],),
                             doc='Array of minimum X values for each workspace')
        self.declareProperty(FloatArrayProperty('XMax', [],),
                             doc='Array of maximum X values for each workspace')

    def PyExec(self):
        ws_group = self.getProperty('InputWorkspaces').value
        x_min = self.getProperty('XMin').value
        x_max = self.getProperty('XMax').value
        largest_range_spectrum, rebin_param = self.get_common_bin_range_and_largest_spectra(ws_group)
        ws_group = Rebin(InputWorkspace=ws_group,
                         Params=rebin_param,
                         StoreInADS=False)
        while ws_group.size() > 1:
            ConjoinWorkspaces(InputWorkspace1=ws_group[0],
                              InputWorkspace2=ws_group[1],
                              CheckOverlapping=False)

        ws_conjoined, offset, scale, chisq = MatchSpectra(InputWorkspace=ws_group[0],
                                                          ReferenceSpectrum=largest_range_spectrum,
                                                          CalculateScale=False)
        x_min, x_max, bin_width = self.fit_x_lims_to_match_histogram_bins(ws_conjoined, x_min, x_max)

        ws_conjoined = CropWorkspaceRagged(InputWorkspace=ws_conjoined, XMin=x_min, XMax=x_max)
        ws_conjoined = Rebin(InputWorkspace=ws_conjoined, Params=[min(x_min), bin_width, max(x_max)])
        merged_ws = SumSpectra(InputWorkspace=ws_conjoined, WeightedSum=True, MultiplyBySpectra=False, StoreInADS=False)
        DeleteWorkspace(ws_group)
        DeleteWorkspace(ws_conjoined)
        self.setProperty('OutputWorkspace', merged_ws)

    @staticmethod
    def get_common_bin_range_and_largest_spectra(ws_group):
        x_min = np.inf
        x_max = -np.inf
        x_num = -np.inf
        ws_max_range = 0
        largest_range_spectrum = 0
        for i in range(ws_group.size()):
            x_data = ws_group[i].dataX(0)
            x_min = min(np.min(x_data), x_min)
            x_max = max(np.max(x_data), x_max)
            x_num = max(x_data.size, x_num)
            ws_range = np.max(x_data) - np.min(x_data)
            if ws_range > ws_max_range:
                largest_range_spectrum = i + 1
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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MergeWorkspacesWithLimits)
