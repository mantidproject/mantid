# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel, PlotInformation
from mantid.plots import MantidAxes

NUM_AXES = 2
EXAMPLE_DATA = [["MUSR62260; Group; top; Asymmetry; MA", 1], ["MUSR62260; Group; bot; Asymmetry; MA", 1]]
NORMALISATION_STATE = False


class ExternalPlottingModelTest(unittest.TestCase):

    def setUp(self):
        self.external_plotting_model = ExternalPlottingModel()
        self.mock_artist = mock.NonCallableMock()

    def create_mock_mantid_axes(self):
        mock_axes = []
        for i in range(NUM_AXES):
            mock_axis = mock.Mock(spec=MantidAxes)
            mock_axis.get_artist_normalization_state.return_value = NORMALISATION_STATE
            mock_axis.get_artists_workspace_and_spec_num.return_value = EXAMPLE_DATA[i]
            mock_axis.get_tracked_artists.return_value = [self.mock_artist]
            mock_axes.append(mock_axis)
        return mock_axes

    def create_plot_information(self):
        plot_information = []
        for i in range(NUM_AXES):
            ws = EXAMPLE_DATA[i][0]
            index = EXAMPLE_DATA[i][1]
            plot_information.append(PlotInformation(workspace=ws, specNum=index, axis=i,
                                                    normalised=NORMALISATION_STATE))
        return plot_information

    def test_get_plotted_workspace_and_indicies_from_axes_calls_correct_functions(self):
        axes = self.create_mock_mantid_axes()

        self.external_plotting_model.get_plotted_workspaces_and_indices_from_axes(axes)
        for ax in axes:
            ax.get_tracked_artists.assert_called_once()
            ax.get_artists_workspace_and_spec_num.assert_called_once_with(self.mock_artist)

    def test_get_plotted_workspaces_and_indicies_from_axes_returns_correctly(self):
        axes = self.create_mock_mantid_axes()
        expected_plot_information = self.create_plot_information()

        return_plot_information = self.external_plotting_model.get_plotted_workspaces_and_indices_from_axes(axes)

        for return_plot_info, expected_plot_info in zip(return_plot_information, expected_plot_information):
            self.assertEqual(return_plot_info, expected_plot_info)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
