# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns.data_structures.dns_widget import \
    DNSWidget
from mantidqtinterfaces.dns.options.tof_powder_options_model import \
    DNSTofPowderOptionsModel
from mantidqtinterfaces.dns.options.tof_powder_options_presenter \
    import DNSTofPowderOptionsPresenter
from mantidqtinterfaces.dns.options.tof_powder_options_view import \
    DNSTofPowderOptionsView
from mantidqtinterfaces.dns.options.tof_powder_options_widget import \
    DNSTofPowderOptionsWidget

app, within_mantid = get_qapplication()


class DNSTofPowderOptionsWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSTofPowderOptionsWidget('tof_powder_options', parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSTofPowderOptionsWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSTofPowderOptionsView)
        self.assertIsInstance(self.widget.model, DNSTofPowderOptionsModel)
        self.assertIsInstance(self.widget.presenter,
                              DNSTofPowderOptionsPresenter)


if __name__ == '__main__':
    unittest.main()
