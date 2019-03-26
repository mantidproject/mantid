# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.py3compat import mock

from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.Common import mock_widget


class LoadViewTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self.view = LoadView()
        self.slot = mock.Mock()
        self.view.load_button = mock.Mock()
        self.view.co_button = mock.Mock()
        self.view.spinbox = mock.Mock()

    def test_on_load_clicked(self):
        self.view.load_button.clicked.connect(self.slot)
        self.view.load_button.clicked.connect.assert_called_with(self.slot)

    def test_unreg_on_load_clicked(self):
        self.view.load_button.clicked.connect(self.slot)
        self.view.load_button.clicked.disconnect(self.slot)
        self.view.load_button.clicked.disconnect.assert_called_with(self.slot)

    def test_on_co_add_clicked(self):
        self.view.co_button.clicked.connect(self.slot)
        self.view.co_button.clicked.connect.assert_called_with(self.slot)

    def test_unreg_on_co_add_clicked(self):
        self.view.co_button.clicked.connect(self.slot)
        self.view.co_button.clicked.disconnect(self.slot)
        self.view.co_button.clicked.disconnect.assert_called_with(self.slot)

    def test_on_spinbox_changed(self):
        self.view.spinbox.valueChanged.connect(self.slot)
        self.view.spinbox.valueChanged.connect.assert_called_with(self.slot)

    def test_unreg_on_spinbox_changed(self):
        self.view.spinbox.valueChanged.connect(self.slot)
        self.view.spinbox.valueChanged.disconnect(self.slot)
        self.view.spinbox.valueChanged.disconnect.assert_called_with(self.slot)


if __name__ == "__main__":
    unittest.main()
