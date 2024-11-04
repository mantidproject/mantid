# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common import group_object
from mantidqtinterfaces.Muon.GUI.Common import pair_object

from mantidqtinterfaces.Muon.GUI.Common.muon_context.muon_context import Groups, Pairs


class ContextExampleModel(object):
    def __init__(self, context):
        self._context = context

    def getSubContext(self):
        subContext = {}
        group_names = []
        group_dets = []
        tmp = self._context.get(Groups)
        for group in tmp:
            group_names.append(group.name)
            group_dets.append(group.dets)
        subContext["Group Names"] = group_names
        subContext["Group dets"] = group_dets

        pair = self._context.get(Pairs)[0]  # there is only one
        # do some validation on pairs
        if pair.FGroup not in group_names:
            pair.setFGroup(group_names[0])
        if pair.BGroup not in group_names:
            pair.setBGroup(group_names[1])

        subContext["Pair_F"] = pair.FGroup
        subContext["Pair_B"] = pair.BGroup
        subContext["Pair_alpha"] = pair.alpha

        return subContext

    def updateContext(self, subContext):
        group_names = subContext["Group Names"]
        group_dets = subContext["Group dets"]
        groups = []
        for k in range(len(group_names)):
            groups.append(group_object.group(group_names[k], group_dets[k]))
        self._context.set(Groups, groups)

        alpha = subContext["Pair_alpha"]
        F_group = subContext["Pair_F"]
        if F_group not in group_names:
            F_group = group_names[0]
        B_group = subContext["Pair_B"]
        if B_group not in group_names:
            B_group = group_names[0]
        name = "pair test"
        pair = pair_object.pair(name, F_group, B_group, alpha)
        self._context.set(Pairs, [pair])

    def getContext(self):
        return self._context
