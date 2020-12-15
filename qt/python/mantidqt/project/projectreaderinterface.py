# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abc


class ProjectReaderInterface(metaclass=abc.ABCMeta):
    def __init__(self, project_file_ext, filename):
        self.workspace_names = None
        self.plot_list = None
        self.interface_list = None
        self.project_file_ext = project_file_ext
        self.filename = filename

    @abc.abstractmethod
    def read_project(self):
        pass

    @abc.abstractmethod
    def read_workspaces(self):
        pass

    @abc.abstractmethod
    def read_plots(self):
        pass

    @abc.abstractmethod
    def read_interfaces(self):
        pass
