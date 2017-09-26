from __future__ import (absolute_import, division, print_function)
from Muon import MaxEnt_model

"""
 simple class to create a single object
 containing all of the models.
 Only need to pass a single object to all
 presenters
"""


class ModelConstructor(object):

    def __init__(self,includeTransform):
        # construct transformation memebers
        if includeTransform:
            self.transformModels={}
            alg = MaxEnt_model.MaxEntModel()
            self.transformModels["MaxEnt"]=MaxEnt_model.MaxEntThread(alg)

    def getModel(self,name):
        return self.transformModels[name]
