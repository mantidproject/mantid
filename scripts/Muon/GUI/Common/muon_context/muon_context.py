# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# Muon context - contains all of the values from the GUI
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict
import copy

from Muon.GUI.Common import group_object
from Muon.GUI.Common import pair_object

import pythonTSV as TSVHelper
from mantidqtpython import MantidQt

# constant variable names
Tab2Text = "some text"
HelpText = "help_text"
LoadText = "load-dummy"
Groups = "groups"
Pairs = "pairs"


class MuonContext(object):

    def __init__(self, name):
        self._name = name
        self.common_context = OrderedDict()
        self.common_context[Tab2Text] = "boo-start up"
        self.common_context[HelpText] = "Help_dummy"
        self.common_context[LoadText] = "load_dummy"
        self.common_context[Groups] = [group_object.group(
                                       "fwd", [1, 2]),
                                       group_object.group("bwd", [3, 4, 5]),
                                       group_object.group("top", [1, 2, 3, 4, 5])]
        self.common_context[
            Pairs] = [
                pair_object.pair(
                    "test_pair",
                    "fwd",
                    "bwd",
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
        # save ....
        TSVSec = MantidQt.API.TSVSerialiser()
        TSV0 = MantidQt.API.TSVSerialiser()
        keys = self.common_context.keys()

        TSV0.writeLine("keys")
        TSV0.storeInt(len(keys))
        for key in keys:
            TSV0.storeString(key)
        for key in keys:
            TSVHelper.writeLine(TSV0, key)
            value = self.common_context[key]
            try:
                TSVHelper.saveToTSV(TSV0, value)
            except:
                try:
                    self.saveCustom(TSV0, key, value)
                except:
                    pass
        lines = TSV0.outputLines()
        safeName = TSVHelper.makeLineNameSafe(self._name)
        TSVSec.writeSection(safeName, lines)
        return TSVSec.outputLines()

    def saveCustom(self, TSV0, key, value):
        tmpTSV = MantidQt.API.TSVSerialiser()
        tmpTSV.writeLine("members")
        tmpTSV.storeInt(len(value))
        # store all of the names/keys on one line
        for obj in value:
            tmpTSV.storeString(obj.name)
        # the below method writes a new line
        for obj in value:
            obj.save(tmpTSV)
        safeKey = TSVHelper.makeLineNameSafe(key)
        TSV0.writeSection(safeKey, tmpTSV.outputLines())

    def loadFromProject(self, project):
        full_load = MantidQt.API.TSVSerialiser(project)
        # get section
        safeName = TSVHelper.makeLineNameSafe(self._name)
        secs = full_load.sections(safeName)

        load = MantidQt.API.TSVSerialiser(secs[0])
        load.selectLine("keys")
        numKeys = load.readInt()
        keys = []
        for k in range(numKeys):
            tmp = load.readString()
            keys.append(tmp)
        for key in keys:
            value = self.common_context[key]
            try:
                self.common_context[
                    key] = TSVHelper.loadFromTSV(load, key, value)
            except:
                self.customLoad(load, key, value)
                pass

    def customLoad(self, load, key, value):
        safeKey = TSVHelper.makeLineNameSafe(key)
        sec = load.sections(safeKey)
        tmpTSV = MantidQt.API.TSVSerialiser(sec[0])
        tmpTSV.selectLine("members")
        num = tmpTSV.readInt()
        names = []
        for k in range(num):
            names.append(tmpTSV.readString())
        loaded_values = []
        for name in names:
            loaded_values.append(copy.deepcopy(value[0]))
            loaded_values[-1].load(tmpTSV, name)
        self.common_context[key] = loaded_values
