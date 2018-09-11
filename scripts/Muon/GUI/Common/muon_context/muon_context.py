# Muon context - contains all of the values from the GUI
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common import group_object
from Muon.GUI.Common import pair_object

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
        self.common_context[Pairs] = [pair_object.pair("test pair","bwd","top",0.9)]

    def set(self,key,value):
        self.common_context[key]=value

    def get(self,key):
        return self.common_context[key]

    def printContext(self):
         print(Groups)
         for group in self.common_context[Groups]:
             print(group.name)
         print
         print(Pairs)
         for pair in self.common_context[Pairs]:
             print(pair.getName(),pair.getFGroup(),pair.getBGroup(),pair.getAlpha())
         print()

