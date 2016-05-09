import unittest
from mantid.simpleapi import ComputeIncoherentDOS, CreateSampleWorkspace, LoadInstrument, ScaleX, SetSampleMaterial, SofQW, Transpose
import numpy as np

class ComputeIncoherentDOSTest(unittest.TestCase):

    def test_computeincoherentdos(self):
        ws = CreateSampleWorkspace()
        # Will fail unless the input workspace has Q and DeltaE axes.
        with self.assertRaises(RuntimeError):
            ws_DOS = ComputeIncoherentDOS(ws)
        ws = CreateSampleWorkspace(binWidth = 0.1, XMin = 0, XMax = 50, XUnit = 'DeltaE')
        ws = ScaleX(ws, -25, "Add")
        with self.assertRaises(RuntimeError):
            ws_DOS = ComputeIncoherentDOS(ws)
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap = True)
        ws = SofQW(ws, [0, 0.05, 8], 'Direct', 25)
        # This should work!
        ws_DOS = ComputeIncoherentDOS(ws)
        self.assertEquals(ws_DOS.getAxis(0).getUnit().unitID(), 'DeltaE')
        self.assertEquals(ws_DOS.getNumberHistograms(), 1)
        ws = Transpose(ws)
        ws_DOS = ComputeIncoherentDOS(ws, EnergyBinning = '0, 5, 200', Wavenumbers = True)
        self.assertEquals(ws_DOS.getAxis(0).getUnit().unitID(), 'DeltaE_inWavenumber')
        self.assertEquals(ws_DOS.blocksize(), 40)
        # Check that conversion to states/meV is done
        SetSampleMaterial(ws, 'Al')
        ws_DOSn = ComputeIncoherentDOS(ws, EnergyBinning = '0, 5, 200', Wavenumbers = True, StatesPerEnergy = True)
        self.assertTrue('states' in ws_DOSn.YUnitLabel())
        material = ws.sample().getMaterial()
        factor = material.relativeMolecularMass() / (material.totalScatterXSection() * 1000)
        self.assertAlmostEqual(np.max(ws_DOSn.readY(0)), np.max(ws_DOS.readY(0))*factor, places=4)

if __name__=="__main__":
    unittest.main()
