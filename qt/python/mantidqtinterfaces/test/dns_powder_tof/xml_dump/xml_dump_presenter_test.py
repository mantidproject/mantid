# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS xml data dump presenter.
"""

import unittest
from collections import OrderedDict
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model import DNSXMLDumpModel
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_presenter import DNSXMLDumpPresenter
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_view import DNSXMLDumpView


class DNSXMLDumpPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    model = None
    view = None
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.view = mock.create_autospec(DNSXMLDumpView, instance=True)
        cls.model = mock.create_autospec(DNSXMLDumpModel, instance=True)
        cls.parent = mock.Mock()
        cls.presenter = DNSXMLDumpPresenter(view=cls.view, model=cls.model, name="xml_dump", parent=cls.parent)

        cls.model.xml_file_to_dict.return_value = {"test": 123}
        cls.view.get_file_header.return_value = {"manitd_version": 1.0}
        cls.view.open_load_filename.return_value = ["test.xml", 2]
        cls.view.open_save_filename.return_value = ["test2.xml", 2]

    def setUp(self):
        self.view.get_file_header.reset_mock()
        self.view.open_load_filename.reset_mock()
        self.view.open_save_filename.reset_mock()
        self.model.xml_file_to_dict.reset_mock()
        self.view.show_status_message.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSXMLDumpPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertIsNone(self.presenter.last_filename)

    def test_load_xml(self):
        testv = self.presenter.load_xml()
        self.model.xml_file_to_dict.assert_called_once()
        self.assertEqual(testv, {"test": 123})

    def test_save_as_xml(self):
        self.view.open_save_filename.return_value = ["", 2]
        testv = self.presenter.save_as_xml()
        self.assertEqual(testv, "")
        self.model.dict_to_xml_file.assert_not_called()
        self.view.open_save_filename.return_value = ["test2.xml", 2]
        testv = self.presenter.save_as_xml()
        self.view.get_file_header.assert_called_once()
        self.model.dict_to_xml_file.assert_called_once_with(OrderedDict(), "test2.xml", {"manitd_version": 1.0})
        self.view.show_status_message.assert_called_once()
        self.assertEqual(testv, "test2.xml")

    def test_save_xml(self):
        self.presenter.last_filename = None
        self.presenter.save_xml()
        self.view.open_save_filename.assert_called_once()
        self.view.open_save_filename.reset_mock()
        self.view.get_file_header.reset_mock()
        self.view.show_status_message.reset_mock()

        self.presenter.last_filename = "te.xml"
        self.presenter.save_xml()
        self.view.open_save_filename.assert_not_called()
        self.view.get_file_header.assert_called_once()
        self.model.dict_to_xml_file(OrderedDict(), "te.xml", {"manitd_version": 1.0})
        self.view.show_status_message.assert_called_once()

    def test_get_xml_file_path_for_loading(self):
        self.view.open_load_filename.return_value = ["test.xml", 2]
        testv = self.presenter._get_xml_file_path_for_loading()
        self.view.open_load_filename.assert_called_once()
        self.assertEqual(testv, "test.xml")
        self.view.open_load_filename.return_value = ["test", 2]
        testv = self.presenter._get_xml_file_path_for_loading()
        self.assertEqual(testv, "test.xml")

    def get_xml_file_path_for_saving(self):
        self.view.open_save_filename.return_value = ["test.xml", 2]
        testv = self.presenter._get_xml_file_path_for_loading()
        self.view.open_save_filename.assert_called_once()
        self.assertEqual(testv, "test.xml")
        self.view.open_save_filename.return_value = ["test", 2]
        testv = self.presenter._get_xml_file_path_for_loading()
        self.assertEqual(testv, "test.xml")


if __name__ == "__main__":
    unittest.main()
