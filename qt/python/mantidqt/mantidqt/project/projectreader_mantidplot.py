# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from mantidqt.project.projectparser_mantidplot import MantidPlotProjectParser
from mantidqt.project.projectreaderinterface import ProjectReaderInterface
from mantid import logger


class ProjectReaderMantidPlot(ProjectReaderInterface):
    def __init__(self, project_file_ext, filename):
        super().__init__(project_file_ext, filename)
        self.parser = None

    def read_project(self):
        try:
            self.parser = MantidPlotProjectParser(self.filename)
            self.read_workspaces()
            self.read_interfaces()
            self.read_plots()
        except Exception as err:
            logger.warning("Mantidplot project file unable to be loaded/read", err)

    def read_workspaces(self):
        logger.notice("Loading workspaces from Mantidplot project file " + self.filename)
        self.workspace_names = self.parser.get_workspaces()

    def read_interfaces(self):
        logger.notice("Loading interfaces from mantid plot project file not supported")

    def read_plots(self):
        logger.notice("Loading plots from MantidPlot project file")
        self.plot_list = self.parser.get_plots()
