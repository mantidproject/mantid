from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common import group_object
from Muon.GUI.Common import pair_object

from Muon.GUI.Common.muon_context.muon_context import *

class TestModel(object):

    def __init__(self,context):
        self._context = context

    def getSubContext(self):
        subContext = {}
        group_names = []
        tmp = self._context.get(Groups)
        for group in tmp:
            group_names.append(group.name)
        subContext["Group Names"] = group_names
        pair = self._context.get(Pairs)[0] # there is only one
        subContext["Pair_F"] = pair.getFGroup()
        subContext["Pair_B"] = pair.getBGroup()
        subContext["Pair_alpha"] = pair.getAlpha()
        
        return subContext

    def updateContext(self,subContext):
        group_names = subContext["Group Names"]
        groups = []
        for name in group_names:
           groups.append(group_object.group(name))
        self._context.set(Groups,groups)

        alpha = subContext["Pair_alpha"]
        F_group = subContext["Pair_F"]
        B_group = subContext["Pair_B"]
        name = "pair test"
        pair = pair_object.pair(name,F_group,B_group,alpha)
        self._context.set(Pairs,[pair])

    def getContext(self):
        return self._context 
