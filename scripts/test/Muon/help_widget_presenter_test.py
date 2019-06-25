# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidgetPresenter
from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView



class HelpWidgetPresenterTest(GuiTest):
    def setUp(self):
        self.view = HelpWidgetView("test")
        self.presenter = HelpWidgetPresenter(self.view)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.ManageUserDirectories')
    def test_that_manage_directories_button_clicked_opens_directory_manager(self, ManageUserDirectories_mock):
        self.view.manage_user_dir_button.clicked.emit(True)

        ManageUserDirectories_mock.openUserDirsDialog.assert_called_once_with(self.view)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
