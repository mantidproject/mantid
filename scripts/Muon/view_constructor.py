from __future__ import (absolute_import, division, print_function)

from Muon import FFT_view
from Muon import MaxEnt_view
from Muon import transform_selection_view


class ViewConstructor(object):
    """
     simple class to create a single object
     containing all of the views.
     Only need to pass a single object to all
     presenters
    """

    def __init__(self,includeTransform,parent=None ):
        # construct transformation memebers
        if includeTransform:
            self.transformMethods = {}
            self.transformMethods["FFT"] = FFT_view.FFTView(parent)
            self.transformMethods["MaxEnt"] = MaxEnt_view.MaxEntView(parent)
            self.transformSelector = transform_selection_view.TransformSelectionView(parent)

    def getTransformMethods(self):
        return self.transformMethods

    def getTransformSelection(self):
        return self.transformSelector
