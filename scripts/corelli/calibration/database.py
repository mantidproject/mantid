# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# File that defines functionality for Corelli calibration database

import pathlib
from datetime import datetime
import re
from typing import Optional

from mantid.dataobjects import TableWorkspace
from mantid.api import WorkspaceGroup, Workspace
from mantid.simpleapi import CreateEmptyTableWorkspace, SaveNexusProcessed, LoadNexusProcessed


def init_corelli_table( name:Optional[str] = None )->TableWorkspace :
    """
    Function that initializes a Corelli calibration TableWorkspace columns
    """
    table:TableWorkspace = CreateEmptyTableWorkspace(OutputWorkspace=name) if name else CreateEmptyTableWorkspace()

    table.addColumn(type="int", name="Detector ID")
    table.addColumn(type="vector_double", name="Detector Position")
    return table


def has_valid_columns(table:TableWorkspace)->bool:
    """
    Check if table column names are valid Corelli calibration tables
    """
    names = table.getColumnNames()
    if names != ['Detector ID', 'Detector Position']:
        return False

    return True


def filename_bank_table( bankID:int, database_path:str, date:str, table_type:str = 'calibration' ) -> str:
    """
    Function that returns the absolute path for a bank file using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bankID bank number that is calibrated
    :param date format YYYYMMDD
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param type 'calibration', 'mask' or 'fit'
    """

    verify_date_YYYYMMDD('filename_bank_table', date)
    subdirectory:str = database_path + '/' + 'bank' + str(bankID).zfill(3)
    pathlib.Path(subdirectory).mkdir(parents=True, exist_ok=True)
    message = f'{"Cannot process Corelli filename_bank_table, table_type must be calibration, mask or fit"}'
    assert table_type == 'calibration' or table_type == 'mask' or table_type == 'fit', message

    filename = subdirectory + '/' + table_type + '_corelli_bank' + str(bankID).zfill(3) + '_' + date + '.nxs.h5'
    # make it portable
    filename = str(pathlib.Path(filename).resolve())
    return filename


def save_manifest_file( bankIDs:list, database_path:str, date:str)->None:
    """
    Function that saves or updates an existing manifest_corelli_date.csv file.
    There is one file stored for each calibration day. If file exist it will append two columns:
    bankID, timestamp (ISO format)
    :param bankIDs input of bankIDs that have been calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for manifest file:
                    database/corelli/manifest_corelli_YYYYMMDD.csv
    :param date format YYYYMMDD
    """
    # verify timestamps are correct
    verify_date_YYYYMMDD('save_manifest_file', date)
    filename = database_path + '/manifest_corelli_' + date + '.csv'

    if pathlib.Path(filename).is_file():
        file = open(filename, 'a+')
    else:
        file = open(filename, 'w')
        file.write('bankID, timestamp\n') # header

    lines:str = ''
    for t, bankID in enumerate(bankIDs):
        lines += str(bankID) + ', ' + str(date) + '\n'

    file.write(lines)
    file.close()


def save_bank_table( data:Workspace, bankID:int, database_path:str, date:str,
                     table_type:str = 'calibration') -> None:
    """
    Function that saves a bank calibrated TableWorkspace into a single HDF5 file
    using corelli format and using current date:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs.h5
    :param data input Workspace (TableWorkspace) data for calibrated pixels
    :param bankID bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param table_type 'calibration', 'mask' or 'fit'
    """
    verify_date_YYYYMMDD('save_bank_table', date)
    filename:str = filename_bank_table(bankID, database_path, date, table_type)
    SaveNexusProcessed(data,filename)


def load_bank_table( bankID:int, database_path:str, date:str, table_type:str = 'calibration'
                     ) -> TableWorkspace:
    """
    Function that loads the latest bank calibrated TableWorkspace from a single HDF5 file
    using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bankID bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param date current day in YYYYMMDD format
    :param table_type 'calibration', 'mask' or 'fit'
    :return TableWorkspace with corresponding data
    """
    verify_date_YYYYMMDD('load_bank_table', date)
    filename:str = filename_bank_table(bankID, database_path, date, table_type)
    outputWS = LoadNexusProcessed(filename)
    return outputWS


def combine_temporal_banks( database_path:str, date:str, table_type:str = 'calibration' )->WorkspaceGroup:
    """
    Function that combines stored banks for a certain calibration date file,
    if not available the latest calibration date is picked up.
    Output WorkspaceGroup can be an input to combine_spatial_banks
    Example:
            database/corelli/
                bank001/table_type_corelli_bank001_20201201.nxs.h5
                bank002/table_type_corelli_bank002_20201201.nxs.h5
                bank003/table_type_corelli_bank003_20201201.nxs.h5
                bank004/table_type_corelli_bank004_20200601.nxs.h5

    will return a WorkspaceGroup with TableWorkspace elements from the above
    files if date = 20201201. Since bank004 wasn't calibrated on 20201201,
    20200601 is selected as the most recent one before 20201201.
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param date pivot date in YYYYMMDD
    :param table_type 'calibration', 'mask' or 'fit'
    :return WorkspaceGroup, each component is a calibrated TableWorkspace bank
    """

    tables:WorkspaceGroup = WorkspaceGroup()

    bank_dirs = list(pathlib.Path(database_path).glob('bank'+ ('[0-9]' * 3) ))
    # filter directories
    bank_dirs = [dir for dir in bank_dirs if dir.is_dir()]
    bank_dirs.sort()

    for bank_dir in bank_dirs:

        # extract files in bank calibration directory
        files = list(pathlib.Path(bank_dir).glob(table_type + '_corelli_bank' + ('[0-9]' * 3) + '*' ) )

        available_dates = []

        for file in files:
            split = re.split( r'_|\.', file.name )
            if len(split) < 4:
                raise ValueError('Bank file ' +str(file) + ' does not meet the accepted format')
            available_dates.append(int(split[3]))

        available_dates.sort(reverse=True)

        # check available dates choose the latest in the past if not found for current date
        for available_date in available_dates:

            if available_date <= int(date):
                bankID:int = int(str(bank_dir)[-3:])
                table = load_bank_table(bankID, database_path, str(available_date), table_type)
                tables.addWorkspace(table)
                break

    return tables


def combine_spatial_banks(tables:WorkspaceGroup, name: Optional[str] = None)->TableWorkspace:
    """
    Function that inputs a GroupWorkspace of TableWorkspace .
    :param tables: input GroupWorkspace with independent bank TableWorkspace for Corelli
    :param name: input name for the TableWorkSpace output
    :return unified TableWorkspace for all banks
    """
    message = f'{"Cannot process Corelli combine_spatial_banks, input is not of type WorkspaceGroup"}'
    assert isinstance(tables, WorkspaceGroup), message

    combined_table:TableWorkspace = init_corelli_table(name) if name else init_corelli_table()

    for i in range(tables.getNumberOfEntries()):
        table = tables.getItem(i)

        # check type
        message = f'Cannot process Corelli combine_spatial_banks, table ' + str(i) + ' is not of type TableWorkspace'
        assert isinstance(table, TableWorkspace), message

        # check column names
        if not has_valid_columns(table):
            raise RuntimeError('Table index ' + str(i) + ' is not a valid Corelli TableWorkspace ["Detector ID", "Detector Position"]')

        table_dict = table.toDict()

        # loop through rows
        for r in range(0,table.rowCount()):
            combined_table.addRow( [table_dict['Detector ID'][r], table_dict['Detector Position'][r] ])

    return combined_table


def verify_date_YYYYMMDD(function_name:str, date:str)->None:
    """
    Function that verifies if date input string has a correct date format: YYYYMMDD.
    Throws an exception otherwise.
    :param function_name input that calls this funciton for better exception handling
    :param date input date string to be verified
    :raises ValueError if date is not in YYYYMMDD format
    """
    try:
        if len(date) != 8:
            raise ValueError('')
        datetime.strptime(date, '%Y%m%d')
    except ValueError:
        raise ValueError('date in function ' + function_name + ' ' + date +  ' is not YYYYMMDD')
