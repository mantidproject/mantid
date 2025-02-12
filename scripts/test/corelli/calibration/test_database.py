# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from contextlib import contextmanager
from datetime import datetime
import numpy as np
from numpy.testing import assert_allclose
from os import path, remove
import pathlib
import shutil
import tempfile
from typing import List
import unittest

from mantid import AnalysisDataService, config
from mantid.api import mtd, WorkspaceGroup
from mantid.dataobjects import MaskWorkspace, TableWorkspace
from mantid.simpleapi import CreateEmptyTableWorkspace, CreateSampleWorkspace, DeleteWorkspace, DeleteWorkspaces, LoadNexusProcessed

from corelli.calibration.database import (
    combine_spatial_banks,
    combine_temporal_banks,
    day_stamp,
    filename_bank_table,
    has_valid_columns,
    init_corelli_table,
    load_bank_table,
    load_calibration_set,
    new_corelli_calibration,
    save_bank_table,
    save_calibration_set,
    save_manifest_file,
    _table_to_workspace,
    verify_date_format,
)
from corelli.calibration.bank import calibrate_banks


class TestCorelliDatabase(unittest.TestCase):
    test_dir = tempfile.TemporaryDirectory("_data_corelli")

    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the tests cases for calibrate_bank, consisting of data for only one bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        """
        config.appendDataSearchSubDir("CORELLI/calibration")
        for directory in config.getDataSearchDirs():
            if "UnitTest" in directory:
                data_dir = path.join(directory, "CORELLI", "calibration")
                break
        cls.workspaces_temporary = list()
        cls.cases = dict()
        for bank_case in ("124016_bank10", "123454_bank58", "124023_bank10", "124023_banks_14_15"):
            workspace = "CORELLI_" + bank_case
            LoadNexusProcessed(Filename=path.join(data_dir, workspace + ".nxs"), OutputWorkspace=workspace)
            cls.cases[bank_case] = workspace
            cls.workspaces_temporary.append(workspace)

    def setUp(self) -> None:
        # create a mock database
        # tests save_bank_table and load_bank_table, save_manifest
        self.database_path: str = TestCorelliDatabase.test_dir.name
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD

        calibrated_ws = init_corelli_table()
        calibrated_ws.addRow([28672, -1.2497636826045173])
        calibrated_ws.addRow([28673, -1.2462425728938251])
        calibrated_ws.addRow([28674, -1.2427213977528369])
        calibrated_ws.addRow([28675, -1.2392001571797284])
        save_bank_table(calibrated_ws, 10, self.database_path, date)

        calibrated_ws = init_corelli_table()
        calibrated_ws.addRow([28676, -1.2597636826045173])
        calibrated_ws.addRow([28677, -1.2562425728938251])
        calibrated_ws.addRow([28678, -1.2527213977528369])
        calibrated_ws.addRow([28679, -1.2492001571797284])
        save_bank_table(calibrated_ws, 20, self.database_path, date)

        calibrated_ws = init_corelli_table()
        calibrated_ws.addRow([28700, -1.1511478720770645])
        calibrated_ws.addRow([28701, -1.1476249296284657])
        calibrated_ws.addRow([28702, -1.2427213977528369])
        calibrated_ws.addRow([28703, -1.2392001571797284])
        save_bank_table(calibrated_ws, 30, self.database_path, date)

        calibrated_ws = init_corelli_table()
        calibrated_ws.addRow([28704, -1.1611478720770645])
        calibrated_ws.addRow([28705, -1.1776249296284657])
        calibrated_ws.addRow([28706, -1.2827213977528369])
        calibrated_ws.addRow([28707, -1.2992001571797284])
        save_bank_table(calibrated_ws, 40, self.database_path, "20200601")  # use different date

        calibrated_ws = init_corelli_table("calibration_" + str(40))
        calibrated_ws.addRow([28704, -1.1711478720770645])
        calibrated_ws.addRow([28705, -1.1876249296284657])
        calibrated_ws.addRow([28706, -1.2927213977528369])
        calibrated_ws.addRow([28707, -1.3092001571797284])
        save_bank_table(calibrated_ws, 40, self.database_path, "20200101")  # use different date

        # placeholder to read from the database
        self.ws_group = WorkspaceGroup()
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD
        self.ws_group.addWorkspace(load_bank_table(10, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table(20, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table(30, self.database_path, date))
        self.ws_group.addWorkspace(load_bank_table(40, self.database_path, "20200601"))

    def test_init_corelli_table(self):
        corelli_table = init_corelli_table()
        assert isinstance(corelli_table, TableWorkspace)

    def test_has_valid_columns(self):
        corelli_table = init_corelli_table()
        self.assertEqual(has_valid_columns(corelli_table), True)

        table_incomplete: TableWorkspace = CreateEmptyTableWorkspace()
        table_incomplete.addColumn(type="int", name="Detector ID")
        self.assertEqual(has_valid_columns(table_incomplete), False)

    def test_filename_bank_table(self):
        abs_subdir: str = self.database_path + "/bank001/"
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD

        # calibration
        expected_filename = str(pathlib.Path(abs_subdir + "/calibration_corelli_bank001_" + date + ".nxs.h5").resolve())
        filename = filename_bank_table(1, self.database_path, date, "calibration")
        self.assertEqual(filename, expected_filename)

        # mask
        expected_filename = str(pathlib.Path(abs_subdir + "/mask_corelli_bank001_" + date + ".nxs.h5").resolve())
        filename = filename_bank_table(1, self.database_path, date, "mask")
        self.assertEqual(filename, expected_filename)

        # fit
        expected_filename = str(pathlib.Path(abs_subdir + "/fit_corelli_bank001_" + date + ".nxs.h5").resolve())
        filename = filename_bank_table(1, self.database_path, date, "fit")
        self.assertEqual(filename, expected_filename)

        # verify assertion is raised for invalid name
        with self.assertRaises(AssertionError) as ar:
            filename_bank_table(1, self.database_path, date, "wrong")

        self.assertEqual("wrong is not a valid table type" in str(ar.exception), True)

    def test_combine_spatial_banks(self):
        # test with name
        combined_table = combine_spatial_banks(self.ws_group, name="calibrated_banks")
        self.assertTrue(combined_table.getName() == "calibrated_banks")

        # test without name
        combined_table = combine_spatial_banks(self.ws_group)
        combined_dict = combined_table.toDict()

        expected_dict = {
            "Detector ID": [
                28672,
                28673,
                28674,
                28675,
                28676,
                28677,
                28678,
                28679,
                28700,
                28701,
                28702,
                28703,
                28704,
                28705,
                28706,
                28707,
            ],
            "Detector Y Coordinate": [
                -1.24976368,
                -1.24624257,
                -1.2427214,
                -1.23920016,
                -1.25976368,
                -1.25624257,
                -1.25272140,
                -1.24920016,
                -1.15114787,
                -1.14762493,
                -1.2427214,
                -1.23920016,
                -1.16114787,
                -1.17762493,
                -1.28272140,
                -1.29920016,
            ],
        }

        self.assertEqual(expected_dict["Detector ID"], combined_dict["Detector ID"])

        for i, expected_array in enumerate(expected_dict["Detector Y Coordinate"]):
            self.assertAlmostEqual(expected_array, combined_dict["Detector Y Coordinate"][i])

    def test_save_manifest_file(self):
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD
        filename = self.database_path + "/manifest_corelli_" + date + ".csv"

        # writing
        save_manifest_file(self.database_path, [10, 11], [date, date])
        self.assertTrue(pathlib.Path(filename).is_file())

        file_contents = pathlib.Path(filename).read_text()  # safe one liner
        expected_manifest = f"bankID, timestamp\n10, {date}\n11, {date}\n"
        self.assertEqual(file_contents, expected_manifest)

        remove(filename)

    def test_day_stamp(self) -> None:
        self.assertEqual(day_stamp(self.cases["124016_bank10"]), 20200106)
        self.assertEqual(day_stamp(self.cases["124023_bank10"]), 20200109)
        self.assertEqual(day_stamp(self.cases["123454_bank58"]), 20200103)
        self.assertEqual(day_stamp(self.cases["124023_banks_14_15"]), 20200109)

    def test_save_calibration_set(self) -> None:
        calibrations, masks = calibrate_banks(self.cases["124023_banks_14_15"], "14-15")
        for w in ("calibrations", "masks", "fits"):
            assert AnalysisDataService.doesExist(w)

        # Save everything (typical case)
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, "calibrations", "masks", "fits")
        for bn in ("014", "015"):  # bank number
            for ct in ("calibration", "mask", "fit"):  # table type
                assert path.exists(path.join(database.name, f"bank{bn}", f"{ct}_corelli_bank{bn}_20200109.nxs.h5"))
        database.cleanup()

        #  Save only the calibration tables
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, "calibrations")
        for bn in ("014", "015"):  # bank number
            assert path.exists(path.join(database.name, f"bank{bn}", f"calibration_corelli_bank{bn}_20200109.nxs.h5"))
        database.cleanup()

        #  Save only the calibration tables as a list of strings
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, ["calib14", "calib15"])
        for bn in ("014", "015"):  # bank number
            assert path.exists(path.join(database.name, f"bank{bn}", f"calibration_corelli_bank{bn}_20200109.nxs.h5"))
        database.cleanup()

        #  Save only the calibration tables as a list of workspaces
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, [mtd["calib14"], mtd["calib15"]])
        for bn in ("014", "015"):  # bank number
            assert path.exists(path.join(database.name, f"bank{bn}", f"calibration_corelli_bank{bn}_20200109.nxs.h5"))
        database.cleanup()

        # Save only one table of each type, passing strings
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, "calib14", "mask14", "fit14")
        for ct in ("calibration", "mask", "fit"):  # table type
            assert path.exists(path.join(database.name, "bank014", f"{ct}_corelli_bank014_20200109.nxs.h5"))
        database.cleanup()

        # Save only one table of each type, passing workspaces
        database = tempfile.TemporaryDirectory()
        save_calibration_set(self.cases["124023_banks_14_15"], database.name, mtd["calib14"], mtd["mask14"], mtd["fit14"])
        for ct in ("calibration", "mask", "fit"):  # table type
            assert path.exists(path.join(database.name, "bank014", f"{ct}_corelli_bank014_20200109.nxs.h5"))
        database.cleanup()

    def test_verify_date_format(self) -> None:
        # success
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD
        verify_date_format("test_verify_date_format", date)

        # failure
        # verify assertion is raised for invalid date format
        with self.assertRaises(ValueError) as ar:
            verify_date_format("test_verify_date_format", "120711")

        self.assertEqual("date in function test_verify_date_format" in str(ar.exception), True)

        with self.assertRaises(ValueError) as ar:
            verify_date_format("test_verify_date_format", "XX220101")

        self.assertEqual("date in function test_verify_date_format" in str(ar.exception), True)

    def test_combine_temporal_banks(self) -> None:
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD
        group_ws, bank_stamps = combine_temporal_banks(self.database_path, date)
        self.assertEqual([bs[0] for bs in bank_stamps], [10, 20, 30, 40])  # bank numbers
        self.assertEqual([bs[1] for bs in bank_stamps], [int(date), int(date), int(date), 20200601])  # day-stamps

        # list of expected group_ws components as dictionaries
        expected = []
        expected.append(
            {"Detector ID": [28672, 28673, 28674, 28675], "Detector Y Coordinate": [-1.24976368, -1.24624257, -1.24272140, -1.23920016]}
        )

        expected.append(
            {"Detector ID": [28676, 28677, 28678, 28679], "Detector Y Coordinate": [-1.25976368, -1.25624257, -1.25272140, -1.24920016]}
        )

        expected.append(
            {"Detector ID": [28700, 28701, 28702, 28703], "Detector Y Coordinate": [-1.15114787, -1.14762493, -1.24272140, -1.23920016]}
        )
        # bank 40 from '20200601'
        expected.append(
            {"Detector ID": [28704, 28705, 28706, 28707], "Detector Y Coordinate": [-1.16114787, -1.17762493, -1.28272140, -1.29920016]}
        )

        for b, ws in enumerate(group_ws):
            table_dict = ws.toDict()
            self.assertEqual(expected[b]["Detector ID"], table_dict["Detector ID"])

            for i, expected_array in enumerate(expected[b]["Detector Y Coordinate"]):
                self.assertAlmostEqual(expected_array, table_dict["Detector Y Coordinate"][i])

    def test_new_corelli_calibration_and_load_calibration(self):
        r"""Creating a database is time consuming, thus we test both new_corelli_calibration and load_calibration"""
        # populate a calibration database with a few cases. There should be at least one bank with two calibrations
        database = tempfile.TemporaryDirectory()
        cases = [("124016_bank10", "10"), ("124023_bank10", "10"), ("124023_banks_14_15", "14-15")]
        for bank_case, bank_selection in cases:
            # Produce workspace groups 'calibrations', 'masks', 'fits'
            calibrate_banks(self.cases[bank_case], bank_selection)
            masks = "masks" if AnalysisDataService.doesExist("masks") else None
            save_calibration_set(self.cases[bank_case], database.name, "calibrations", masks, "fits")
            DeleteWorkspaces(["calibrations", "fits"])
            if AnalysisDataService.doesExist("masks"):
                DeleteWorkspaces(["masks"])

        # invoque creation of  new corelli calibration without a date
        calibration_file, mask_file, manifest_file = new_corelli_calibration(database.name)
        for file_path in (calibration_file, mask_file, manifest_file):
            assert path.exists(file_path)
        assert open(manifest_file).read() == "bankID, timestamp\n10, 20200109\n14, 20200109\n15, 20200109\n"

        # load latest calibration and mask (day-stamp of '124023_bank10' is 20200109)
        calibration, mask = load_calibration_set(self.cases["124023_bank10"], database.name, mask_format="TableWorkspace")
        calibration_expected = LoadNexusProcessed(Filename=calibration_file)
        mask_expected = LoadNexusProcessed(Filename=mask_file)
        assert_allclose(calibration.column(1), calibration_expected.column(1), atol=1e-4)
        assert mask.column(0) == mask_expected.column(0)

        # invoque a new corelli calibration with a date falling in between the bank (bank10) in
        # in our small dataset having two calibrations
        calibration_file, mask_file, manifest_file = new_corelli_calibration(database.name, date="20200108")
        for file_path in (calibration_file, mask_file, manifest_file):
            assert path.exists(file_path)
        assert open(manifest_file).read() == "bankID, timestamp\n10, 20200106\n"

        # load oldest calibration and mask(day-stamp of '124023_bank10' is 20200106)
        calibration, mask = load_calibration_set(self.cases["124016_bank10"], database.name, mask_format="TableWorkspace")
        calibration_expected = LoadNexusProcessed(Filename=calibration_file)
        mask_expected = LoadNexusProcessed(Filename=mask_file)
        assert_allclose(calibration.column(1), calibration_expected.column(1), atol=1e-4)
        assert mask.column(0) == mask_expected.column(0)

        database.cleanup()

    def test_table_to_workspace(self) -> None:
        r"""Test the conversion of a TableWorkspace containing the masked detector ID's to a MaskWorkspace object"""
        output_workspace = "test_table_to_workspace_masked"
        # Have a fake mask table, masking bank 42
        mask_table = CreateEmptyTableWorkspace(OutputWorkspace=output_workspace)
        mask_table.addColumn(type="int", name="Detector ID")
        begin, end = 167936, 172030  # # Bank 42 has detector ID's from 167936 to 172030
        for detector_id in range(begin, 1 + end):
            mask_table.addRow([detector_id])
        # Convert to MaskWorkspace
        mask_table = _table_to_workspace(mask_table)
        # Check the output workspace is of type MaskWorkspace
        assert isinstance(mask_table, MaskWorkspace)
        # Check the output workspace has 1 on workspace indexes for bank 42, and 0 elsewhere
        mask_flags = mask_table.extractY().flatten()
        offset = 3  # due to the detector monitors, workspace_index = detector_id + offset
        masked_workspace_indexes = slice(begin + offset, 1 + end + offset)
        assert np.all(mask_flags[masked_workspace_indexes])  # all values are 1
        mask_flags = np.delete(mask_flags, masked_workspace_indexes)
        assert not np.any(mask_flags)  # no value is 1
        DeleteWorkspace(output_workspace)

    def test_load_calibration_set(self) -> None:
        r"""
        1. create an empty "database"
          1.1. create a workspace with a particular daystamp
          1.2. try to find a file in the database
        2. create a database with only one calibration file with daystamp 20200601
          1.1 create a workspace with the following daystamps and see in which cases the calibration file is loaded
              20200101, 20200601, 20201201
        3. create a database with two calibration files with day-stamsp 20200401 and 20200801
          3.1 create a workspace with the following day-stamps and see which (in any) calibration is selected
             20200101, 20200401, 20200601, 20200801, 20201201
        """

        @contextmanager
        def mock_database(day_stamps: List[int]):
            r"""create a database with mock calibration files"""
            dir_path = tempfile.mkdtemp()
            path = pathlib.Path(dir_path)
            for daystamp in day_stamps:
                file_path = path / f"calibration_corelli_{daystamp}.nxs.h5"
                with open(str(file_path), "w") as fp:
                    fp.write("mock")
            try:
                yield dir_path
            finally:
                shutil.rmtree(dir_path)

        def set_daystamp(input_workspace: str, daystamp: int):
            r"""Update the run_start log entry of a workspace
            :param input_workspace: handle to a workspace (not its name!)
            :param daystamp: 8-digit integer
            """
            x = str(daystamp)
            run_start = f"{x[0:4]}-{x[4:6]}-{x[6:]}T20:54:07.265105667"
            run = input_workspace.getRun()
            run.addProperty(name="run_start", value=run_start, replace=True)

        workspace = CreateSampleWorkspace(OutputWorkspace="test_load_calibration_set")
        set_daystamp(workspace, 20200101)

        # empty calibration database (corner case)
        with mock_database([]) as database_path:
            instrument_tables = load_calibration_set(workspace, database_path)
            assert list(instrument_tables) == [None, None]

        # database with only one calibration file (corner case)
        with mock_database([20200601]) as database_path:
            set_daystamp(workspace, 20200101)  # no calibration found
            assert list(load_calibration_set(workspace, database_path)) == [None, None]

            set_daystamp(workspace, 20200601)
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)  # should pick calibration 20200601
            self.assertEqual("20200601" in str(ar.exception), True)

            set_daystamp(workspace, 20201201)
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)
            self.assertEqual("calibration_corelli_20200601.nxs.h5" in str(ar.exception), True)

        # database with two calibration files (general case)
        with mock_database([20200401, 20200801]) as database_path:
            set_daystamp(workspace, "20200101")
            assert list(load_calibration_set(workspace, database_path)) == [None, None]

            set_daystamp(workspace, "20200401")
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)
            self.assertEqual("calibration_corelli_20200401.nxs.h5" in str(ar.exception), True)

            set_daystamp(workspace, "20200601")
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)
            self.assertEqual("calibration_corelli_20200401.nxs.h5" in str(ar.exception), True)

            set_daystamp(workspace, "20200801")
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)
            self.assertEqual("calibration_corelli_20200801.nxs.h5" in str(ar.exception), True)

            set_daystamp(workspace, "20201201")
            with self.assertRaises(RuntimeError) as ar:
                load_calibration_set(workspace, database_path)
            self.assertEqual("calibration_corelli_20200801.nxs.h5" in str(ar.exception), True)
        workspace.delete()

    def tearDown(self) -> None:
        date: str = datetime.now().strftime("%Y%m%d")  # format YYYYMMDD
        remove(filename_bank_table(10, self.database_path, date))
        remove(filename_bank_table(20, self.database_path, date))
        remove(filename_bank_table(30, self.database_path, date))
        remove(filename_bank_table(40, self.database_path, "20200601"))
        remove(filename_bank_table(40, self.database_path, "20200101"))
        TestCorelliDatabase.test_dir.cleanup()

    @classmethod
    def tearDownClass(cls) -> None:
        r"""Delete temporary workspaces"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)


if __name__ == "__main__":
    unittest.main()
