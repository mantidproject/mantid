# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid import config, FileFinder

# standard
from pathlib import Path
from typing import Optional, Union


def run_file(run_number: Union[str, int], instrument: Optional[str] = None, oncat: Optional[bool] = True) -> Optional[None]:
    r"""
    @brief Test whether the file for a run number exists.
    @details Search first the datasearch directories and if file is not found, use the locations
    provided by ONCat
    @param run_number : just the bare run number, e.g. 12345
    @param instrument : if None, retrieve the default instrument from the configuration service
    @param oncat : whether to use the ONCat archiving service
    @returns None if file not found, otherwise absolute path to events file
    """
    if isinstance(run_number, str):  # verify run_number represents a positive integer number
        try:
            int(run_number)
        except ValueError:
            raise ValueError(f"{run_number} does not represent a number")
        if int(run_number) <= 0:
            raise ValueError(f"{run_number} does not represent an positive integer number")
    if instrument is None:
        instrument = config["default.instrument"]
    root_name = f"{instrument}_{run_number}"
    # check in 'datasearch.directories'
    for extension in (".h5", ".nxs", ".nxs.h5"):
        file_path = FileFinder.getFullPath(root_name + extension)
        if Path(file_path).is_file():
            return file_path
    # check via locations provided by ONCat
    if oncat:
        try:
            for option in FileFinder.findRuns(root_name):
                if Path(option).is_file():
                    return option
        except RuntimeError:
            return None  # no suggestions found
    return None
