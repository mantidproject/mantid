# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import LoadGudrunOutput, Workspace

import unittest
import tempfile


class LoadGudrunOutputTest(unittest.TestCase):

    def setUp(self):
        self.file_name = 'POLARIS00097947-min{}01'

    def test_valid_extensions(self):
        self.assertIsNotNone(LoadGudrunOutput(InputFile=self.file_name.format('.dcs'), OutputWorkspace='out_ws'))
        self.assertIsNotNone(LoadGudrunOutput(InputFile=self.file_name.format('.mdcs'), OutputWorkspace='out_ws'))
        self.assertIsNotNone(LoadGudrunOutput(InputFile=self.file_name.format('.mint'), OutputWorkspace='out_ws'))
        self.assertIsNotNone(LoadGudrunOutput(InputFile=self.file_name.format('.mdor'), OutputWorkspace='out_ws'))
        self.assertIsNotNone(LoadGudrunOutput(InputFile=self.file_name.format('.mgor'), OutputWorkspace='out_ws'))

    def test_invalid_extension(self):
        self.assertRaises(ValueError, LoadGudrunOutput, 'file.nxs', 'out_ws')

    def test_file_does_not_exist(self):
        self.assertRaises(ValueError, LoadGudrunOutput, 'file.dcs01', 'out_ws')

    def test_load_dcs(self):
        actual = LoadGudrunOutput(self.file_name.format('.dcs'))
        self.assertIsInstance(actual, Workspace)
        self.assertEqual(actual.getNumberBins(), 100)
        self.assertEqual(actual.getNumberHistograms(), 5)

    def test_load_mdsc(self):
        actual = LoadGudrunOutput(self.file_name.format('.mdcs'))
        self.assertIsInstance(actual, Workspace)
        self.assertEqual(actual.getNumberBins(), 100)
        self.assertEqual(actual.getNumberHistograms(), 1)

    def test_load_mint(self):
        actual = LoadGudrunOutput(self.file_name.format('.mint'))
        self.assertIsInstance(actual, Workspace)
        self.assertEqual(actual.getNumberBins(), 100)
        self.assertEqual(actual.getNumberHistograms(), 1)

    def test_load_mdor(self):
        actual = LoadGudrunOutput(self.file_name.format('.mdor'))
        self.assertIsInstance(actual, Workspace)
        self.assertEqual(actual.getNumberBins(), 100)
        self.assertEqual(actual.getNumberHistograms(), 1)

    def test_load_mgor(self):
        actual = LoadGudrunOutput(self.file_name.format('.mgor'))
        self.assertIsInstance(actual, Workspace)
        self.assertEqual(actual.getNumberBins(), 100)
        self.assertEqual(actual.getNumberHistograms(), 1)

    def test_one_column_data_file(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.dcs01') as tmp:
            tmp.write('1234\n2345\n3456\n4567')
            tmp.close()
            self.assertRaises(ValueError, LoadGudrunOutput, tmp.name, 'out_ws')

    def test_even_column_data_file(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.dcs01') as tmp:
            tmp.write('1234 2345 3456 4567\n1234 2345 3456 4567')
            tmp.close()
            self.assertRaises(ValueError, LoadGudrunOutput, tmp.name, 'out_ws')

    def test_rename_with_default_output(self):
        actual = LoadGudrunOutput(InputFile=self.file_name.format('.dcs'), OutputWorkspace='')
        self.assertEqual(actual.name(), 'POLARIS00097947-dcs01')

    def test_name_when_given(self):
        actual = LoadGudrunOutput(InputFile=self.file_name.format('.dcs'), OutputWorkspace='actual')
        self.assertEqual(actual.name(), 'actual')


if __name__ == '__main__':
    unittest.main()
