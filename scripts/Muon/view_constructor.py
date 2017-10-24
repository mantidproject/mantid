from __future__ import (absolute_import, division, print_function)

from Muon import fft_view
from Muon import maxent_view
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
            self.transformMethods["FFT"] = fft_view.FFTView(parent)
            self.transformMethods["MaxEnt"] = maxent_view.MaxEntView(parent)
            # create default transform selection widget
            self.transformSelector = transform_selection_view.TransformSelectionView(parent)

    def getTransformMethods(self):
        return self.transformMethods

    def getTransformSelection(self):
        return self.transformSelector
