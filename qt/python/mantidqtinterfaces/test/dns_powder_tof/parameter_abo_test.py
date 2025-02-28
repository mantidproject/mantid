# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from collections import OrderedDict
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.parameter_abo import ParameterAbo


class ParameterAboTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    observer1 = None

    @classmethod
    def setUpClass(cls):
        cls.observer1 = mock.create_autospec(DNSObserver, instance=True)
        cls.observer1.name = "observer1"
        cls.observer1.load_xml = mock.Mock(return_value={"test": 1})
        cls.observer1.save_xml = mock.Mock()
        cls.observer1.save_as_xml = mock.Mock()
        cls.testdic = {"a": 1}

    def setUp(self):
        self.model = ParameterAbo()
        self.observer1.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.model, ParameterAbo)
        self.assertIsInstance(self.model, object)
        self.assertEqual(self.model.observers, [])
        self.assertEqual(self.model.observer_dict, {})
        self.assertEqual(self.model.gui_parameter, OrderedDict())

    def test_clear_gui_parameter_dict(self):
        self.model.gui_parameter["a"] = 1
        self.model.clear_gui_parameter_dict()
        self.assertEqual(self.model.gui_parameter, OrderedDict())

    def test_register(self):
        self.model.register(self.observer1)
        self.assertEqual(len(self.model.observers), 1)
        self.assertEqual(self.model.observer_dict["observer1"], self.observer1)
        self.assertEqual(self.observer1.request_from_abo, self.model.process_request)
        self.observer1.get_option_dict.assert_called_once()

    def test_unregister(self):
        self.model.observers = [self.observer1]
        self.model.observer_dict = {"observer1": self.observer1}
        self.assertEqual(len(self.model.observers), 1)
        self.assertEqual(self.model.observer_dict["observer1"], self.observer1)
        self.model.unregister(self.observer1)
        self.assertEqual(self.model.observers, [])
        self.assertEqual(self.model.observer_dict, {})

    def test_clear(self):
        self.model.observers = [self.observer1]
        self.model.observer_dict = {"observer1": self.observer1}
        self.assertEqual(len(self.model.observers), 1)
        self.assertEqual(self.model.observer_dict["observer1"], self.observer1)
        self.model.clear()
        self.assertEqual(self.model.observers, [])
        self.assertEqual(self.model.observer_dict, {})

    def test__notify_observers(self):
        self.model.observers = [self.observer1, self.observer1]
        self.model._notify_observers()
        self.assertEqual(self.observer1.update.call_count, 2)

    def test_notify_modus_change(self):
        self.model.observers = [self.observer1, self.observer1]
        self.model.notify_modus_change()
        self.assertEqual(self.observer1.on_modus_change.call_count, 2)

    def test_notify_focused_tab(self):
        self.model.notify_focused_tab(self.observer1)
        self.observer1.tab_got_focus.assert_called_once()

    def xml_load(self):
        self.model.observers = [self.observer1]
        self.model.observer_dict["xml_dump"] = self.observer1
        self.model.xml_load()
        self.assertEqual(self.model.gui_parameter, {"test": 1})
        self.observer1.set_view_from_param.assert_called_once()

    def test_xml_save(self):
        self.model.observer_dict["xml_dump"] = self.observer1
        self.model.xml_save()
        self.observer1.save_xml.assert_called_once()

    def test_xml_save_as(self):
        self.model.observer_dict["xml_dump"] = self.observer1
        self.model.xml_save_as()
        self.observer1.save_as_xml.assert_called_once()

    def test_update_from_observer(self):
        self.model.update_from_observer(self.observer1)
        self.assertEqual(len(self.model.gui_parameter), 1)
        self.observer1.get_option_dict.assert_called_once()

    def test_update_from_all_observers(self):
        self.model.observers = [self.observer1, self.observer1]
        self.model.update_from_all_observers()
        self.assertEqual(len(self.model.gui_parameter), 1)

    def test_process_request(self):
        self.model.observers = [self.observer1, self.observer1]
        self.model.process_request()
        self.assertEqual(self.observer1.process_request.call_count, 2)


if __name__ == "__main__":
    unittest.main()
