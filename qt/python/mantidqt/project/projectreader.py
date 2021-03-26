# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json

from mantidqt.project.projectreader_mantidplot import ProjectReaderMantidPlot
from mantidqt.project.projectreader_workbench import ProjectReaderWorkbench


def is_json_file(filename):
    with open(filename) as data:
        try:
            json.load(data)
            return True
        except ValueError:
            return False


class ProjectReader(object):
    def __init__(self, project_extension):
        self.project_reader_instance = None
        self.project_extension = project_extension

    @property
    def workspace_names(self):
        if self.project_reader_instance:
            return self.project_reader_instance.workspace_names
        else:
            return None

    @property
    def plot_list(self):
        if self.project_reader_instance:
            return self.project_reader_instance.plot_list
        else:
            return None

    @property
    def interface_list(self):
        if self.project_reader_instance:
            return self.project_reader_instance.interface_list
        else:
            return None

    def read_project(self, filename):
        self._create_project_reader(filename)
        self.project_reader_instance.read_project()

    def _create_project_reader(self, filename):
        if is_json_file(filename):
            self.project_reader_instance = ProjectReaderWorkbench(self.project_extension, filename)
        else:
            self.project_reader_instance = ProjectReaderMantidPlot(self.project_extension, filename)
