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
from Direct.AbsorptionShapes import (anAbsorptionShape,Cylinder,FlatPlate,HollowCylinder,Sphere)

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

        ash = anAbsorptionShape({'AtomicNumber':12,'AttenuationXSection':0.5,'SampleMassDensity':120}) 
        res = ash.material;
        self.assertEqual(res['AtomicNumber'],12)
        self.assertEqual(res['AttenuationXSection'],0.5)
        self.assertEqual(res['SampleMassDensity'],120)

        # Add extra material property, consistent with other properties.
        ash.material = {'ScatteringXSection':20}
        res = ash.material;
        self.assertEqual(res['AttenuationXSection'],0.5)
        self.assertEqual(res['ScatteringXSection'],20)
        self.assertEqual(len(res),4)

    def test_adsrp_cylinder(self):
        ash = Cylinder('V',[10,2])
        res = ash.shape
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['Radius'],2)
        self.assertTrue(ash._axis_is_default)

        ash.shape = [5,1,[0,1,0],[0.,0.,-0.5]]
        res = ash.shape
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Radius'],1)
        self.assertEqual(res['Axis'],[0,1,0])
        self.assertEqual(res['Center'],[0,0,-0.5])
        self.assertFalse(ash._axis_is_default)


        ash.shape = {'Height':5,'Radius':2,'Axis':[1,0,0],'Center':[0.,0.,0.]}
        res = ash.shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Radius'],2)
        self.assertEqual(res['Axis'],[1,0,0])
        self.assertEqual(res['Center'],[0,0,0])

        test_ws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)
        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97,0.0,0.0],4)
        mccor_ws,mc_corr = ash.correct_absorption(test_ws,NumberOfWavelengthPoints=20)
        n_bins = mc_corr.blocksize()
        mccorr_ranges = [n_bins,mc_corr.readY(0)[0],mc_corr.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(mccorr_ranges ,[97,0.2657,0.0271],4)

    def test_MARI_axis_cylinder(self):
        """ Test that default axis for MARI is different"""
        ash = Cylinder('Fe',[10,2])
        res = ash.shape
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['Radius'],2)
        self.assertTrue(ash._axis_is_default)
        test_ws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)
        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True)
        res = ash.shape
        self.assertEqual(res['Axis'],[0.,1.,0])

        EditInstrumentGeometry (test_ws,[1],[0],InstrumentName='MARI')
        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True)
        res = ash.shape
        self.assertEqual(res['Axis'],[1.,0.,0])


    def test_adsrp_Plate(self):
        ash = FlatPlate('V',[10,2,0.1])
        res = ash.shape
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['Width'],2)
        self.assertEqual(res['Thick'],0.1)

        ash.shape = [5,1,0.2,[0,1,0],10]
        res = ash.shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Width'],1)
        self.assertEqual(res['Thick'],0.2)
        self.assertEqual(res['Center'],[0,1,0])
        self.assertEqual(res['Angle'],10)

        ash.shape = {'Height':5,'Width':1,'Thick':2,'Center':[0.,0.,0.],'Angle':20}
        res = ash.shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['Width'],1)
        self.assertEqual(res['Thick'],2)
        self.assertEqual(res['Center'],[0,0,0])
        self.assertEqual(res['Angle'],20)

        test_ws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)

        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True,ElementSize=5)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97, 0., 0.],4)

        mccor_ws,mc_corr = ash.correct_absorption(test_ws,is_mc=True,NumberOfWavelengthPoints=20)
        n_bins = mc_corr.blocksize()
        mccorr_ranges = [n_bins,mc_corr.readY(0)[0],mc_corr.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(mccorr_ranges ,[97,0.5253,0.1296],4)


    def test_adsrp_hollow_cylinder(self):
        ash = HollowCylinder('V',[10,2,4])
        res = ash.shape
        self.assertEqual(res['Height'],10)
        self.assertEqual(res['InnerRadius'],2)
        self.assertEqual(res['OuterRadius'],4)

        ash.shape = [5,1,2,[1,0,0],[0,0,0]]
        res = ash.shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['InnerRadius'],1)
        self.assertEqual(res['OuterRadius'],2)
        self.assertEqual(res['Axis'],[1,0,0])
        self.assertEqual(res['Center'],[0,0,0])


        ash.shape = {'Height':5,'InnerRadius':0.01,'OuterRadius':2,'Center':[0.,0.,0.],'Axis':[0,1,0]}
        res = ash.shape;
        self.assertEqual(res['Height'],5)
        self.assertEqual(res['InnerRadius'],0.01)
        self.assertEqual(res['OuterRadius'],2)
        self.assertEqual(res['Axis'],[0,1,0])
        self.assertEqual(res['Center'],[0,0,0])


        test_ws =  CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)
        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True,ElementSize=5)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97,0.0,0.000],4)

        mccor_ws,mc_corr = ash.correct_absorption(test_ws,is_mc=True,NumberOfWavelengthPoints=20)
        n_bins = mc_corr.blocksize()
        mccorr_ranges = [n_bins,mc_corr.readY(0)[0],mc_corr.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(mccorr_ranges ,[97,0.2657,0.0303],4)
    #
    def test_string_conversion(self):
        """ check if shape conversion to string representation works"""
        ash = HollowCylinder('V',[10,2,4])
        ash_str = str(ash)
        ash_rec = anAbsorptionShape.from_str(ash_str)

        self.assertTrue(isinstance(ash_rec,HollowCylinder))
        self.assertDictEqual(ash.material,ash_rec.material)
        self.assertDictEqual(ash.shape,ash_rec.shape)

        ash = Sphere(['Al',10],10)
        ash_str = str(ash)
        ash_rec = anAbsorptionShape.from_str(ash_str)

        self.assertTrue(isinstance(ash_rec,Sphere))
        self.assertDictEqual(ash.material,ash_rec.material)
        self.assertDictEqual(ash.shape,ash_rec.shape)

    #
    def test_adsrp_sphere(self):
        ash = Sphere('Al',10)
        res = ash.shape
        self.assertEqual(res['Radius'],10)

        ash.shape = [5,[1,0,0]]
        res = ash.shape;
        rad = res['Radius']
        self.assertEqual(rad,5)
        cen = res['Center']
        self.assertEqual(cen,[1,0,0])


        ash.shape = {'Radius':3,'Center':[0.,0.,0.]}
        res = ash.shape;
        rad = res['Radius']
        self.assertEqual(rad,3)
        cen = res['Center']
        self.assertEqual(cen,[0,0,0])


        test_ws =  CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)
        test_ws = ConvertUnits(test_ws,'DeltaE',Emode='Direct',EFixed=2000)
        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True,ElementSize=6)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97,0.0,0.0],4)

        cor_ws,corrections = ash.correct_absorption(test_ws,is_fast=True)
        n_bins = corrections.blocksize()
        corr_ranges = [n_bins,corrections.readY(0)[0],corrections.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(corr_ranges,[97,0.0,0.0],4)
 
        mccor_ws,mc_corr = ash.correct_absorption(test_ws,is_mc=True,NumberOfWavelengthPoints=20)
        n_bins = mc_corr.blocksize()
        mccorr_ranges = [n_bins,mc_corr.readY(0)[0],mc_corr.readY(0)[n_bins-1]]
        np.testing.assert_almost_equal(mccorr_ranges ,[97,0.6645,0.5098],4)



if __name__=="__main__":
    #ast = AdsorbtionShapesTest('test_adsrp_cylinder')
    #ast.test_adsrp_cylinder()
    unittest.main()
