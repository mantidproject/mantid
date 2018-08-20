from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import LoadGudrunOutput, Workspace
import unittest
import tempfile


class LoadGudrunOutputTest(unittest.TestCase):

    def test_valid_extensions(self):
        self.assertIsNotNone(LoadGudrunOutput('file.dcs01'))
        self.assertIsNotNone(LoadGudrunOutput('file.mdcs01'))
        self.assertIsNotNone(LoadGudrunOutput('file.mint01'))
        self.assertIsNotNone(LoadGudrunOutput('file.mdor01'))
        self.assertIsNotNone(LoadGudrunOutput('file.mgor01'))

    def test_invalid_extension(self):
        self.assertRaisesRegexp(ValueError, "Unexpected file extensions \'.nxs\'. Valid extensions are: "
                                            "[\'.dcs\', \'.mdsc\', \'.mint01\', \'.mdor\', \'mgor\'",
                                LoadGudrunOutput, 'file.nxs')

    def test_file_does_not_exist(self):
        tempfile.mktemp()
        self.assertRaisesRegexp(ValueError, "Unexpected file extensions \'.nxs\'. Valid extensions are: "
                                            "[\'.dcs\', \'.mdsc\', \'.mint01\', \'.mdor\', \'mgor\'",
                                LoadGudrunOutput, 'file.nxs')

    def test_load_dcs(self):
        actual = LoadGudrunOutput('file.dcs01')
        self.assertIsInstance(actual, Workspace)

    def test_load_mdsc(self):
        actual = LoadGudrunOutput('file.mdcs01')
        self.assertIsInstance(actual, Workspace)

    def test_load_mint(self):
        actual = LoadGudrunOutput('file.mint01')
        self.assertIsInstance(actual, Workspace)

    def test_load_mdor(self):
        actual = LoadGudrunOutput('file.mdor01')
        self.assertIsInstance(actual, Workspace)

    def test_load_mgor(self):
        actual = LoadGudrunOutput('file.mgor01')
        self.assertIsInstance(actual, Workspace)

    def test_one_column_data_file(self):
        # create fake data file (1 column)
        self.fail("Not implemented")

    def test_even_column_data_file(self):
        # create fake data file (4 columns)
        self.fail("Not implemented")
