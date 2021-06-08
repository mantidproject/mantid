# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# 3rd party
from mantid import config, FileFinder

# standard
from pathlib import Path
from typing import Optional, Union


def run_exists(run_number: Union[str, int], instrument: Optional[str] =None) -> bool:
    r"""
    @brief Test whether the file for a run number exists.
    @details Search first the datasearch directories and if file is not found, use the locations
    provided by ONCat
    @param run_number : just the bare run number, e.g. 12345
    @param instrument : if None, then retrieve the default instrument from the configuration service
    """
    if instrument is None:
        instrument = config['default.instrument']
    path = f'{instrument}_{run_number}'
    # check in `datasearch.directories`
    if Path(FileFinder.getFullPath(path)).exists():
        return True
    # check via locations provided by ONCat
    try:
        for option in FileFinder.findRuns(path):
            if Path(option).exists():
                return True
    except RuntimeError:
        return False  # no suggestions found
    return False
