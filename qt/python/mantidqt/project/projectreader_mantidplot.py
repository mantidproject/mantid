# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from mantidqt.project.projectreaderinterface import ProjectReaderInterface
from mantid import logger
import re

class ProjectReaderMantidPlot(ProjectReaderInterface):

    def __init__(self, project_file_ext, filename):
        super().__init__(project_file_ext, filename)
        self.full_text = []

    def read_project(self):
        try:
            with open(self.filename) as f:
                self.full_text = f.read()
                self.read_workspaces()
                self.read_interfaces()
                self.read_plots()
        except Exception:
            logger.warning("Mantidplot project file unable to be loaded/read")

    def read_workspaces(self):
        #Get the string inside the mantidworkspaces tags, allowing for whitespace at either end
        workspaces_pattern = r"<mantidworkspaces>\s*(.*?)\s*<\/mantidworkspaces>"
        ws_match = re.search(workspaces_pattern, self.full_text, re.DOTALL)
        if ws_match:
            # split by tab
            ws_list = ws_match.group(1).split('\t')
            if len(ws_list) > 1 and ws_list[0] == "WorkspaceNames":
                # the first entry is just an identification tag
                self.workspace_names = ws_list[1:]
                logger.notice("Loading workspaces from Mantidplot project file " + self.filename)

    def read_interfaces(self):
        logger.notice("Loading interfaces from mantid plot project file not supported")

    def read_plots(self):
        logger.notice("Loading plots from mantid plot project file not supported")
