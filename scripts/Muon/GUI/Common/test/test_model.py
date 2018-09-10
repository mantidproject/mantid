from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common import group_object

from Muon.GUI.Common.muon_context.muon_context import *

class TestModel(object):

    def __init__(self,context):
        self._context = context

    def getSubContext(self):
        group_names = []
        tmp = self._context.get(Groups)
        for group in tmp:
            group_names.append(group.name)

        return group_names
