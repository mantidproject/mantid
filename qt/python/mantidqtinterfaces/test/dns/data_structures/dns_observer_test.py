# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from collections import OrderedDict
from unittest import mock

from mantidqtinterfaces.dns.data_structures.dns_observer import DNSObserver


class DNSObserverTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.parent.parent = mock.Mock()
        cls.parent.parent.modus = mock.Mock()
        cls.view = mock.Mock()
        cls.model = mock.Mock()
        cls.observer = DNSObserver(parent=cls.parent,
                                   name='test2',
                                   view=cls.view,
                                   model=cls.model)
        cls.parent.parent.modus.name = 'test'
        cls.view.set_state = mock.Mock()
        cls.view.get_state = mock.Mock(return_value={'1': 2})
        cls.view.raise_error = mock.Mock()

    def test___init__(self):
        self.assertIsInstance(self.observer, object)
        self.assertIsInstance(self.observer, DNSObserver)
        self.assertEqual(self.observer.name, 'test2')
        self.assertIsInstance(self.observer.view, mock.Mock)
        self.assertIsInstance(self.observer.model, mock.Mock)
        self.assertIsInstance(self.observer.parent, mock.Mock)
        self.assertIsInstance(self.observer.param_dict, OrderedDict)
        self.assertIsInstance(self.observer.own_dict, OrderedDict)
        self.assertEqual(self.observer.modus, '')

    def test_update(self):
        self.observer.on_modus_change = mock.Mock()
        self.observer.update({'test2': {'123': 2}})
        self.assertEqual(self.observer.modus, 'test')
        self.observer.on_modus_change.assert_called_once()

    def test_set_view_from_param(self):
        self.observer.set_view_from_param()
        self.view.set_state.assert_called_once()

    def test_get_option_dict(self):
        testv = self.observer.get_option_dict()
        self.view.get_state.assert_called_once()
        self.assertEqual(testv, {'1': 2})

    def test_raise_error(self):
        self.observer.raise_error('123', critical=True, doraise=False)
        self.view.raise_error.assert_not_called()
        self.observer.raise_error('123', critical=True, doraise=True)
        self.view.raise_error.assert_called_once_with('123', True, False)

    # def test_process_request(self):
    # pass

    # def test_tab_got_focus(self):
    # pass

    # def test_on_modus_change(self):
    # pass

    # def test_process_commandline_request(self):
    # pass


if __name__ == '__main__':
    unittest.main()
