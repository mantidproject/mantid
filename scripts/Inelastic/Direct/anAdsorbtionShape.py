# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

#pylind: disable=attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import (AlgorithmManager, Algorithm)
import random




class anAdsorbtionShape(object):
    """ The parent class for all shapes, used to perform various adsorbtion corrections
    in direct inelastic analysis.

    Contains material and enviroment properties.
    """

    def __init__(obj,MaterialValue=None):
        # enviroment according to existing definitions of the enviroment
        obj._Environment=None
        # devault sample material (formula)
        obj._Material=None
        # the workspace used for testing correct properties 
        rhash = random.randint(1,100000)
        obj._testWorkspace = CreateSampleWorkspace(OutputWorkspace='_adsShape_'+str(rhash))
        if MaterialValue is not None:
            obj.material = MaterialValue
    #
    def __del__(self):
        DeleteWorkspace(obj._testWorkspace)
    #
    @property
    def material(obj):
        return obj._Material
    @material.setter
    def material(obj,value):
        if value is None:
            obj._Material = None;
        elif isinstance(value,(list,tuple)):
            obj._Material = {'ChemicalFormula':value[0],'SampleNumberDensity':float(value[1])}
        elif isinstance(value,dict):
            for key,val in value.iteritems():
                obj._Material[key]  = val
        else:
            raise TypeError('*** Material property accepts only list, tuple or dictionary consisting of 2 values'\
                ' corresponging to material formula and material density')
        #
        Mater_properties = obj._Material
        SetSampleMaterial(obj._testWorkspace,**Mater_properties)




#-----------------------------------------------------------------
if __name__ == "__main__":
    pass
    #unittest.main()
