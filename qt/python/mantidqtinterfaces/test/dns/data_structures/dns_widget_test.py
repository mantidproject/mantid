# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.dns.data_structures.dns_widget import \
    DNSWidget


class DNSWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        name = 'test'
        cls.widget = DNSWidget(name=name, parent=cls.parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget, object)
        self.assertEqual(self.widget.parent, self.parent)
        self.assertTrue(hasattr(self.widget, 'view'))
        self.assertTrue(hasattr(self.widget, 'presenter'))
        self.assertTrue(hasattr(self.widget, 'model'))
        self.assertEqual(self.widget.name, 'test')

    def test_has_view(self):
        self.widget.view = None
        self.assertFalse(self.widget.has_view())
        self.widget.view = 1
        self.assertTrue(self.widget.has_view())

    def test_has_model(self):
        self.widget.model = None
        self.assertFalse(self.widget.has_model())
        self.widget.model = 1
        self.assertTrue(self.widget.has_model())

    def test_has_presenter(self):
        self.widget.presenter = None
        self.assertFalse(self.widget.has_presenter())
        self.widget.presenter = 1
        self.assertTrue(self.widget.has_presenter())

    def test_update_progress(self):
        self.widget.presenter = mock.Mock()
        self.widget.presenter.update_progress = mock.Mock()
        self.widget.update_progress(1, 10)
        self.widget.presenter.update_progress.assert_called_once_with(1, 10)


if __name__ == '__main__':
    unittest.main()
