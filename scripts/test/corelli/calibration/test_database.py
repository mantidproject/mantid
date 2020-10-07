
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from os import remove
import unittest
import numpy as np
import pathlib
from datetime import datetime
import tempfile

from mantid.dataobjects import TableWorkspace
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateEmptyTableWorkspace
from corelli.calibration.database import combine_spatial_banks, init_corelli_table, save_bank_table, filename_bank_table,\
    load_bank_table, has_valid_columns, save_manifest_file


class TestCorelliDatabase(unittest.TestCase):

    test_dir = tempfile.TemporaryDirectory('_data_corelli')

    def setUp(self) -> None:

        # create a mock database
        # tests save_bank_table and load_bank_table, save_manifest
        self.database_path:str = TestCorelliDatabase.test_dir.name

        calibratedWS10 = init_corelli_table()
        calibratedWS10.addRow( [28672,[2.291367950578997, -1.2497636826045173, -0.7778867990739283]])
        calibratedWS10.addRow( [28673,[2.291367950578997, -1.2462425728938251, -0.7778867990739283]])
        calibratedWS10.addRow( [28674,[2.291367950578997, -1.2427213977528369, -0.7778867990739283]])
        calibratedWS10.addRow( [28675,[2.291367950578997, -1.2392001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS10, 10, self.database_path)

        calibratedWS20 = init_corelli_table()
        calibratedWS20.addRow( [28700,[2.291367950578997, -1.1511478720770645, -0.7778867990739283]])
        calibratedWS20.addRow( [28701,[2.291367950578997, -1.1476249296284657, -0.7778867990739283]])
        calibratedWS20.addRow( [28702,[2.291367950578997, -1.2427213977528369, -0.7778867990739283]])
        calibratedWS20.addRow( [28703,[2.291367950578997, -1.2392001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS20, 20, self.database_path)

        # placeholder to read from the database
        self.ws_group = WorkspaceGroup()
        calibratedWS10 = load_bank_table( 10, self.database_path)
        self.ws_group.addWorkspace(calibratedWS10)

        calibratedWS20 = load_bank_table( 20, self.database_path)
        self.ws_group.addWorkspace(calibratedWS20)

    def test_init_corelli_table(self):

        corelli_table = init_corelli_table()
        assert isinstance(corelli_table, TableWorkspace)

    def test_has_valid_columns(self):

        corelli_table = init_corelli_table()
        self.assertEqual(has_valid_columns(corelli_table), True)

        table_incomplete:TableWorkspace = CreateEmptyTableWorkspace()
        table_incomplete.addColumn(type="int", name="Detector ID")
        self.assertEqual(has_valid_columns(table_incomplete), False)

    def test_filename_bank_table(self):

        abs_subdir:str = self.database_path + '/bank001/'
        time:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD

        # calibration
        expected_filename = str(pathlib.Path(abs_subdir + '/calibration_corelli_bank001_' + time + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, 'calibration')
        self.assertEqual(filename, expected_filename)

        # mask
        expected_filename = str(pathlib.Path(abs_subdir + '/mask_corelli_bank001_' + time + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, 'mask')
        self.assertEqual(filename, expected_filename)

        # acceptance
        expected_filename = str(pathlib.Path(abs_subdir + '/acceptance_corelli_bank001_' + time + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, 'acceptance')
        self.assertEqual(filename, expected_filename)

        # verify assertion is raised for invalid name
        with self.assertRaises( AssertionError ) as ar:
            filename_bank_table(1, self.database_path, 'wrong')

        self.assertEqual( 'table_type must be calibration, mask or acceptance' in str(ar.exception), True)

    def test_combine_spatial_banks(self):

        combined_table = combine_spatial_banks(self.ws_group)
        combined_dict = combined_table.toDict()

        expected_dict = {'Detector ID': [28672, 28673, 28674, 28675, 28700, 28701, 28702, 28703],
                         'Detector Position': [  np.array([2.29136795, -1.24976368, -0.7778868]),
                                                 np.array([2.29136795, -1.24624257, -0.7778868]),
                                                 np.array([2.29136795, -1.2427214,  -0.7778868]),
                                                 np.array([2.29136795, -1.23920016, -0.7778868]),
                                                 np.array([2.29136795, -1.15114787, -0.7778868]),
                                                 np.array([2.29136795, -1.14762493, -0.7778868]),
                                                 np.array([2.29136795, -1.2427214,  -0.7778868]),
                                                 np.array([2.29136795, -1.23920016, -0.7778868])
                                                 ]
                         }

        self.assertEqual(expected_dict['Detector ID'], combined_dict['Detector ID'] )

        for i,expected_array in enumerate(expected_dict['Detector Position']):
            self.assertAlmostEqual( expected_array.all(), combined_dict['Detector Position'][i].all() )

    def test_save_manifest_file(self):

        time:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        filename = self.database_path + '/manifest_corelli_' + time + '.csv'

        # writing
        ts = datetime.now().replace(microsecond=0).isoformat()
        save_manifest_file( [10,11], self.database_path )
        self.assertTrue(pathlib.Path(filename).is_file())

        file_contents = pathlib.Path(filename).read_text() # safe one liner
        expected_manifest = f'bankID, timestamp\n10, {ts}\n11, {ts}\n'
        self.assertEqual(file_contents, expected_manifest)

        # appending
        ts = datetime.now().replace(microsecond=0).isoformat()
        save_manifest_file( [12], self.database_path )
        self.assertTrue(pathlib.Path(filename).is_file())

        file_contents = pathlib.Path(filename).read_text() # safe one liner
        expected_manifest = f'bankID, timestamp\n10, {ts}\n11, {ts}\n12, {ts}\n'
        self.assertEqual(file_contents, expected_manifest)

        remove(filename)

    def tearDown(self) -> None:

        remove(filename_bank_table(10, self.database_path))
        remove(filename_bank_table(20, self.database_path))
        TestCorelliDatabase.test_dir.cleanup()


if __name__ == "__main__":
    unittest.main()
