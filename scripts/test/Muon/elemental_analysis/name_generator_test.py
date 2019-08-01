# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

import unittest
import sys

from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name


class NameGeneratorTest(unittest.TestCase):
    def test_that_gen_name_returns_name_if_it_contains_element(self):
        element_name = 'name2'
        name = 'name1name2name3nam4'

        self.assertEqual(gen_name(element_name, name), 'name1name2name3nam4')

    def test_that_gen_name_combines_element_and_label(self):
        element = 'element'
        name = 'label'

        self.assertEqual(gen_name(element, name), 'element label')

    def test_that_gen_name_with_non_string_element_throws(self):
        element1 = None
        element2 = 1
        element3 = (3.0, 'string')
        name = 'valid_name'

        with self.assertRaises(TypeError) as err:
            gen_name(element1, name)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception),
                             "'None' expected element to be 'str', found '<type 'NoneType'>' instead")
        else:
            self.assertEqual(str(err.exception),
                             "'None' expected element to be 'str', found '<class 'NoneType'>' instead")

        with self.assertRaises(TypeError) as err:
            gen_name(element2, name)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception),
                             "'1' expected element to be 'str', found '<type 'int'>' instead")
        else:
            self.assertEqual(str(err.exception),
                             "'1' expected element to be 'str', found '<class 'int'>' instead")

        with self.assertRaises(TypeError) as err:
            gen_name(element3, name)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception),
                             "'(3.0, 'string')' expected element to be 'str', found '<type 'tuple'>' instead")
        else:
            self.assertEqual(str(err.exception),
                             "'(3.0, 'string')' expected element to be 'str', found '<class 'tuple'>' instead")

    def test_that_gen_name_with_non_string_name_throws(self):
        element = 'valid element'
        name1 = None
        name2 = 1
        name3 = (3.0, 'string')

        with self.assertRaises(TypeError) as err:
            gen_name(element, name1)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception), "'None' expected name to be 'str', found '<type 'NoneType'>' instead")
        else:
            self.assertEqual(str(err.exception), "'None' expected name to be 'str', found '<class 'NoneType'>' instead")

        with self.assertRaises(TypeError) as err:
            gen_name(element, name2)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception), "'1' expected name to be 'str', found '<type 'int'>' instead")
        else:
            self.assertEqual(str(err.exception), "'1' expected name to be 'str', found '<class 'int'>' instead")

        with self.assertRaises(TypeError) as err:
            gen_name(element, name3)
        if sys.version_info[:2] < (3, 0):
            self.assertEqual(str(err.exception),
                             "'(3.0, 'string')' expected name to be 'str', found '<type 'tuple'>' instead")
        else:
            self.assertEqual(str(err.exception),
                             "'(3.0, 'string')' expected name to be 'str', found '<class 'tuple'>' instead")

    def test_that_gen_name_with_unicode_string_does_not_throw(self):
        element = u'element'
        label = u'label'
        result = u'element label'
        self.assertEqual(gen_name(element, label), result)


if __name__ == '__main__':
    unittest.main()
