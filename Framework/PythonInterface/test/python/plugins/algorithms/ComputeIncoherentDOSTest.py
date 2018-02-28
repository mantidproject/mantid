from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import ComputeIncoherentDOS, CreateSampleWorkspace, LoadInstrument, ScaleX, Scale, SetSampleMaterial, SofQW3, Transpose
import numpy as np
from scipy import constants

class ComputeIncoherentDOSTest(unittest.TestCase):

    def createPhononWS(self, T, en, e_units):
        fn = 'name=Gaussian, PeakCentre='+str(en)+', Height=1, Sigma=0.5;'
        fn +='name=Gaussian, PeakCentre=-'+str(en)+', Height='+str(np.exp(-en*11.6/T))+', Sigma=0.5;'
        ws = CreateSampleWorkspace(binWidth = 0.1, XMin = -25, XMax = 25, XUnit = e_units, Function = 'User Defined', UserDefinedFunction=fn)
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap = True)
        with self.assertRaises(RuntimeError):
            ws_DOS = ComputeIncoherentDOS(ws)
        ws = SofQW3(ws, [0, 0.05, 8], 'Direct', 25)
        qq = np.arange(0, 8, 0.05)+0.025
        for i in range(ws.getNumberHistograms()):
            ws.setY(i, ws.readY(i)*qq[i]**2)
            ws.setE(i, ws.readE(i)*qq[i]**2)
        return ws

    def convertToWavenumber(self, ws):
        mev2cm = (constants.elementary_charge / 1000) / (constants.h * constants.c * 100)
        u0 = ws.getAxis(0).getUnit().unitID()
        u1 = ws.getAxis(1).getUnit().unitID()
        if u0 == 'DeltaE' or u1 == 'DeltaE':
            if u0 == 'MomentumTransfer':
                ws = Transpose(ws)
            ws.getAxis(0).setUnit('DeltaE_inWavenumber')
            ws = ScaleX(ws, mev2cm)
            ws = Scale(ws, 1/mev2cm)
            if u0 == 'MomentumTransfer':
                ws = Transpose(ws)
        return ws

    def test_computeincoherentdos(self):
        ws = CreateSampleWorkspace()
        # Will fail unless the input workspace has Q and DeltaE axes.
        with self.assertRaises(RuntimeError):
            ws_DOS = ComputeIncoherentDOS(ws)
        ws = CreateSampleWorkspace(XUnit = 'DeltaE')
        with self.assertRaises(RuntimeError):
            ws_DOS = ComputeIncoherentDOS(ws)
        # Creates a workspace with two optic phonon modes at +E and -E with Q^2 dependence and population correct for T=300K
        ws = self.createPhononWS(300, 5, 'DeltaE')
        # This should work!
        ws_DOS = ComputeIncoherentDOS(ws)
        self.assertEquals(ws_DOS.getAxis(0).getUnit().unitID(), 'DeltaE')
        self.assertEquals(ws_DOS.getNumberHistograms(), 1)
        # Checks that it works if the workspace has |Q| along x instead of y, and converts energy to wavenumber
        ws = Transpose(ws)
        ws_DOS = ComputeIncoherentDOS(ws, Temperature = 300, EnergyBinning = '-20, 0.2, 20', Wavenumbers = True)
        self.assertEquals(ws_DOS.getAxis(0).getUnit().unitID(), 'DeltaE_inWavenumber')
        self.assertEquals(ws_DOS.blocksize(), 200)
        # Checks that the Bose factor correction is ok.
        dos_eplus = np.max(ws_DOS.readY(0)[100:200])
        dos_eminus = np.max(ws_DOS.readY(0)[:100])
        self.assertAlmostEqual(dos_eplus / dos_eminus, 1., places=1)
        # Check that unit conversion from cm^-1 to meV works and also that conversion to states/meV is done
        ws = self.convertToWavenumber(ws)
        SetSampleMaterial(ws, 'Al')
        ws_DOSn = ComputeIncoherentDOS(ws, EnergyBinning = '-160, 1.6, 160', StatesPerEnergy = True)
        self.assertTrue('states' in ws_DOSn.YUnitLabel())
        self.assertEquals(ws_DOSn.getAxis(0).getUnit().unitID(), 'DeltaE')
        material = ws.sample().getMaterial()
        factor = material.relativeMolecularMass() / (material.totalScatterXSection() * 1000) * 4 * np.pi
        self.assertAlmostEqual(np.max(ws_DOSn.readY(0)) / (np.max(ws_DOS.readY(0))*factor), 1., places=1)

if __name__=="__main__":
    unittest.main()
