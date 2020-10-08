# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# File that defines functionality for Corelli calibration database

import pathlib
from datetime import datetime

from mantid.dataobjects import TableWorkspace
from mantid.api import WorkspaceGroup, Workspace
from mantid.simpleapi import CreateEmptyTableWorkspace, SaveNexusProcessed, LoadNexusProcessed


def init_corelli_table()->TableWorkspace :
    """
    Function that initializes a Corelli calibration TableWorkspace columns
    """
    table:TableWorkspace = CreateEmptyTableWorkspace()
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


def filename_bank_table( bankID:int, database_path:str, table_type:str = 'calibration' ) -> str:
    """
    Function that returns the absolute path for a bank file using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bankID bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param type 'calibration', 'mask' or 'acceptance'
    """
    subdirectory:str = database_path + '/' + 'bank' + str(bankID).zfill(3)
    pathlib.Path(subdirectory).mkdir(parents=True, exist_ok=True)
    message = f'{"Cannot process Corelli filename_bank_table, table_type must be calibration, mask or acceptance"}'
    assert table_type == 'calibration' or table_type == 'mask' or table_type == 'acceptance', message

    time:str = datetime.now().strftime('%Y%m%d') # format YYYYMMDD
    filename = subdirectory + '/' + table_type + '_corelli_bank' + str(bankID).zfill(3) + '_' + time + '.nxs.h5'
    # make it portable
    filename = str(pathlib.Path(filename).resolve())
    return filename


def save_bank_table( data:Workspace, bankID:int, database_path:str, table_type:str = 'calibration' ) -> None:
    """
    Function that saves a bank calibrated TableWorkspace into a single HDF5 file
    using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param data input TableWorkspace data for calibrated pixels
    :param bankID bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param table_type 'calibration', 'mask' or 'acceptance'
    """
    filename:str = filename_bank_table(bankID, database_path, table_type)
    SaveNexusProcessed(data,filename)


def load_bank_table( bankID:int, database_path:str, table_type:str = 'calibration' ) -> TableWorkspace:
    """
    Function that loads a bank calibrated TableWorkspace from a single HDF5 file
    using corelli format:
    database_path/bank0ID/type_corelli_bank0ID_YYYYMMDD.nxs
    :param bankID bank number that is calibrated
    :param database_path location of the corelli database (absolute or relative)
           Example: database/corelli/ for
                    database/corelli/bank001/
                    database/corelli/bank002/
    :param table_type 'calibration', 'mask' or 'acceptance'
    :return TableWorkspace with corresponding data
    """
    filename:str = filename_bank_table(bankID, database_path, table_type)
    outputWS = LoadNexusProcessed(filename)
    return outputWS


def combine_spatial_banks(tables:WorkspaceGroup)->TableWorkspace:
    """
    Function that inputs a GroupWorkspace of TableWorkspace .
    :param tables: input GroupWorkspace with independent bank TableWorkspace for Corelli
    :return unified TableWorkspace for all banks
    """
    message = f'{"Cannot process Corelli combine_spatial_banks, input is not of type WorkspaceGroup"}'
    assert isinstance(tables, WorkspaceGroup), message

    combined_table:TableWorkspace = init_corelli_table()

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
