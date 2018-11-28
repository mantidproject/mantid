# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from os import path, listdir
from mantid.simpleapi import LoadNexusProcessed, LoadMD
from mantid import logger


class WorkspaceLoader(object):
    def load_workspaces(self, directory):
        filenames = listdir(directory)
        for filename in filenames:
            workspace_name, file_ext = path.splitext(filename)
            if file_ext != u'.project':
                try:
                    # Attempt to load but if failed, fail with a warning
                    if file_ext is ".md":
                        LoadMD(directory + "/" + filename, outputworkspace=workspace_name)
                    else:
                        LoadNexusProcessed(directory + "/" + filename, outputworkspace=workspace_name)
                except Exception as exception:
                    if isinstance(exception, KeyboardInterrupt):
                        raise KeyboardInterrupt
                    else:
                        logger.warning("Couldn't load file in project: " + filename)
