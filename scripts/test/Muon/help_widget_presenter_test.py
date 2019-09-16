# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidgetPresenter
from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView



@start_qapplication
class HelpWidgetPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = HelpWidgetView("test")
        self.presenter = HelpWidgetPresenter(self.view)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.manageuserdirectories.OpenManageUserDirectories')
    def test_that_manage_directories_button_clicked_opens_directory_manager(self, OpenManageUserDirectories_mock):
        self.view.manage_user_dir_button.clicked.emit(True)

        OpenManageUserDirectories_mock.open_mud.assert_called_once_with(self.view)

    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.manageuserdirectories.OpenManageUserDirectories')
    def test_that_manage_directories_button_does_not_open_multiple_times(self, OpenManageUserDirectories_mock):
        OpenManageUserDirectories_mock.currently_open_mud = True
        self.view.manage_user_dir_button.clicked.emit(True)

        # Checking that the value of currently_open_mud hasn't changed.
        self.assertTrue(OpenManageUserDirectories_mock.currently_open_mud)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
