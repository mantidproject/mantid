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
import numpy as np
from AbsorptionShapes import (anAbsorptionShape,Cylinder)

class AdsorbtionShapesTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(AdsorbtionShapesTest, self).__init__(methodName)

    def test_an_Absrpn_shape_parent(self):
        ash = anAbsorptionShape(['V']);
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

        self.assertRaises(TypeError,anAbsorptionShape.material.__set__,ash,[1,2,3])
        self.assertRaises(TypeError,anAbsorptionShape.material.__set__,ash,[1,2])

    def test_adsrp_cylinder(self):
        ash = Cylinder('V',[10,2])
        res = ash.cylinder_shape
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['Radius'],2)

        ash.cylinder_shape = [5,1,[0,1,0],[0.,0.,-0.5]]
        res = ash.cylinder_shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Radius'],1)
        self.assertEqual(res['Axis'],[0,1,0])
        self.assertEqual(res['Center'],[0,0,-0.5])

        ash.cylinder_shape = {'Height':50,'Radius':10,'Axis':[1,1,0],'Center':[0.,0.,0.]}
        res = ash.cylinder_shape;
        self.assertEqual(res['Height'],50)
        self.assertEqual(res['Radius'],10)
        self.assertEqual(res['Axis'],[1,1,0])
        self.assertEqual(res['Center'],[0,0,0])

        test_ws = CreateSampleWorkspace()
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)
        cor_ws,corrections = ash.correct_absorption(test_ws)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97,0.0006,0],4)
        mccor_ws,mc_corr = ash.correct_absorption(test_ws,is_mc=True,NumberOfWavelengthPoints=20)
        n_bins = mc_corr.blocksize()
        mccorr_ranges = [n_bins,mc_corr.readY(0)[0],mc_corr.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(mccorr_ranges ,[97,0.0042,0.0001],4)



if __name__=="__main__":
    #unittest.main()
    ast = AdsorbtionShapesTest('test_adsrp_cylinder')
    ast.test_adsrp_cylinder()
