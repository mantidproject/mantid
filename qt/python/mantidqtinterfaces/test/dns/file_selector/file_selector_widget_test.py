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
from mantidqtinterfaces.dns.file_selector.file_selector_model import \
    DNSFileSelectorModel
from mantidqtinterfaces.dns.file_selector.file_selector_presenter \
    import DNSFileSelectorPresenter
from mantidqtinterfaces.dns.file_selector.file_selector_view import \
    DNSFileSelectorView
from mantidqtinterfaces.dns.file_selector.file_selector_widget \
    import DNSFileSelectorWidget

app, within_mantid = get_qapplication()


class DNSFileSelectorWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSFileSelectorWidget('file_selector', parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSFileSelectorWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSFileSelectorView)
        self.assertIsInstance(self.widget.model, DNSFileSelectorModel)
        self.assertIsInstance(self.widget.presenter, DNSFileSelectorPresenter)


if __name__ == '__main__':
    unittest.main()
