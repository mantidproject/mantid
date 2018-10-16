# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# Muon context - contains all of the values from the GUI
from __future__ import (absolute_import, division, print_function)
from six import iteritems
from collections import OrderedDict

from Muon.GUI.Common import group_object
from Muon.GUI.Common import pair_object

import pythonTSV as TSV
from mantidqtpython import MantidQt

# constant variable names
Tab2Text = "someText"
HelpText = "helpText"
LoadText = "loadDummy"
Groups = "groups"
Pairs = "pairs"


class MuonContext(object):

    def __init__(self,name):
        self._name = name
        self.common_context = OrderedDict()
        self.common_context[Tab2Text] = "boo-start up"
        self.common_context[HelpText] = "Help_dummy"
        self.common_context[LoadText] = "load_dummy"
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
        TSVSec = MantidQt.API.TSVSerialiser()
        TSV0 = MantidQt.API.TSVSerialiser()
        keys = self.common_context.keys()
        
        TSV0.writeLine("keys")
        TSV0.storeInt(len(keys))
        for key  in keys:
             TSV0.storeString(key)
        for key  in keys:
            TSV0.writeLine(key)
            value = self.common_context[key]
            try:
                 TSV.saveToTSV(TSV0,value)
            except:
                 try:
                    self.saveCustom(TSV0,key,value)
                 except:
                    pass
        lines = TSV0.outputLines()

        #tmp  = MantidQt.API.TSVSerialiser()
        #tmp.writeLine("test")
        #tmp.storeString("hi")
        #lines=tmp.outputLines()
        print(lines)
        #TSVSec.writeSection(self._name,lines)
        return lines#TSVSec.outputLines()

    def saveCustom(self,TSV0,key,value):
        tmpTSV = MantidQt.API.TSVSerialiser()
        for obj in value:
            obj.save(tmpTSV)
        TSV0.writeSection(key,tmpTSV.outputLines())

    def loadFromProject(self, project):
        #print(p)
        #print()
        #full_load = mantidqtpython.MantidQt.API.TSVSerialiser(project)
        #get section
        #secs = full_load.sections(self._name)
        #print(secs)

        load = MantidQt.API.TSVSerialiser(project)#tmp.outputLines())#secs[0])
        load.selectLine("keys")
        numKeys = load.readInt()
        keys = []
        for k in range(numKeys):
           tmp = load.readString()
           keys.append(tmp)
        #print("baaa")
        #load.selectLine(Tab2Text)
        #kkk = load.readInt()
        #tmp = load.readString()
        #print(tmp,"mmmm", kkk)
        for key in keys:
            try:
                #load.selectLine(key)
                #print("waa",key,load.lineAsString(key))
                
                value= self.common_context[key]
                self.common_context[key] = TSV.loadFromTSV(load,key, value)
                print("boo",self.common_context[key],key,value)
            except:
                pass
