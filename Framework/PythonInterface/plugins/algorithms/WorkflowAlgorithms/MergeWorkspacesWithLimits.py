# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (ConjoinWorkspaces, CropWorkspaceRagged, DeleteWorkspace, MatchSpectra, Rebin, SumSpectra)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, WorkspaceProperty, )
from mantid.kernel import (Direction, FloatArrayProperty)
import numpy as np


class MergeWorkspacesWithLimits(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

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

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('WorkspaceGroup', '', direction=Direction.Input),
                             doc='Workspace group for merging')
        self.declareProperty(WorkspaceProperty('MergedWorkspace', '', direction=Direction.Output),
                             doc='The weighted merged workspace')
        self.declareProperty(FloatArrayProperty('XMin', [],),
                             doc='Array of minimum X values for each workspace')
        self.declareProperty(FloatArrayProperty('XMax', [],),
                             doc='Array of maximum X values for each workspace')

    def PyExec(self):
        ws_group = self.getProperty('WorkspaceGroup').value
        x_min = self.getProperty('XMin').value
        x_max = self.getProperty('XMax').value
        min_x = np.inf
        max_x = -np.inf
        num_x = -np.inf
        ws_max_range = 0
        largest_range_spectrum = 0
        for i in range(ws_group.size()):
            x_data = ws_group[i].dataX(0)
            min_x = min(np.min(x_data), min_x)
            max_x = max(np.max(x_data), max_x)
            num_x = max(x_data.size, num_x)
            ws_range = np.max(x_data) - np.min(x_data)
            if ws_range > ws_max_range:
                largest_range_spectrum = i + 1
                ws_max_range = ws_range
        ws_group = Rebin(InputWorkspace=ws_group,
                         Params=[min_x, (max_x - min_x) / num_x, max_x],
                         StoreInADS=False)
        while ws_group.size() > 1:
            ConjoinWorkspaces(InputWorkspace1=ws_group[0],
                              InputWorkspace2=ws_group[1])

        ws_conjoined, offset, scale, chisq = MatchSpectra(InputWorkspace=ws_group[0],
                                                          ReferenceSpectrum=largest_range_spectrum)
        bin_width = np.inf
        for i in range(x_min.size):
            pdf_x_array = ws_conjoined.readX(i)
            x_min[i] = pdf_x_array[np.amin(np.where(pdf_x_array >= x_min[i]))]
            x_max[i] = pdf_x_array[np.amax(np.where(pdf_x_array <= x_max[i]))]
            bin_width = min(pdf_x_array[1] - pdf_x_array[0], bin_width)
        ws_conjoined = CropWorkspaceRagged(InputWorkspace=ws_conjoined, XMin=x_min, XMax=x_max)
        ws_conjoined = Rebin(InputWorkspace=ws_conjoined, Params=[min(x_min), bin_width, max(x_max)])
        merged_ws = SumSpectra(InputWorkspace=ws_conjoined, WeightedSum=True, MultiplyBySpectra=False, StoreInADS=False)
        DeleteWorkspace(ws_group)
        DeleteWorkspace(ws_conjoined)
        self.setProperty('MergedWorkspace', merged_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MergeWorkspacesWithLimits)
