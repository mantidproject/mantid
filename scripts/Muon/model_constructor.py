from __future__ import (absolute_import, division, print_function)
from Muon import maxent_model

from Muon import fft_model


class ModelConstructor(object):

    """
     simple class to create a single object
     containing all of the models.
     Only need to pass a single object to all
     presenters
    """

    def __init__(self,includeTransform):
        # construct transformation memebers
        if includeTransform:
            self.transformModels={}

            MaxEnt =maxent_model.MaxEntModel()
            self.transformModels["MaxEnt"]=MaxEnt
            FFT =fft_model.FFTModel()
            FFTWrapper=fft_model.FFTWrapper(FFT)
            self.transformModels[FFT.getName()]=FFTWrapper

    def getModel(self,name):
        return self.transformModels[name]
