# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name


class NameGeneratorTest(unittest.TestCase):
    def exception_produced(self, element, name, err_value, err_type):
        with self.assertRaises(TypeError) as err:
            gen_name(element, name)
            self.assertEqual(str(err.exception),
                             "{} expected element to be 'str', found '<class '{}'>' instead"
                             .format(err_value, err_type))

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

        self.exception_produced(element1, name, str(element1), 'NoneType')
        self.exception_produced(element2, name, str(element2), 'int')
        self.exception_produced(element3, name, str(element3), 'tuple')

    def test_that_gen_name_with_non_string_name_throws(self):
        element = 'valid element'
        name1 = None
        name2 = 1
        name3 = (3.0, 'string')

        self.exception_produced(element, name1, str(name1), 'NoneType')
        self.exception_produced(element, name2, str(name2), 'int')
        self.exception_produced(element, name3, str(name3), 'tuple')

    def test_that_gen_name_with_unicode_string_does_not_throw(self):
        element = u'element'
        label = u'label'
        result = u'element label'
        self.assertEqual(gen_name(element, label), result)


if __name__ == '__main__':
    unittest.main()
