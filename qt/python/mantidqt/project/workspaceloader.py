# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from os import path, listdir

from mantid import logger


class WorkspaceLoader(object):
    def load_workspaces(self, directory):
        from mantid.simpleapi import Load  # noqa
        filenames = listdir(directory)
        for filename in filenames:
            workspace_name, file_ext = path.splitext(filename)
            if file_ext != u'.project':
                try:
                    Load(directory + "/" + filename, OutputWorkspace=workspace_name)
                except BaseException as exception:
                    if isinstance(exception, KeyboardInterrupt):
                        raise KeyboardInterrupt
                    else:
                        logger.warning("Couldn't load file in project: " + filename)
