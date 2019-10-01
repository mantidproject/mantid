# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
from mantid.simpleapi import *
from mantid import api
import unittest
from AbsorptionShapes import (anAbsrpnShape,AbsrpnCylinder)

class AdsorbtionShapesTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(AdsorbtionShapesTest, self).__init__(methodName)

    def test_an_Absrpn_shape_parent(self):
        ash = anAbsrpnShape(['V']);
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'V')

        ash.material = 'Cr'
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'Cr')


        ash.material = ['Br',10]
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'Br')
        self.assertEqual(res['SampleNumberDensity'],10)


        ash.material = {'ChemicalFormula':'Al','SampleNumberDensity':0.5}
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'Al')
        self.assertEqual(res['SampleNumberDensity'],0.5)

        self.assertRaises(TypeError,anAbsrpnShape.material.__set__,ash,[1,2,3])
        self.assertRaises(TypeError,anAbsrpnShape.material.__set__,ash,[1,2])

    def test_adsrp_cylinder(self):
        ash = AbsrpnCylinder('V',[10,2]);
        res = ash.cylinder_shape;
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['Radius'],2)

        ash.cylinder_shape = [5,1,[0,1,0],[0.,0.,-0.5]]
        res = ash.cylinder_shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Radius'],1)
        self.assertEqual(res['Axis'],[0,1,0])
        self.assertEqual(res['Center'],[0,0,-0.5])

        ash.cylinder_shape = {'Height':50,'Radius':10,'Axis':[1,1,0],'Center':[0.,0.,0.5]}
        res = ash.cylinder_shape;
        self.assertEqual(res['Height'],50)
        self.assertEqual(res['Radius'],10)
        self.assertEqual(res['Axis'],[1,1,0])
        self.assertEqual(res['Center'],[0,0,0.5])


if __name__=="__main__":
    #unittest.main()
    ast = AdsorbtionShapesTest('test_adsrp_cylinder')
    ast.test_adsrp_cylinder()
