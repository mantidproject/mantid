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




class anAbsrpnShape(object):
    """ The parent class for all shapes, used to perform various adsorption corrections
    in direct inelastic analysis.

    Contains material and environment properties. Environment is not yet used.

    Usage:
    anShape = anAdsrbShape([ChemicalFormula,NumberDensity])
        where:
        -- ChemicalFormula is the string, describing the sample formula
        -- NumberDensity   is the number density of the sample in number
           of atoms or formula units per cubic Angstrom. If provided, will be used
           instead of the one, calculated from the Chemical formula
        e.g:
        anShape = anAdsrbShape(['Li',0.1])
    or 
    anShape = anAdsrbShape({'ChemicalFormula':'Formula','NumberDensity':Number}) 
        e.g:
        anShape = anAdsrbShape({'ChemicalFormula':'(Li7)2-C-H4-N-Cl6','NumberDensity':0.8}) 
    or 
    anShape = anAdsrbShape(Dictionary with any number of key-value pairs recognized by SetSampleMaterial algorithm)
    the value should unambiguously define sample material for absorption calculations.

    The set material value may be accessed or changes through the property material: e.g.
    mat = anShape.material
    or 
    anShape.material = {'ChemicalFormula','Br'}
    """

    def __init__(self,MaterialValue=None):
        # environment according to existing definitions of the environment
        self._Environment=None
        # default sample material (formula)
        self._Material=None
        # the shape holder. Not defined in this class but the holder for all other classes
        self._ShapeDescription = None
        # the workspace used for testing correct properties 
        rhash = random.randint(1,100000)
        self._testWorkspace = CreateSampleWorkspace(OutputWorkspace='_adsShape_'+str(rhash))
        if MaterialValue is not None:
            self.material = MaterialValue
    #
    def __del__(self):
        DeleteWorkspace(self._testWorkspace)
    #
    @property
    def material(self):
        return self._Material
    @material.setter
    def material(self,value):
        if value is None:
            self._Material = None;
            return
        if isinstance(value,str):
            value = [value]
        if isinstance(value,(list,tuple)):
            if len(value) == 2:
                self._Material = {'ChemicalFormula':value[0],'SampleNumberDensity':float(value[1])}
            elif len(value) == 1:
                self._Material = {'ChemicalFormula':value[0]}
            else:
                raise TypeError('*** If material is defined by list or tuple, it may consist of 1 or 2 members, '\
                    'defining the material formula and material number density')
        elif isinstance(value,dict):
            for key,val in value.iteritems():
                self._Material[key]  = val
        else:
            raise TypeError('*** Material accepts only list or tuple containing up to 2 values'\
                ' corresponding to material formula and material number density or dictionary with keys,'\
                ' recognized by SetSampleMaterial algorithm')
        #
        Mater_properties = self._Material
        if not isinstance(Mater_properties['ChemicalFormula'],str):
            raise TypeError('*** The chemical formula for the material must be described by a string')
        # Test if the material property are recognized by SetSampleMaterial algorithm.
        SetSampleMaterial(self._testWorkspace,**Mater_properties)
    #
    def correct_adsorption_fast(self,ws):
        """ Method to correct adsorption on a shape using fast (analytical) method if such method is available 
        
        Not available on arbitrary shapes
        """
        raise RuntimeError('*** The correct_adsorbtion_fast is not implemented for an abstract shape')

    def correct_adsorption_MC(self,ws,**argi):
        """
        """
        raise RuntimeError('*** Monte-Carlo adsorption corrections are not implemented for an abstract shape')


class AbsrpnCylinder(anAbsrpnShape):
    """Define the absorption cylinder and calculate adsorbtion corrections from this cylinder
    
    Usage:
    abs = AbsrpnCylinder(SampleMaterial,CylinderParameters)
    Define the absorption cylinder with SampleMaterial defined as described on 
    anAbsrpnShape class and CylinderParameters in the form:
    a) The list:
    CylinderParameters = [Height,Radus,[Axis]] 
    where Height, Radus are the cylinder height and radius in cm and 
    Axis, if present is the direction of the cylinder wrt to the bean direction [0,0,1]
    e.g.:
    abs = AbsrpnCylinder(['Al',0.1],[10,2,[1,0,0],[0,0,0]])
    b) The diary: 
    CylinderParameters = {Height:Height_value,Radus:Radius_value,[Axis:axisValue],[Center:TheSampleCentre]} 
    e.g:
    abs = AbsrpnCylinder(['Al',0.1],[Height:10,Radius:2,Axis:[1,0,0]])

    Correct absorbtion on the cylinder using CylinderAbsorption algorithm:
    ws = abs.correct_adsorption_fast(ws)

    Correct absorbtion on the defined cylinder using Monte-Carlo Absorption algorithm:
    ws = ads.correct_adsorption_MC(ws,{AdditionalMonte-Carlo Absorption parameters});
    """
    def __init__(self,Material=None,CylinderParams=None):

        anAbsrpnShape.__init__(self,Material)
        self.cylinder_shape = CylinderParams
    @property
    def cylinder_shape(self):
        return self._ShapeDescription
    @cylinder_shape.setter
    def cylinder_shape(self,value):
        if value is None:
            self._ShapeDescription = None;
            return
        shape_dict = {'Shape':'Cylinder'};
        if isinstance(value,(list,tuple)):
            keys = ['Height','Radius','Axis','Center']
            for i in range(0,len(value)):
                val = value[i]
                if not isinstance(val,(list,tuple)):
                    val = float(val)
                shape_dict[keys[i]] = val
        elif isinstance(value,dict):
            for key,val in value.iteritems():
                if not isinstance(val,(list,tuple)):
                    val = float(val)
                shape_dict[key]  = val
        else:
            raise TypeError('*** Cylinder shape parameter accepts only list or tuple containing up to 4 values'\
                ' corresponding to cylinder parameters (Height,Radius,Axis and Centre) or dictionary with keys,'\
                ' recognized by SetSample algorithm Geometry property')
        #
        if not 'Center' in shape_dict:
            shape_dict['Center']=[0,0,0]

        self._ShapeDescription = shape_dict

        # Test if the material property are recognized by SetSampleMaterial algorithm.
        SetSample(self._testWorkspace,Geometry=shape_dict)




#-----------------------------------------------------------------
if __name__ == "__main__":
    pass
