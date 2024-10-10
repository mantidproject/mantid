# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This modules provides a function, importAllFromADS, that creates a python
variable in the globals dict of the current frame for each
name in the analysis data service.

If the name is not a valid variable then no variable is created.

The function is also attached to the AnalysisDataService and named importAll.

"""

from mantid.api import AnalysisDataServiceImpl
import inspect as _inspect
import keyword as _keyword
import re as _re

# Valid identifier
IDENT_REGEX = _re.compile(r"^[a-zA-Z_]\w*$")
# Invalid characters
INVALID_CHARS_REGEX = _re.compile(r"[^0-9a-zA-Z_]")
# Starts with a number
LEADING_NUMS_REGEX = _re.compile(r"^[0-9]")


def _importAll(mtd):
    """
    Creates a named variable in the globals dictionary
    of the current frame (inspect.currentframe).

    For example, if the ADS contains a workspace with the name "deltax" then

        mtd.importAll()

    will create a python variable for that workspace as if the user had typed
    mtd['deltax'].

    @param mtd The Analysis Data Service Object
    """

    def clean(name):
        """
        Returns a name cleaned up so that it is a valid
        identifier
        """
        # If it is exactly a keyword just prefix with an underscore
        if _keyword.iskeyword(name):
            varname = "_" + name
        else:
            # Replace invalid characters with underscore
            varname = INVALID_CHARS_REGEX.sub("_", name)
            # If it starts with a number prefix with underscore
            if LEADING_NUMS_REGEX.match(varname) is not None:
                varname = "_" + varname
        return varname

    stack = _inspect.stack()
    try:
        # stack[1][0] is the frame object of the caller to this function
        locals_ = stack[1][0].f_locals
    finally:
        del stack
    ads_names = mtd.getObjectNames()
    vars = {}
    # The name may not be a valid variable name, i.e keyword or operator separated
    for name in ads_names:
        varname = name
        if not is_valid_identifier(varname):
            varname = clean(name)
            print('Warning: "%s" is an invalid identifier, "%s" has been imported instead.' % (name, varname))
        vars[varname] = mtd[name]
    # Update the caller's dictionary
    locals_.update(vars)


def is_valid_identifier(name):
    """
    Returns True if the given string
    is a valid variable name in Python
    """
    if _keyword.iskeyword(name):
        return False
    # If the regex matches it is a valid identifier
    # Rules can be found here https://www.w3schools.com/python/gloss_python_variable_names.asp
    return IDENT_REGEX.match(name) is not None


# Attach to ADS as importAll
setattr(AnalysisDataServiceImpl, "importAll", _importAll)
