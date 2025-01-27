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
import xml.etree.ElementTree as etree
from collections import OrderedDict
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model import DNSXMLDumpModel


class DNSXMLDumpModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.model = DNSXMLDumpModel(parent=cls.parent)
        cls.testdic = {"a": 1, "c": {"c1": 1.1, "c2": 2.2}}

    def test___init__(self):
        self.assertIsInstance(self.model, DNSXMLDumpModel)
        self.assertIsInstance(self.model, DNSObsModel)

    def test_convert_type(self):
        # bool
        testv = self.model._convert_type("True", "bool")
        self.assertIsInstance(testv, bool)
        self.assertTrue(testv)
        testv = self.model._convert_type("False", "bool")
        self.assertIsInstance(testv, bool)
        self.assertFalse(testv)
        # int
        testv = self.model._convert_type("123", "int")
        self.assertIsInstance(testv, int)
        self.assertEqual(testv, 123)
        # float
        testv = self.model._convert_type("1.23", "float")
        self.assertIsInstance(testv, float)
        self.assertEqual(testv, 1.23)
        # empty list
        testv = self.model._convert_type("[]", "emptylist")
        self.assertIsInstance(testv, list)
        self.assertEqual(testv, [])
        # list
        testv = self.model._convert_type("[1, 2, 3]", "intlist")
        self.assertIsInstance(testv, list)
        self.assertIsInstance(testv[0], int)
        self.assertEqual(testv, [1, 2, 3])
        # None
        testv = self.model._convert_type("None", "None")
        self.assertIsNone(testv)
        testv = self.model._convert_type("test", "hu")
        self.assertIsInstance(testv, str)
        self.assertEqual(testv, "test")

    def test_dict_elm_to_xml(self):
        testv = self.model._dict_element_to_xml({})
        self.assertIsInstance(testv, etree.Element)
        testv = self.model._dict_element_to_xml(self.testdic)
        self.assertIsInstance(testv, etree.Element)
        self.assertIsInstance(testv[0], etree.Element)
        self.assertIsInstance(testv[1], etree.Element)
        self.assertIsInstance(testv[1][0], etree.Element)
        self.assertEqual(testv[0].tag, "a")
        self.assertEqual(testv[0].attrib, {"type": "int"})
        self.assertEqual(testv[1][0].attrib, {"type": "float"})
        self.assertEqual(testv[1][0].tag, "c1")

    @patch("mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model.save_txt")
    def test_dic_to_xml_file(self, mock_savetxt):
        self.model.dict_to_xml_file(self.testdic, "test.xml", {"mv": 1.0})
        mock_savetxt.assert_called_once()
        mock_savetxt.reset_mock()
        self.model.dict_to_xml_file(self.testdic, "", {"mv": 1.0})
        mock_savetxt.assert_not_called()

    def test_dictionary_to_xml_string(self):
        # this is not really a unittest, since it tests also if etree
        # returns right stuff
        testv = self.model._dict_to_xml_string(self.testdic)
        teststr = (
            '<?xml version="1.0" ?>\n<document>\n  <a type="int">1'
            '</a>\n  <c>\n    <c1 type="float">1.1</c1>\n    <c2 t'
            'ype="float">2.2</c2>\n  </c>\n</document>\n'
        )
        self.assertIsInstance(testv, str)
        self.assertEqual(testv, teststr)

    def test_return_type(self):
        self.assertEqual(self.model._return_type(True), "bool")
        self.assertEqual(self.model._return_type(1), "int")
        self.assertEqual(self.model._return_type(1.2), "float")
        self.assertEqual(self.model._return_type([1.2, 1.2]), "floatlist")
        self.assertEqual(self.model._return_type([]), "emptylist")
        self.assertEqual(self.model._return_type(None), "None")
        self.assertEqual(self.model._return_type("123"), "str")
        self.assertEqual(self.model._return_type(self), "str")

    @patch("mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model.etree.parse")
    def test_load_file_to_xml_tree(self, mock_parse):
        mock_parse.return_value = "test"
        testv = self.model._load_file_to_xml_tree("123")
        mock_parse.assert_called_once_with("123")
        self.assertEqual(testv, "test")
        mock_parse.side_effect = IOError()
        mock_parse.reset_mock()
        testv = self.model._load_file_to_xml_tree("123")
        mock_parse.assert_called_once_with("123")
        self.assertFalse(testv)

    def test_check_instrument_name(self):
        tree = self.model._dict_element_to_xml({"header": {"instrument_name": "DNS"}})
        self.assertTrue(self.model._check_instrument_name(tree))
        tree = self.model._dict_element_to_xml({"header": {"instrument_name": "MLZ"}})
        self.assertFalse(self.model._check_instrument_name(tree))

    def test_xml_file_to_dict(self):
        self.assertIsNone(self.model.xml_file_to_dict(""))

    def test_xml_to_dict(self):
        tree = self.model._dict_element_to_xml(self.testdic)
        testv = self.model._xml_to_dict(tree, {})
        self.assertEqual(testv, {"document": OrderedDict([("a", 1), ("c", OrderedDict([("c1", 1.1), ("c2", 2.2)]))])})


if __name__ == "__main__":
    unittest.main()
