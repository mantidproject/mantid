# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import json
import os

from mantidqt.project.workspaceloader import WorkspaceLoader
from mantidqt.project.plotsloader import PlotsLoader
from mantidqt.project.decoderfactory import DecoderFactory
from mantid import AnalysisDataService as ADS, logger


def _confirm_all_workspaces_loaded(workspaces_to_confirm):
    if workspaces_to_confirm is None:
        return True

    current_workspaces = ADS.getObjectNames()
    for ws in workspaces_to_confirm:
        if ws not in current_workspaces:
            logger.warning("Project Loader was unable to load back all of project workspaces")
            return False
    return True


class ProjectLoader(object):
    def __init__(self, project_file_ext):
        self.project_reader = ProjectReader(project_file_ext)
        self.workspace_loader = WorkspaceLoader()
        self.plot_loader = PlotsLoader()
        self.decoder_factory = DecoderFactory()
        self.project_file_ext = project_file_ext

    def load_project(self, file_name, load_workspaces=True):
        """
        Will load the project in the given file_name
        :param file_name: String or string castable object; the file_name of the project
        :param load_workspaces: Bool; True if you want ProjectLoader to handle loading workspaces else False.
        :return: Bool; True if all workspace loaded successfully, False if not loaded successfully.
        """
        # It can be expected that if at this point it is NoneType that it's an error
        if file_name is None:
            return

        # Read project
        self.project_reader.read_project(file_name)

        directory = os.path.dirname(file_name)
        # Load in the workspaces
        if load_workspaces:
            self.workspace_loader.load_workspaces(directory=directory,
                                                  workspaces_to_load=self.project_reader.workspace_names)

        workspace_success = _confirm_all_workspaces_loaded(workspaces_to_confirm=self.project_reader.workspace_names)

        if workspace_success:
            # Load plots
            if self.project_reader.plot_list is not None:
                self.plot_loader.load_plots(self.project_reader.plot_list)

            # Load interfaces
            if self.project_reader.interface_list is not None:
                self.load_interfaces(directory=directory)

        return workspace_success

    def load_interfaces(self, directory):
        for interface in self.project_reader.interface_list:
            # Find decoder
            decoder = self.decoder_factory.find_decoder(interface["tag"])

            # Decode and Show the interface
            try:
                decoded_interface = decoder.decode(interface, directory)
                decoded_interface.show()
            except Exception as e:
                # Catch any exception and log it for the encoder
                if isinstance(e, KeyboardInterrupt):
                    raise
                logger.warning("Project Loader: An interface could not be loaded error: " + str(e))


class ProjectReader(object):
    def __init__(self, project_file_ext):
        self.workspace_names = None
        self.plot_list = None
        self.interface_list = None
        self.project_file_ext = project_file_ext

    def read_project(self, file_name):
        """
        :param file_name: String or string castable object; the file_name of the project
        Will read the project file in from the file_name that is given.
        try:
        """
        try:
            with open(file_name) as f:
                json_data = json.load(f)
                self.workspace_names = json_data["workspaces"]
                self.plot_list = json_data["plots"]
                self.interface_list = json_data["interfaces"]
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            logger.warning("JSON project file unable to be loaded/read")
