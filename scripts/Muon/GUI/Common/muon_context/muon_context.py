# Muon context - contains all of the values from the GUI
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common import group_object

# constant variable names
#CurrentFile = "InputWorkspace"
#LoadedFiles = "LoadedFiles"
Groups = "groups"
Pairs = "pairs"

class MuonContext(object):
    def __init__(self):
        self.common_context = {}
        #self.common_context[CurrentFile] = " "
        #self.common_context[LoadedFiles] = []
        self.common_context[Groups] = [group_object.group("fwd"), group_object.group("bwd"),group_object.group("top")]
        self.common_context[Pairs] = []


    def get(self,key):
        return self.common_context[key]

    def print(self):
         groups = []
         for group in self.common_context[Groups]:
             groups.append(group.name)
         print(Groups, groups)

