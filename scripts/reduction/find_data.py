# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,redefined-builtin
import os
import mantid.api as api
from mantid.kernel import logger


def find_data(file, instrument="", allow_multiple=False):
    """
    Finds a file path for the specified data set, which can either be:
        - a run number
        - an absolute path
        - a file name
    @param file: file name or part of a file name
    @param instrument: if supplied, FindNeXus will be tried as a last resort
    """
    # First, assume a file name
    file = str(file).strip()

    # If we allow multiple files, users may use ; as a separator,
    # which is incompatible with the FileFinder
    n_files = 1
    if allow_multiple:
        file = file.replace(";", ",")
        toks = file.split(",")
        n_files = len(toks)

    instrument = str(instrument)
    file_path = api.FileFinder.getFullPath(file)
    if os.path.isfile(file_path):
        return file_path

    # Second, assume a run number and pass the instrument name as a hint
    try:
        # FileFinder doesn't like dashes...
        instrument = instrument.replace("-", "")
        f = api.FileFinder.findRuns(instrument + file)
        if os.path.isfile(f[0]):
            if allow_multiple:
                # Mantid returns its own list object type, so make a real list out if it
                if len(f) == n_files:
                    return [i for i in f]
            else:
                return f[0]
    except:
        # FileFinder couldn't make sense of the supplied information
        pass

    # Third, assume a run number, without instrument name to take care of list of full paths
    try:
        f = api.FileFinder.findRuns(file)
        if os.path.isfile(f[0]):
            if allow_multiple:
                # Mantid returns its own list object type, so make a real list out if it
                if len(f) == n_files:
                    return [i for i in f]
            else:
                return f[0]
    except:
        # FileFinder couldn't make sense of the supplied information
        pass

    # If we didn't find anything, raise an exception
    logger.error("\n\nCould not find a file for %s: check your reduction parameters\n\n" % str(file))
    raise RuntimeError("Could not find a file for %s" % str(file))
