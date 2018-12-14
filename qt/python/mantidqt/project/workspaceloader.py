# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from os import path

from mantid import logger


class WorkspaceLoader(object):
    @staticmethod
    def load_workspaces(directory, workspaces_to_load):
        """
        The method that is called to load in workspaces. From the given directory and the workspace names provided.
        :param directory: String or string castable object; The project directory
        :param workspaces_to_load: List of Strings; of the workspaces to load
        """
        from mantid.simpleapi import Load  # noqa
        for workspace in workspaces_to_load:
            try:
                Load(path.join(directory, (workspace + ".nxs")), OutputWorkspace=workspace)
            except BaseException as exception:
                if isinstance(exception, KeyboardInterrupt):
                    raise KeyboardInterrupt
                else:
                    logger.warning("Couldn't load file in project: " + workspace + ".nxs")
