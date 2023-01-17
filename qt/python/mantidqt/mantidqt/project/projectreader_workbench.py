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
import json


class ProjectReaderWorkbench(ProjectReaderInterface):
    def __init__(self, project_file_ext, filename):
        super().__init__(project_file_ext, filename)
        self.json_data = []

    def read_project(self):
        try:
            with open(self.filename) as f:
                self.json_data = json.load(f)
                self.read_workspaces()
                self.read_interfaces()
                self.read_plots()
        except Exception as err:
            logger.warning("JSON project file unable to be loaded/read", err)

    def read_workspaces(self):
        self.workspace_names = self.json_data["workspaces"]

    def read_interfaces(self):
        self.interface_list = self.json_data["interfaces"]

    def read_plots(self):
        self.plot_list = self.json_data["plots"]
