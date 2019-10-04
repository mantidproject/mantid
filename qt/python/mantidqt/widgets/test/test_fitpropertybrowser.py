# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.py3compat.mock import MagicMock, Mock, patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser.fitpropertybrowser import FitPropertyBrowser
from testhelpers import assertRaisesNothing


@start_qapplication
class FitPropertyBrowserTest(unittest.TestCase):

    def test_initialization_does_not_raise(self):
        assertRaisesNothing(self, self._create_widget)

    @patch('mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.normaliseData')
    def test_normalise_data_set_on_fit_menu_shown(self, normaliseData_mock):
        for normalised in [True, False]:
            ws_artist_mock = Mock(is_normalized=normalised, workspace_index=0)
            axes_mock = Mock(tracked_workspaces={'ws_name': [ws_artist_mock]})
            property_browser = self._create_widget()
            with patch.object(property_browser, 'get_axes', lambda: axes_mock):
                with patch.object(property_browser, 'workspaceName', lambda: 'ws_name'):
                    property_browser.getFitMenu().aboutToShow.emit()
            property_browser.normaliseData.assert_called_once_with(normalised)
            normaliseData_mock.reset_mock()

    def test_plot_guess_plots_for_table_workspaces(self):
        table = WorkspaceFactory.createTable()
        table.addColumn('double', 'X', 1)
        table.addColumn('double', 'Y', 2)
        for i in range(1,10):
            table.addRow([0.1*i, 5])
        name = "table_name"
        AnalysisDataService.Instance().addOrReplace(name, table)
        property_browser = self._create_widget()
        property_browser.getFittingFunction = Mock(return_value='name=FlatBackground')
        property_browser.workspaceName = Mock(return_value=name)
        property_browser.startX = Mock(return_value=0.15)
        property_browser.endX = Mock(return_value=0.95)
        property_browser.plot_guess()

        self.assertEqual(1, property_browser.get_axes().plot.call_count)

    # Private helper functions
    def _create_widget(self, canvas=MagicMock(), toolbar_manager=Mock()):
        return FitPropertyBrowser(canvas, toolbar_manager)


if __name__ == '__main__':
    unittest.main()
