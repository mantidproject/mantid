# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# File that defines functionality for Corelli calibration database

from datetime import datetime
import enum
import pathlib
import re
from typing import List, Optional, Tuple, Union

from mantid.dataobjects import EventWorkspace, MaskWorkspace, TableWorkspace, Workspace2D
from mantid.api import mtd, Workspace, WorkspaceGroup
from mantid.kernel import logger
from mantid.simpleapi import (
    ClearMaskFlag,
    CreateEmptyTableWorkspace,
    ExtractMask,
    LoadEmptyInstrument,
    LoadNexusProcessed,
    MaskDetectors,
    SaveNexusProcessed,
)

# Functions exposed to the general user (public) API
__all__ = ["day_stamp", "load_calibration_set", "new_corelli_calibration", "save_calibration_set"]

# Custom type aliases
CalibrationInputSetTypes = Union[str, TableWorkspace, Workspace2D, List[TableWorkspace], List[str], WorkspaceGroup]
InputWorkspaceTypes = Union[str, Workspace2D, EventWorkspace]


class TableType(enum.Enum):
    r"""Allowed types of calibration-related tables to be saved and/or loaded"""

    CALIBRATION = "calibration"
    FIT = "fit"
    MASK = "mask"

    @classmethod
    def assert_valid_type(cls, query_type: str):
        r"""
        Assert if a query type is a valid table type, one of 'calibration', 'fit', 'mask'
        :param query_type: input query type.
        :raises: AssertionError
        """
        assert query_type in [table_type.value for table_type in list(cls)], f"{query_type} is not a valid table type"


def init_corelli_table(name: Optional[str] = None, table_type="calibration") -> TableWorkspace:
    """
    Function that initializes a Corelli calibration TableWorkspace columns
    """
    table: TableWorkspace = CreateEmptyTableWorkspace(OutputWorkspace=name) if name else CreateEmptyTableWorkspace()

    # expected columns declarations (column_type, column_name) for each type of calibration table
    column_declarations = {"calibration": [("int", "Detector ID"), ("double", "Detector Y Coordinate")], "mask": [("int", "Detector ID")]}
    for column_type, colum_name in column_declarations[table_type]:
        table.addColumn(type=column_type, name=colum_name)
    return table


def has_valid_columns(table: TableWorkspace, table_type="calibration") -> bool:
    """
    Check if table column names are valid Corelli calibration tables
    """
    names_expected = {"calibration": ["Detector ID", "Detector Y Coordinate"], "mask": ["Detector ID"]}
    names = table.getColumnNames()
    if names != names_expected[table_type]:
        return False

    return True


def filename_bank_table(bank_id: int, database_path: str, date: str, table_type: str = "calibration") -> str:
    """
    Function that returns the absolute path for a bank file using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bank_id bank number that is calibrated
    :param date format YYYYMMDD
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param table_type: 'calibration', 'mask' or 'fit'
    """
    TableType.assert_valid_type(table_type)
    verify_date_format("filename_bank_table", date)
    subdirectory: str = database_path + "/" + "bank" + str(bank_id).zfill(3)
    pathlib.Path(subdirectory).mkdir(parents=True, exist_ok=True)

    filename = subdirectory + "/" + table_type + "_corelli_bank" + str(bank_id).zfill(3) + "_" + date + ".nxs.h5"
    # make it portable
    filename = str(pathlib.Path(filename).resolve())
    return filename


def save_manifest_file(
    database_path: str,
    bank_ids: List[Union[int, str]],
    day_stamps: List[Union[int, str]],
    manifest_day_stamp: Optional[Union[int, str]] = None,
) -> str:
    """
    Function that saves a manifest_corelli_date.csv file.
    There is one file stored for each time a new new correlli calibration is invoked. The file
    consists of two columns: bank_id, timestamp (ISO format)

    :param database_path: location of the corelli database (absolute or relative)
    :param bank_ids: input bank numbers that have been calibrated, as a list of `int` or `str`
    :param day_stamps: day stamp for each of the calibrated banks in YYYMMDD format, either as a `int` or `str`
    :param manifest_day_stamp: day stamp, in YYYYMMDD format, to bear on the manifest file name, either
        as an `int` or `str`. if `None`,  the last day stamp of list `day_stamps` is selected

    :return absolute path of the manifest file
    """
    # verify day_stamps are correct
    [verify_date_format("save_manifest_file", date) for date in day_stamps]
    if manifest_day_stamp is None:
        manifest_day_stamp = str(sorted(day_stamps)[-1])
    verify_date_format("save_manifest_file", manifest_day_stamp)

    filename = database_path + "/manifest_corelli_" + str(manifest_day_stamp) + ".csv"

    file = open(filename, "w")
    file.write("bankID, timestamp\n")  # header

    lines: str = ""
    for bank_id, date in zip(bank_ids, day_stamps):
        lines += str(bank_id) + ", " + str(date) + "\n"

    file.write(lines)
    file.close()
    return filename


def save_bank_table(data: Workspace, bank_id: int, database_path: str, date: str, table_type: str = "calibration") -> None:
    """
    Function that saves a bank calibrated TableWorkspace into a single HDF5 file
    using corelli format and using current date:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs.h5
    :param data input Workspace (TableWorkspace) data for calibrated pixels
    :param bank_id bank number that is calibrated
    :param date format YYYYMMDD
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param table_type 'calibration', 'mask' or 'fit'
    """
    verify_date_format("save_bank_table", date)
    filename: str = filename_bank_table(bank_id, database_path, date, table_type)
    SaveNexusProcessed(data, filename)


def save_calibration_set(
    input_workspace: InputWorkspaceTypes,
    database_path: str,
    calibrations: CalibrationInputSetTypes,
    masks: Optional[CalibrationInputSetTypes] = None,
    fits: Optional[CalibrationInputSetTypes] = None,
) -> None:
    r"""
    Save one or more calibration workspaces to the database.

    The calibration date is picked up from the run start time stored in the metadata of the input workspace.
    The input 'calibrations', 'masks' and 'fits' will typically results from invoking `bank.calibrate_banks`
    on a wire-scan run.

    Example:
    Assume we have:
    - workspace 'integrated_counts' containing metadata `run_start` with value 20201122
    - directory '/tmp/temp_database'
    - workspace group 'calibrations' with member tables 'bank10', 'bank11', and 'bank12'
    - workspace group 'masks' with member tables 'mask10' and 'mask12'
    - workspace group 'fits' with member workspaces 'fit10', 'fit11', and 'fit12'
    Then save_calibration_set(integrated_counts, temp_database, calibrations, masks, fits) saves the following files:
    - /tmp/temp_database/bank010/calibration_corelli_bank010_20201122.nxs.h5
    - /tmp/temp_database/bank010/mask_corelli_bank010_20201122.nxs.h5
    - /tmp/temp_database/bank010/fit_corelli_bank010_20201122.nxs.h5
    - /tmp/temp_database/bank011/calibration_corelli_bank011_20201122.nxs.h5
    - /tmp/temp_database/bank011/fit_corelli_bank011_20201122.nxs.h5
    - /tmp/temp_database/bank012/calibration_corelli_bank012_20201122.nxs.h5
    - /tmp/temp_database/bank012/mask_corelli_bank012_20201122.nxs.h5
    - /tmp/temp_database/bank012/fit_corelli_bank012_20201122.nxs.h5

    :param input_workspace: workspace from which the day-stamp will be retrieved.
    :param database_path: absolute path where the calibration files will be saved
    :param calibrations: one or more bank calibration tables. If more than one, a list or a `WorkspaceGroup` are
        acceptable
    :param masks: one or more bank calibration tables. If more than one, a list or a `WorkspaceGroup` are
        acceptable
    :param fits: one or more bank calibration tables. If more than one, a list or a `WorkspaceGroup` are
        acceptable


    """
    date = str(day_stamp(input_workspace))

    def extract_bank_number(input_workspace: str) -> int:
        r"""Extract bank number from a string such as calib10, mask56, fit87"""
        return int(re.match(r"[a-zA-Z_]+(\d+)", input_workspace).groups()[0])

    def workspace_group(input_set) -> Optional[WorkspaceGroup]:
        r"""determine if the input_set corresponds to one WorkspaceGroup, and return it if it is so"""
        if isinstance(input_set, WorkspaceGroup):
            return input_set
        if isinstance(input_set, str) and isinstance(mtd[input_set], WorkspaceGroup):
            return mtd[input_set]
        return None

    def set_to_list(input_set: Optional[CalibrationInputSetTypes]) -> List[Tuple[str, int]]:
        r"""Extract the number for every workspace in the input set"""
        if input_set is None:
            return list()
        # the input set contains more than one workspace. Either as a list or a workspace group
        if isinstance(input_set, list):  # a list of workspaces
            return [(str(w), extract_bank_number(str(w))) for w in input_set]
        if workspace_group(input_set) is not None:
            return [(str(w), extract_bank_number(str(w))) for w in workspace_group(input_set)]
        # the input set contains only one workspace
        return [
            (str(input_set), extract_bank_number(str(input_set))),
        ]

    for input_set, table_type in [(calibrations, "calibration"), (masks, "mask"), (fits, "fit")]:
        if input_set is None:  # case of no 'masks' or no 'fits'
            continue
        for workspace, bank_number in set_to_list(input_set):
            save_bank_table(workspace, bank_number, database_path, date, table_type=table_type)


def load_bank_table(bank_id: int, database_path: str, date: str, table_type: str = "calibration") -> TableWorkspace:
    """
    Function that loads the latest bank calibrated TableWorkspace from a single HDF5 file
    using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bank_id bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param date current day in YYYYMMDD format
    :param table_type 'calibration', 'mask' or 'fit'
    :return TableWorkspace with corresponding data
    """
    TableType.assert_valid_type(table_type)
    verify_date_format("load_bank_table", date)
    filename: str = filename_bank_table(bank_id, database_path, date, table_type)
    logger.notice(f"Loading bank{bank_id} {table_type} file from database")
    outputWS = LoadNexusProcessed(filename)
    return outputWS


def combine_temporal_banks(database_path: str, date: str, table_type: str = "calibration") -> Tuple[WorkspaceGroup, List[Tuple[int, str]]]:
    """
    Function that combines stored bank calibrations into a group workspace

    If two or more calibrations exists for a given bank, the calibration with the closest
    day-stamp to `date` (but previous to `date`) is selected.

    In addition to the workspace group, the function returns a lists of (bank-number, day-stamp) pairs

    Output WorkspaceGroup can be an input to combine_spatial_banks
    Example:
            database/corelli/
                bank001/table_type_corelli_bank001_20201201.nxs.h5
                bank002/table_type_corelli_bank002_20201201.nxs.h5
                bank003/table_type_corelli_bank003_20201201.nxs.h5
                bank004/table_type_corelli_bank004_20200601.nxs.h5

    will return a WorkspaceGroup with TableWorkspace elements from the above
    files if date = 20201201. Since bank004 wasn't calibrated on 20201201,
    20200601 is selected as the most recent one before 20201201. The anterior day-stamp
    is 20201201.

    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param date pivot date in YYYYMMDD
    :param table_type 'calibration', 'mask' or 'fit'
    :return workspace group containing the bank calibrations and the list of (bank-number, day-stamp) pairs
    """
    TableType.assert_valid_type(table_type)

    tables: WorkspaceGroup = WorkspaceGroup()

    bank_dirs = list(pathlib.Path(database_path).glob("bank" + ("[0-9]" * 3)))
    # filter directories
    bank_dirs = [bank_dir for bank_dir in bank_dirs if bank_dir.is_dir()]
    bank_dirs.sort()

    bank_stamps = list()  # list of (bank-number, day-stamp) pairs
    for bank_dir in bank_dirs:
        # extract files in bank calibration directory
        files = list(pathlib.Path(bank_dir).glob(table_type + "_corelli_bank" + ("[0-9]" * 3) + "*"))

        available_dates = []

        for file in files:
            split = re.split(r"_|\.", file.name)
            if len(split) < 4:
                raise ValueError("Bank file " + str(file) + " does not meet the accepted format")
            available_dates.append(int(split[3]))

        available_dates.sort(reverse=True)

        # check available dates choose the latest in the past if not found for current date
        for available_date in available_dates:
            if available_date <= int(date):
                bank_id: int = int(str(bank_dir)[-3:])
                table = load_bank_table(bank_id, database_path, str(available_date), table_type)
                tables.addWorkspace(table)
                bank_stamps.append((bank_id, available_date))
                break

    return tables, bank_stamps


def combine_spatial_banks(tables: WorkspaceGroup, table_type: str = "calibration", name: Optional[str] = None) -> TableWorkspace:
    """
    Function that inputs a GroupWorkspace of TableWorkspace .

    :param tables: input GroupWorkspace with independent bank TableWorkspace for Corelli
    :param table_type: input type of table. One of ('calibration', 'mask')
    :param name: input name for the TableWorkSpace output
    :return unified TableWorkspace for all banks
    """
    message = f"{'Cannot process Corelli combine_spatial_banks, input is not of type WorkspaceGroup'}"
    assert isinstance(tables, WorkspaceGroup), message

    combined_table: TableWorkspace = (
        init_corelli_table(table_type=table_type, name=name) if name else init_corelli_table(table_type=table_type)
    )

    for i in range(tables.getNumberOfEntries()):
        table = tables.getItem(i)

        # check type
        message = f"Cannot process Corelli combine_spatial_banks, table {str(i)} is not of type TableWorkspace"
        assert isinstance(table, TableWorkspace), message

        # check column names
        if not has_valid_columns(table, table_type=table_type):
            message = f"Table index {i} of type {table_type}is not a valid Corelli TableWorkspace"
            raise RuntimeError(message)

        table_dict = table.toDict()

        # loop through rows
        column_names = table.getColumnNames()  # 'Detector ID' and 'Detector Y Coordinate' for 'calibration' table_type
        for r in range(0, table.rowCount()):
            combined_table.addRow([table_dict[name][r] for name in column_names])

    return combined_table


def day_stamp(input_workspace: InputWorkspaceTypes) -> int:
    r"""
    Find the day YYYYMMDD stamp (e.g 20200311 for March 11, 2020) using metadata from the
    Nexus events file as input.

    :param input_workspace: input workspace from which the day stamp is to be retrieved.
    """
    run = mtd[str(input_workspace)].getRun()
    for log_key in ("run_start", "start_time"):
        if log_key in run.keys():
            return int(run[log_key].value[0:10].replace("-", ""))
    raise KeyError(f"Day stamp not found in the metadata of {str(input_workspace)}")


def verify_date_format(function_name: str, date: Union[int, str]) -> None:
    """
    Function that verifies if date input string has a correct date format: YYYYMMDD.
    Throws an exception otherwise.
    :param function_name input that calls this function for better exception handling
    :param date input date string to be verified
    :raises ValueError if date is not in YYYYMMDD format
    """
    try:
        if len(str(date)) != 8:
            raise ValueError("")
        datetime.strptime(str(date), "%Y%m%d")
    except ValueError:
        raise ValueError("date in function " + function_name + " " + str(date) + " is not YYYYMMDD")


def new_corelli_calibration(database_path: str, date: Optional[str] = None) -> Tuple[str, str, str]:
    r"""
    Generate a Corelli calibration set of files for a given day stamp, or for today if no day stamp is given.

    For each bank, this function will retrieve the calibration with an anterior date as close as possible
    to the given day stamp. The day stamp for the generated calibration will the most modern day stamp among
    the day stamps of all banks.

    The files to be produced are:
    - database_path/calibration_corelli_YYYYMMDD.nxs.h5
    - database_path/mask_corelli_YYYYMMDD.nxs.h5
    - database_path/manifest_corelli_YYYYMMDD.nxs.h5

    Example: Assume today's date is 20201201 and we have a database with calibrations for two
    different days. Furthermore our instrument has only one bank, for simplicity.
    database_path/
    |_bank001/
      |_calibration_corelli_bank001_20200101.nxs.h5  (calibration in January)
      |_calibration_corelli_bank001_20206101.nxs.h5  (calibration in June)
      |_mask_corelli_bank0010_20200101.nxs.h5
      |_mask_corelli_bank0010_20200601.nxs.h5
    Invoking today new_corelli_calibration(database_path) will create the following files:
    - database_path/calibration_corelli_20206101.nxs.h5
    - database_path/mask_corelli_20206101.nxs.h5
    - database_path/manifest_corelli_20206101.nxs.h5
    Notice the date of the files (20206101) is not today's date (20201201) but the most "modern" day-stamp in
    the database.
    Invoking today new_corelli_calibration(database_path, date=20200301) will create the following files:
    - database_path/calibration_corelli_20201101.nxs.h5
    - database_path/mask_corelli_20201101.nxs.h5
    - database_path/manifest_corelli_20201101.nxs.h5
    The January files (20201101) are selected because we requested an instrument calibration
    with a date (20200301) prior to the June files.



    :param database_path: absolute path to the database containing the bank calibrations
    :param date: day stamp in format YYYYMMDD

    :return: absolute path to files containing the calibrated pixels, the masked pixels,
        and the manifest file, in this order.
    """
    if date is None:
        date = datetime.now().strftime("%Y%m%d")  # today's date in YYYYMMDD format
    verify_date_format("new_corelli_calibration", date)

    file_paths = dict()
    for table_type in ("calibration", "mask"):
        logger.notice(f"** Gathering {table_type} tables from individual banks")
        bank_tables, bank_stamps = combine_temporal_banks(database_path, date, table_type)
        if len(bank_stamps) == 0:
            logger.warning(f"No bank {table_type} files found with date < {date}")
            continue
        logger.notice(f"** Combining {table_type} tables from individual banks")
        table = combine_spatial_banks(bank_tables, table_type=table_type)

        bank_numbers, day_stamps = zip(*bank_stamps)
        last_day_stamp = sorted(day_stamps)[-1]
        filename = str(pathlib.Path(database_path) / f"{table_type}_corelli_{last_day_stamp}.nxs.h5")
        logger.notice(f"** Saving instrument {table_type} to the database")
        SaveNexusProcessed(InputWorkspace=table, Filename=filename)
        file_paths[table_type] = filename

        if table_type == "calibration":
            logger.notice("** Creating and saving the manifest file")
            file_paths["manifest"] = save_manifest_file(database_path, bank_numbers, day_stamps, manifest_day_stamp=last_day_stamp)

    return [file_paths[x] for x in ("calibration", "mask", "manifest")]


def _table_to_workspace(input_workspace: Union[str, TableWorkspace], output_workspace: Optional[str] = None) -> MaskWorkspace:
    r"""
    @brief Convert a CORELLI calibration mask table to a MaskWorkspace

    :param input_workspace : the table containing the detector ID's for the masked detectors
    :param output_workspace : name of the output MaskWorkspace

    :return: handle to the MaskWorkspace
    """
    table_handle = mtd[str(input_workspace)]
    detectors_masked = table_handle.column(0)  # list of masked detectors
    if output_workspace is None:
        output_workspace = str(input_workspace)
    LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace=output_workspace)
    ClearMaskFlag(Workspace=output_workspace)  # for good measure
    MaskDetectors(Workspace=output_workspace, DetectorList=detectors_masked)  # output_workspace is a Workspace2D
    # output_workspace is converted to a MaskWorkspace, where the Y-values of the spectra are now either 0 or 1
    ExtractMask(InputWorkspace=output_workspace, OutputWorkspace=output_workspace)
    return mtd[output_workspace]


def load_calibration_set(
    input_workspace: Union[str, Workspace],
    database_path: str,
    output_calibration_name: str = "calibration",
    output_mask_name: str = "mask",
    mask_format: str = "MaskWorkspace",
) -> Tuple[Optional[TableWorkspace], Optional[TableWorkspace]]:
    r"""
    Retrieve an instrument calibration and instrument mask.

    The input workspaces has a day stamp corresponding to the day when the experiment was started
    (the run-start day). This day is used to find in the database the calibration with the closest
    and prior date to the run-start day.

    :param input_workspace: Workspace containing the run-start in the metadata
    :param database_path: absolute path to the instrument calibration tables
    :param output_calibration_name: name of the TableWorkspace containing the calibrated pixel positions
    :param output_mask_name: name of the TableWorkspace containing the uncalibrated pixels to be masked
    :param mask_format: return the mask either as a 'MaskWorkspace' or as 'TableWorkspace/

    :return: calibration TableWorkspdce and mask MaskWorkspace. Returns `None` for each of these if a suitable
        calibration file is not found in the database.
    """
    valid_mask_formats = ("MaskWorkspace", "TableWorkspace")
    if mask_format not in valid_mask_formats:
        raise ValueError(f"mask_format must be one of {valid_mask_formats}")

    workspace_names = {"calibration": output_calibration_name, "mask": output_mask_name}
    run_start = day_stamp(input_workspace)  # day when the experiment associated to `input_workspace` was started
    instrument_tables = {"calibration": None, "mask": None}  # store the calibration and mask instrument tables

    for table_type in ("calibration", "mask"):
        filename = None  # store the path to the target instrument table
        candidate_files = list(pathlib.Path(database_path).glob(table_type + "_corelli_*"))
        # extract the YYYYMMDD date from each file name, convert to integers
        available_dates = [int(re.search(r"corelli_(\d+)", str(f)).groups()[0]) for f in candidate_files]
        date_to_file = {d: str(f) for d, f in zip(available_dates, candidate_files)}  # mapping
        # numerical sort the available dates
        available_dates.sort()
        # Find the file with the daystamp closest to `run_start` and previous to `run_start`
        for available_dates_index, available_date in enumerate(available_dates):
            if available_date == run_start:
                filename = date_to_file[available_date]
                break
            elif available_date > run_start:  # we overshot with the date
                if available_dates_index == 0:  # database contains one file, but with a later daystamp than `run_start`
                    break  # no calibration files is available with a prior date to `run_start`
                # the correct calibration is the one with a date previous to `run_start`
                filename = date_to_file[available_dates[available_dates_index - 1]]
                break
        # final boundary case: run_start > latest available_date
        if len(available_dates) > 0 and available_dates[-1] < run_start:
            filename = date_to_file[available_dates[-1]]
        if filename is not None:
            logger.notice(f"Found {filename} for {str(input_workspace)} with run start {run_start}")
            instrument_tables[table_type] = LoadNexusProcessed(Filename=filename, OutputWorkspace=workspace_names[table_type])
            # Additional step to convert the mask TableWorkspace to a MaskWorkspace
            if table_type == "mask" and mask_format == "MaskWorkspace":
                instrument_tables[table_type] = _table_to_workspace(workspace_names[table_type])
        else:
            message = f"No {table_type} file found for {str(input_workspace)} with run start {run_start}. "
            if len(available_dates) > 0:
                message += f"Oldest calibration date is {available_dates[0]}"
            logger.warning(message)

    return instrument_tables.values()
