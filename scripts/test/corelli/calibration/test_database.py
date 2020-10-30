
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
    load_bank_table, has_valid_columns, save_manifest_file, verify_date_YYYYMMDD, combine_temporal_banks


class TestCorelliDatabase(unittest.TestCase):

    test_dir = tempfile.TemporaryDirectory('_data_corelli')

    def setUp(self) -> None:

        # create a mock database
        # tests save_bank_table and load_bank_table, save_manifest
        self.database_path:str = TestCorelliDatabase.test_dir.name
        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD

        calibratedWS = init_corelli_table()
        calibratedWS.addRow( [28672,[2.291367950578997, -1.2497636826045173, -0.7778867990739283]])
        calibratedWS.addRow( [28673,[2.291367950578997, -1.2462425728938251, -0.7778867990739283]])
        calibratedWS.addRow( [28674,[2.291367950578997, -1.2427213977528369, -0.7778867990739283]])
        calibratedWS.addRow( [28675,[2.291367950578997, -1.2392001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS, 10, self.database_path, date)

        calibratedWS = init_corelli_table()
        calibratedWS.addRow( [28676,[2.291367950578997, -1.2597636826045173, -0.7778867990739283]])
        calibratedWS.addRow( [28677,[2.291367950578997, -1.2562425728938251, -0.7778867990739283]])
        calibratedWS.addRow( [28678,[2.291367950578997, -1.2527213977528369, -0.7778867990739283]])
        calibratedWS.addRow( [28679,[2.291367950578997, -1.2492001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS, 20, self.database_path, date)

        calibratedWS = init_corelli_table()
        calibratedWS.addRow( [28700,[2.291367950578997, -1.1511478720770645, -0.7778867990739283]])
        calibratedWS.addRow( [28701,[2.291367950578997, -1.1476249296284657, -0.7778867990739283]])
        calibratedWS.addRow( [28702,[2.291367950578997, -1.2427213977528369, -0.7778867990739283]])
        calibratedWS.addRow( [28703,[2.291367950578997, -1.2392001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS, 30, self.database_path, date)

        calibratedWS = init_corelli_table()
        calibratedWS.addRow( [28704,[2.291367950578997, -1.1611478720770645, -0.7778867990739283]])
        calibratedWS.addRow( [28705,[2.291367950578997, -1.1776249296284657, -0.7778867990739283]])
        calibratedWS.addRow( [28706,[2.291367950578997, -1.2827213977528369, -0.7778867990739283]])
        calibratedWS.addRow( [28707,[2.291367950578997, -1.2992001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS, 40, self.database_path, '20200601') # use different date

        calibratedWS = init_corelli_table('calibration_' + str(40))
        calibratedWS.addRow( [28704,[2.291367950578997, -1.1711478720770645, -0.7778867990739283]])
        calibratedWS.addRow( [28705,[2.291367950578997, -1.1876249296284657, -0.7778867990739283]])
        calibratedWS.addRow( [28706,[2.291367950578997, -1.2927213977528369, -0.7778867990739283]])
        calibratedWS.addRow( [28707,[2.291367950578997, -1.3092001571797284, -0.7778867990739283]])
        save_bank_table( calibratedWS, 40, self.database_path, '20200101') # use different date

        # placeholder to read from the database
        self.ws_group = WorkspaceGroup()
        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        self.ws_group.addWorkspace(load_bank_table( 10, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table( 20, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table( 30, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table( 40, self.database_path, '20200601'))

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
        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD

        # calibration
        expected_filename = str(pathlib.Path(abs_subdir + '/calibration_corelli_bank001_' + date + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, date, 'calibration')
        self.assertEqual(filename, expected_filename)

        # mask
        expected_filename = str(pathlib.Path(abs_subdir + '/mask_corelli_bank001_' + date + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, date, 'mask')
        self.assertEqual(filename, expected_filename)

        # fit
        expected_filename = str(pathlib.Path(abs_subdir + '/fit_corelli_bank001_' + date + '.nxs.h5').resolve())
        filename = filename_bank_table(1, self.database_path, date, 'fit')
        self.assertEqual(filename, expected_filename)

        # verify assertion is raised for invalid name
        with self.assertRaises( AssertionError ) as ar:
            filename_bank_table(1, self.database_path, date, 'wrong')

        self.assertEqual( 'table_type must be calibration, mask or fit' in str(ar.exception), True)

    def test_combine_spatial_banks(self):

        # test with name
        combined_table = combine_spatial_banks(self.ws_group, 'calibrated_banks')
        self.assertTrue(combined_table.getName() == 'calibrated_banks')

        # test without name
        combined_table = combine_spatial_banks(self.ws_group)
        combined_dict = combined_table.toDict()

        expected_dict = {'Detector ID': [28672, 28673, 28674, 28675, 28676, 28677, 28678, 28679,
                                         28700, 28701, 28702, 28703, 28704, 28705, 28706, 28707],
                         'Detector Position': [np.array([ 2.29136795, -1.24976368, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.24624257, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.2427214 , -0.7778868 ]),
                                               np.array([ 2.29136795, -1.23920016, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.25976368, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.25624257, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.2527214 , -0.7778868 ]),
                                               np.array([ 2.29136795, -1.24920016, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.15114787, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.14762493, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.2427214 , -0.7778868 ]),
                                               np.array([ 2.29136795, -1.23920016, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.16114787, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.17762493, -0.7778868 ]),
                                               np.array([ 2.29136795, -1.2827214 , -0.7778868 ]),
                                               np.array([ 2.29136795, -1.29920016, -0.7778868 ])
                                               ]
                         }

        self.assertEqual(expected_dict['Detector ID'], combined_dict['Detector ID'] )

        for i,expected_array in enumerate(expected_dict['Detector Position']):
            self.assertAlmostEqual( expected_array.all(), combined_dict['Detector Position'][i].all() )

    def test_save_manifest_file(self):

        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        filename = self.database_path + '/manifest_corelli_' + date + '.csv'

        # writing
        save_manifest_file( [10,11], self.database_path, date)
        self.assertTrue(pathlib.Path(filename).is_file())

        file_contents = pathlib.Path(filename).read_text() # safe one liner
        expected_manifest = f'bankID, timestamp\n10, {date}\n11, {date}\n'
        self.assertEqual(file_contents, expected_manifest)

        # appending
        save_manifest_file( [12], self.database_path, date )
        self.assertTrue(pathlib.Path(filename).is_file())

        file_contents = pathlib.Path(filename).read_text() # safe one liner
        expected_manifest = f'bankID, timestamp\n10, {date}\n11, {date}\n12, {date}\n'
        self.assertEqual(file_contents, expected_manifest)

        remove(filename)

    def test_verify_date_YYYYMMDD(self)-> None:

        # success
        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        verify_date_YYYYMMDD('test_verify_date_YYYYMMDD', date)

        # failure
        # verify assertion is raised for invalid date format
        with self.assertRaises( ValueError ) as ar:
            verify_date_YYYYMMDD('test_verify_date_YYYYMMDD', '120711')

        self.assertEqual( 'date in function test_verify_date_YYYYMMDD' in str(ar.exception), True)

        with self.assertRaises( ValueError ) as ar:
            verify_date_YYYYMMDD('test_verify_date_YYYYMMDD', 'XX220101')

        self.assertEqual( 'date in function test_verify_date_YYYYMMDD' in str(ar.exception), True)

    def test_combine_temporal_banks(self) -> None:

        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        groupWS:WorkspaceGroup = combine_temporal_banks( self.database_path, date )

        # list of expected groupWS components as dictionaries
        expected = []
        expected.append( {'Detector ID': [28672, 28673, 28674, 28675],
                          'Detector Position':
                          [np.array([ 2.29136795, -1.24976368, -0.7778868 ]),
                           np.array([ 2.29136795, -1.24624257, -0.7778868 ]),
                           np.array([ 2.29136795, -1.2427214 , -0.7778868 ]),
                           np.array([ 2.29136795, -1.23920016, -0.7778868 ])
                           ]
                          } )

        expected.append( {'Detector ID': [28676, 28677, 28678, 28679],
                          'Detector Position': [
                              np.array([ 2.29136795, -1.25976368, -0.7778868 ]),
                              np.array([ 2.29136795, -1.25624257, -0.7778868 ]),
                              np.array([ 2.29136795, -1.2527214 , -0.7778868 ]),
                              np.array([ 2.29136795, -1.24920016, -0.7778868 ]) ]
                          }
                         )

        expected.append( {'Detector ID': [28700, 28701, 28702, 28703],
                          'Detector Position': [
                              np.array([ 2.29136795, -1.15114787, -0.7778868 ]),
                              np.array([ 2.29136795, -1.14762493, -0.7778868 ]),
                              np.array([ 2.29136795, -1.2427214 , -0.7778868 ]),
                              np.array([ 2.29136795, -1.23920016, -0.7778868 ])]
                          }
                         )
        # bank 40 from '20200601'
        expected.append( {'Detector ID': [28704, 28705, 28706, 28707],
                          'Detector Position': [
                              np.array([ 2.29136795, -1.16114787, -0.7778868 ]),
                              np.array([ 2.29136795, -1.17762493, -0.7778868 ]),
                              np.array([ 2.29136795, -1.2827214 , -0.7778868 ]),
                              np.array([ 2.29136795, -1.29920016, -0.7778868 ])]})

        for b, ws in enumerate(groupWS):

            table_dict = ws.toDict()
            self.assertEqual(expected[b]['Detector ID'], table_dict['Detector ID'] )

            for i,expected_array in enumerate(expected[b]['Detector Position']):
                self.assertAlmostEqual( expected_array.all(), table_dict['Detector Position'][i].all() )

    def tearDown(self) -> None:

        date:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
        remove(filename_bank_table(10, self.database_path, date))
        remove(filename_bank_table(20, self.database_path, date))
        remove(filename_bank_table(30, self.database_path, date))
        remove(filename_bank_table(40, self.database_path, '20200601'))
        remove(filename_bank_table(40, self.database_path, '20200101'))
        TestCorelliDatabase.test_dir.cleanup()


if __name__ == "__main__":
    unittest.main()
