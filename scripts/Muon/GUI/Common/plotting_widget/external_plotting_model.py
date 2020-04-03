# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List
from mantid.dataobjects import Workspace2D


class PlotInformation(NamedTuple):
    workspace: Workspace2D
    specNum: int
    axis: int
    normalised: bool
    errors: bool


class ExternalPlottingModel(object):

    def get_plotted_workspaces_and_indices_from_axes(self, axes) -> List[PlotInformation]:
        plotted_data = []
        for i, axis in enumerate(axes):
            artists = axis.get_tracked_artists()
            for artist in artists:
                is_normalised = axis.get_artist_normalization_state(artist)
                workspace, index = axis.get_artists_workspace_and_spec_num(artist)
                plotted_data.append(PlotInformation(workspace=workspace, specNum=index, axis=i,
                                                    normalised=is_normalised))
        return plotted_data
