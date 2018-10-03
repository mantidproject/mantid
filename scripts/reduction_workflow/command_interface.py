# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
"""
    List of user commands.
"""
from __future__ import (absolute_import, division, print_function)
from reduction_workflow.reducer import Reducer


class ReductionSingleton(object):
    """ Singleton reduction class """
    # storage for the instance reference
    __instance = None

    def __init__(self):
        """ Create singleton instance """
        # Check whether we already have an instance
        if ReductionSingleton.__instance is None:
            # Create and remember instance
            ReductionSingleton.__instance = Reducer()

        # Store instance reference as the only member in the handle
        self.__dict__['_ReductionSingleton__instance'] = ReductionSingleton.__instance

    @classmethod
    def clean(cls, reducer_cls=None):
        if reducer_cls is None:
            ReductionSingleton.__instance = Reducer()
        else:
            ReductionSingleton.__instance = reducer_cls()

    @classmethod
    def replace(cls, red):
        """
            Set the object pointed to by the singleton with
            the one passed
            @param red: reducer object
        """
        if issubclass(red.__class__, Reducer):
            ReductionSingleton.__instance = red
        else:
            raise RuntimeError('The object passed to ReductionSingleton.replace() must be of type Reducer')

    @classmethod
    def run(cls):
        """
            Execute the reducer and then clean it (regardless of
            if it throws) to ensure that a partially run reducer is
            not left behind
        """
        try:
            if ReductionSingleton.__instance is not None:
                return ReductionSingleton.__instance._reduce()
        finally:
            ReductionSingleton.clean(ReductionSingleton.__instance.__class__)

    def __getattr__(self, attr):
        """ Delegate access to implementation """
        return getattr(self.__instance, attr)

    def __setattr__(self, attr, value):
        """ Delegate access to implementation """
        return setattr(self.__instance, attr, value)

## Utilities


def get_property_manager(name):
    # prop_mng = mantid.PropertyManagerDataService.retrieve(name)
    pass

## List of user commands ######################################################


def Clear(reducer_cls=None):
    """
        Clears the Reducer of changes applied by all previous commands
    """
    ReductionSingleton.clean(reducer_cls)


def DataPath(path):
    ReductionSingleton().set_data_path(path)


def OutputPath(path):
    ReductionSingleton().set_output_path(path)
    ReductionSingleton().reduction_properties["OutputDirectory"] = path


def Reduce1D():
    return ReductionSingleton().reduce()


def Reduce():
    return ReductionSingleton().reduce()


def AppendDataFile(datafile, workspace=None):
    """
        Append a data file in the list of files to be processed.
        @param datafile: data file to be processed
        @param workspace: optional workspace name for this data file
            [Default will be the name of the file]
    """
    ReductionSingleton().append_data_file(datafile, workspace)


def ClearDataFiles():
    """
        Empty the list of data files to be processed while keeping
        all other reduction options.
    """
    ReductionSingleton().clear_data_files()
