# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidgetPresenter
from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView
from Muon.GUI.Common.test_helpers import mock_widget


class HelpWidgetPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.view = HelpWidgetView()
        self.presenter = HelpWidgetPresenter(self.view)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

<<<<<<< HEAD
    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.manageuserdirectories')
    def test_that_manage_directories_button_clicked_opens_directory_manager(self, manage_user_directories_mock):
        self.view.manage_user_dir_button.clicked.emit(True)

        manage_user_directories_mock.ManageUserDirectories.openUserDirsDialog.assert_called_once_with(self.view)
=======
    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.ManageUserDirectories')
    def test_that_manage_directories_button_clicked_opens_directory_manager(self, mock_ManageUserDirectories):
        self.view.manage_user_dir_button.clicked.emit(True)

        mock_ManageUserDirectories.openUserDirsDialog.assert_called_once_with(self.view)
>>>>>>> origin/master


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
