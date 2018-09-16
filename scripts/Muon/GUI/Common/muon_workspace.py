from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import mtd
from mantid import api

class MuonWorkspace(object):
    """A basic muon workspace which is either the workspace or the name of the workspace in the ADS"""

    def __init__(self, workspace):
        self._isInADS = False
        self._workspace = workspace
        self._directory = ""
        self._name = ""

    @property
    def name(self):
        return self._directory + self._name

    @name.setter
    def name(self, full_name):
        self._directory = full_name.split("/")[0]
        self._name = full_name.split("/")[-1]

    @property
    def workspace(self):
        # print("GETTING WORKSPACE")
        if self._isInADS:
            # print("\tis in ADS!")
            return mtd[self._name]
        else:
            # print ("\tis not in ADS!")
            return self._workspace

    @workspace.setter
    def workspace(self, value):
        # print("SETTING WORKSPACE")
        # TODO : add isinstance checks
        if self._isInADS:
            mtd.remove(self._name)
            self._isInADS = False
            self._name = ""
            self._directory = ""
        self._workspace = value

    def show(self, name):
        # print("SHOWING WORKSPACE, NAME : ", str(name), " WORKSPACE ", type(self._workspace))
        if len(name) > 0 and self._isInADS == False:
            self._name = str(name)
            mtd.addOrReplace(str(self._name), self._workspace)
            if self._directory != "":
                # Add to the appropriate group
                group = self._directory.split("/")[-1]
                mtd[group].add(self._name)
            self._workspace = None
            self._isInADS = True
        else:
            print("Cannot store is ADS : name is empty")
            pass

    def hide(self):
        try:
            print("Y : ", mtd[self._name].readY(0))
            self._workspace = mtd[self._name]
            print("here")
            mtd.remove(self._name)
            self._name = ""
            self._isInADS = False
        except:
            print("Cannot remove from ADS")
            pass

    def add_directory_structure(self):

        dirs = self._directory.split("/")
        for directory in dirs:
            try:
                mtd[directory]
            except KeyError:
                group = api.WorkspaceGroup()
                mtd.addOrReplace(directory, group)

            if not isinstance(mtd[directory], api.WorkspaceGroup):
                break
            # add an else for if workspace not a group

        last_dir = ""
        for i, directory in enumerate(dirs):
            if i == 0:
                last_dir = directory
                continue
            mtd[last_dir].add(directory)
            last_dir = directory
