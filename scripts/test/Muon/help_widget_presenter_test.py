# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys

from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidgetPresenter
import unittest
from Muon.GUI.Common import mock_widget

if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock


class HelpWidgetPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.view = HelpWidgetView()
        self.presenter = HelpWidgetPresenter(self.view)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

    def test_that_when_help_button_clicked_correct_message_shown(self):
        self.view.help_button.clicked.emit(True)

        self.view.warning_popup.assert_called_once_with("Help is not currently implemented!")

    @mock.patch('Muon.GUI.Common.help_widget.help_widget_view.MantidQt')
    def test_that_manage_directories_button_clicked_opens_directory_manager(self, mantidqt_mock):
        self.view.manage_user_dir_button.clicked.emit(True)

        mantidqt_mock.API.ManageUserDirectories.openUserDirsDialog.assert_called_once_with(self.view)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
