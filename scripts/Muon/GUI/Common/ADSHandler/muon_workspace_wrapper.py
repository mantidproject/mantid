# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=F0401
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import mtd
from mantid import api
from mantid.api import Workspace


def add_directory_structure(dirs):
    """
    create the nested WorkspaceGroup structure in the ADS specified by the
    stored directory attribute.
    dirs = ["dir1", "dir2"] eg. ['Muon Data', 'MUSR72105', 'MUSR72105 Raw Data']
    """
    if not dirs:
        return
    if len(dirs) > len(set(dirs)):
        raise ValueError("Group names must be unique")

    for directory in dirs:
        if not mtd.doesExist(directory):
            workspace_group = api.WorkspaceGroup()
            mtd.addOrReplace(directory, workspace_group)
        elif not isinstance(mtd[directory], api.WorkspaceGroup):
            mtd.remove(directory)
            workspace_group = api.WorkspaceGroup()
            mtd.addOrReplace(directory, workspace_group)
        else:
            # exists and is a workspace group
            pass

    # Create the nested group structure in the ADS
    previous_dir = ""
    for i, directory in enumerate(dirs):
        if i == 0:
            previous_dir = directory
            continue
        if not mtd[previous_dir].__contains__(directory):
            mtd[previous_dir].add(directory)
        previous_dir = directory


class MuonWorkspaceWrapper(object):
    """
    A wrapper around a single workspace for use with MuonAnalysis.

    A single instance of the wrapped workspace exists, but it is either in the ADS or stored in a
    class attribute. Either way, a single property "workspace" will always return it; with the
    show() and hide() methods storing/retrieving it from the the ADS. The show() attribute takes a
    name, which is a string using a folder-like syntax to describe the workspaces location within a
    nested structure of group workspaces

    so for example name = "group1/group2/workspace" would store the workspace in the ADS inside
    group2, which is inside group1. The "/" character is used to separate folders in the name as in
    the example.

    A basic muon workspace which is either the workspace or the name of the workspace in the ADS
    """

    def __init__(self, workspace, name=''):
        self._is_in_ads = False
        self._workspace = None
        self._directory_structure = ""
        self._workspace_name = ""

        self.workspace = workspace
        self.name = name

    def __str__(self):
        return "MuonWorkspaceWrapper Object \n" \
               "In ADS         : {}\n" \
               "Name           : {}\n" \
               "Directory      : {}\n" \
               "Workspace Type : {}\n".format(self._is_in_ads,
                                              self._workspace_name,
                                              self._directory_structure,
                                              type(self._workspace))

    @property
    def is_hidden(self):
        """Is the workspace hidden (i.e. not in the ADS)."""
        return not self._is_in_ads

    @property
    def name(self):
        """The current name of the workspace."""
        return self._directory_structure + self._workspace_name

    @property
    def workspace_name(self):
        return self._workspace_name

    @property
    def workspace(self):
        """The Workspace object."""
        if not self.is_hidden:
            return mtd[self._workspace_name]
        else:
            return self._workspace

    @name.setter
    def name(self, full_name):
        if not self.is_hidden:
            raise ValueError("Cannot change workspace name whilst it is in ADS")
        self._directory_structure = "/".join(full_name.split("/")[0:-1])
        self._workspace_name = full_name.split("/")[-1]

    @workspace.setter
    def workspace(self, value):
        if not self.is_hidden:
            if mtd.doesExist(self._workspace_name):
                mtd.remove(self._workspace_name)
            self._is_in_ads = False
        if isinstance(value, Workspace):
            self._workspace = value
        else:
            raise AttributeError("Attempting to set object of type {}, must be"
                                 " a Mantid Workspace type".format(type(value)))

    def show(self, name=''):
        """
        Show the workspace in the ADS inside the WorkspaceGroup structure specified in name
        name = dirs/../dirs/workspace_name
        """
        if not self.is_hidden:
            return

        if len(name) > 0:
            self.name = str(name)

        if len(self.name) > 0 and self.is_hidden:
            # add workspace to ADS
            mtd.addOrReplace(self._workspace_name, self._workspace)

            if self._directory_structure != "":
                self.add_directory_structure()
                # Add to the appropriate group
                group = self._directory_structure.split("/")[-1]
                if not mtd[group].__contains__(self._workspace_name):
                    mtd[group].add(self._workspace_name)

            self._workspace = None
            self._is_in_ads = True
        else:
            raise ValueError("Cannot store workspace in ADS with name : ",
                             str(name))

    def hide(self):
        """
        Remove the workspace from the ADS and store it in the class instance
        """
        if mtd.doesExist(self._workspace_name):
            self._workspace = mtd[self._workspace_name]
            mtd.remove(self._workspace_name)

        self._is_in_ads = False

    def add_directory_structure(self):
        """
        create the nested WorkspaceGroup structure in the ADS specified by the
        stored _directory attribute.
        """
        dirs = self._directory_structure.split("/")
        add_directory_structure(dirs)
