# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from json import dump
import os

from mantid.api import AnalysisDataService as ADS
from mantidqt.project.workspacesaver import WorkspaceSaver
from mantidqt.project.plotssaver import PlotsSaver
from mantid import logger


class ProjectSaver(object):
    def __init__(self, project_file_ext):
        self.project_file_ext = project_file_ext

    def save_project(self, file_name, workspace_to_save=None, plots_to_save=None, interfaces_to_save=None,
                     project_recovery=True):
        """
        The method that will actually save the project and call relevant savers for workspaces, plots, interfaces etc.
        :param file_name: String; The file_name of the
        :param workspace_to_save: List; of Strings that will have workspace names in it, if None will save all
        :param plots_to_save: List; of matplotlib.figure objects to save to the project file.
        :param interfaces_to_save: List of Lists of Window and Encoder; the interfaces to save and the encoders to use
        :param project_recovery: Bool; If the behaviour of Project Save should be altered to function correctly inside
        of project recovery
        :return: None; If the method cannot be completed.
        """
        # Check if the file_name doesn't exist
        if file_name is None:
            logger.warning("Please select a valid file name")
            return

        # Check this isn't saving a blank project file
        if (workspace_to_save is None and plots_to_save is None and interfaces_to_save is None) and project_recovery:
            logger.warning("Can not save an empty project")
            return

        directory = os.path.dirname(file_name)
        # Save workspaces to that location
        if project_recovery:
            workspace_saver = WorkspaceSaver(directory=directory)
            workspace_saver.save_workspaces(workspaces_to_save=workspace_to_save)
            saved_workspaces = workspace_saver.get_output_list()
        else:
            # Assume that this is project recovery so pass a list of workspace names
            saved_workspaces = ADS.getObjectNames()

        # Generate plots
        plots_to_save_list = PlotsSaver().save_plots(plots_to_save)

        # Save interfaces
        if interfaces_to_save is None:
            interfaces_to_save = []

        interfaces = self._return_interfaces_dicts(directory=directory, interfaces_to_save=interfaces_to_save)

        # Pass dicts to Project Writer
        writer = ProjectWriter(workspace_names=saved_workspaces,
                               plots_to_save=plots_to_save_list,
                               interfaces_to_save=interfaces,
                               save_location=file_name,
                               project_file_ext=self.project_file_ext)
        writer.write_out()

    @staticmethod
    def _return_interfaces_dicts(directory, interfaces_to_save):
        interfaces = []
        for interface, encoder in interfaces_to_save:
            # Add to the dictionary encoded data with the key as the first tag in the list on the encoder attributes
            try:
                tag = encoder.tags[0]
                encoded_dict = encoder.encode(interface, directory)
                encoded_dict["tag"] = tag
                interfaces.append(encoded_dict)
            except Exception as e:
                # Catch any exception and log it
                if isinstance(e, KeyboardInterrupt):
                    raise
                logger.warning("Project Saver: An interface could not be saver error: " + str(e))

        return interfaces


class ProjectWriter(object):
    def __init__(self, save_location, workspace_names, project_file_ext, plots_to_save, interfaces_to_save):
        self.workspace_names = workspace_names
        self.file_name = save_location
        self.project_file_ext = project_file_ext
        self.plots_to_save = plots_to_save
        self.interfaces_to_save = interfaces_to_save

    def write_out(self):
        """
        Write out the project file that contains workspace names, interfaces information, plot preferences etc.
        """
        # Get the JSON string versions
        to_save_dict = {"workspaces": self.workspace_names, "plots": self.plots_to_save,
                        "interfaces": self.interfaces_to_save}

        # Open file and save the string to it alongside the workspace_names
        if self.project_file_ext not in os.path.basename(self.file_name):
            self.file_name = self.file_name + self.project_file_ext
        try:
            with open(self.file_name, "w+") as f:
                dump(obj=to_save_dict, fp=f)
        except Exception as e:
            # Catch any exception and log it
            if isinstance(e, KeyboardInterrupt):
                raise
            logger.warning("JSON project file unable to be opened/written to")
