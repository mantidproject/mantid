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
        self._directory = "/".join(full_name.split("/")[0:-1])
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
        if len(name) > 0 and self._isInADS is False:
            self.name = str(name)
            print("\t\t Adding ", type(self._workspace), " as ", self._name)
            if mtd.doesExist(self._name):
                mtd.remove(self._name)
            mtd.addOrReplace(self._name, self._workspace)
            if self._directory != "":
                self.add_directory_structure()
                # Add to the appropriate group
                group = self._directory.split("/")[-1]
                print("\t\t Adding ", self._name, " to ", group)
                print("and now : ", api.AnalysisDataServiceImpl.Instance().getObjectNames())
                mtd[group].add(self._name)
            self._workspace = None
            self._isInADS = True
        else:
            print("Cannot store is ADS : name is empty")
            pass

    def hide(self):
        try:
            self._workspace = mtd[self._name]
            mtd.remove(self._name)
            self._name = ""
            self._directory = ""
            self._isInADS = False
        except:
            print("Cannot remove from ADS")
            pass

    def add_directory_structure(self):
        print("add directory structure start: ", api.AnalysisDataServiceImpl.Instance().getObjectNames())
        dirs = self._directory.split("/")
        for directory in dirs:
            try:
                mtd[directory]
            except KeyError as e:
                print("\t\t ", e.args)
                print("\t\t PRODUCING GROUP : ", directory)
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

        print("add directory structure end: ", api.AnalysisDataServiceImpl.Instance().getObjectNames())