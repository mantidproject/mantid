# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MultipleFileProperty, mtd
from testhelpers import create_algorithm

class MultipleFilePropertyTest(unittest.TestCase):

    def tearDown(self):
        """ Cleanup after test """
        del mtd['w']

    def test_value_member_returns_python_str_for_single_file(self):
        algorithm = create_algorithm('Load', Filename='LOQ48127.raw',OutputWorkspace='w',
                                  SpectrumMin=1,SpectrumMax=1,child=True)
        prop = algorithm.getProperty("Filename")
        filenames = prop.value
        self.assertTrue(isinstance(filenames, list))
        self.assertTrue(isinstance(filenames[0], str))
        self.assertEqual(len(filenames), 1)

    def test_value_member_returns_python_list_for_multiple_files(self):
        algorithm = create_algorithm('Load', Filename='MUSR15189,15190,15191.nxs',OutputWorkspace='w',
                                  SpectrumMin=1,SpectrumMax=1,child=True)
        prop = algorithm.getProperty("Filename")
        filenames = prop.value
        self.assertTrue(isinstance(filenames, list))
        self.assertEqual(len(filenames), 3)

    def test_value_member_returns_nested_python_list_for_summed_files(self):
        algorithm = create_algorithm('Load', Filename='MUSR15189,15190+15191.nxs',OutputWorkspace='w',
                                  SpectrumMin=1,SpectrumMax=1,child=True)
        prop = algorithm.getProperty("Filename")
        filenames = prop.value
        self.assertTrue(isinstance(filenames, list))
        self.assertTrue(isinstance(filenames[0], str))
        self.assertEqual(len(filenames[1]), 2)

if __name__ == '__main__':
    unittest.main()
