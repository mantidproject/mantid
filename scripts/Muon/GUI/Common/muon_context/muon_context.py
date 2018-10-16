# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# Muon context - contains all of the values from the GUI
from __future__ import (absolute_import, division, print_function)
from six import iteritems

from Muon.GUI.Common import group_object
from Muon.GUI.Common import pair_object

import pythonTSV as TSV
import mantidqtpython

# constant variable names
Tab2Text = "some text"
HelpText = "help text"
LoadText = "load dummy"
Groups = "groups"
Pairs = "pairs"


class MuonContext(object):

    def __init__(self):
        self.common_context = {}
        self.common_context[Tab2Text] = "boo - start up"
        self.common_context[HelpText] = "Help dummy"
        self.common_context[LoadText] = "load dummy"
        self.common_context[Groups] = [group_object.group(
                                       "fwd", [1,2]),
                                       group_object.group("bwd",[3,4,5]),
                                       group_object.group("top",[1,2,3,4,5])]
        self.common_context[
            Pairs] = [
                pair_object.pair(
                    "test pair",
                    "bwd",
                    "top",
                    0.9)]

    def set(self, key, value):
        self.common_context[key] = value

    def get(self, key):
        return self.common_context[key]

    def printContext(self):
        print(Tab2Text, self.common_context[Tab2Text])
        print(HelpText, self.common_context[HelpText])
        print(LoadText, self.common_context[LoadText])
        print(Groups)
        for group in self.common_context[Groups]:
            group.Print()
        print
        print(Pairs)
        for pair in self.common_context[Pairs]:
            print(pair.name, pair.FGroup, pair.BGroup, pair.alpha)
        print()

    def save(self):
        #save ....
        TSVSec = mantidqtpython.MantidQt.API.TSVSerialiser()
        TSV0 = mantidqtpython.MantidQt.API.TSVSerialiser()
        for key ,value in iteritems(self.common_context):
            TSV0.writeLine(key)
            try:
                 TSV.saveToTSV(TSV0,value)
            except:
                 try:
                    self.saveCustom(TSV0,key,value)
                 except:
                    pass
        lines = TSV0.outputLines()
        TSVSec.writeSection("Muon Analysis 2",lines)
        return TSVSec.outputLines()

    def saveCustom(self,TSV0,key,value):
        tmpTSV =  mantidqtpython.MantidQt.API.TSVSerialiser()
        for obj in value:
            obj.save(tmpTSV)
        TSV0.writeSection(key,tmpTSV.outputLines())

    def loadFromProject(self, project):
       print( "load ...")
