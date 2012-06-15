import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class FindReflectometryLinesTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            wsname = 'POLREF00004699.nxs'
            load = run_algorithm('LoadNexus', Filename=wsname, OutputWorkspace='test_ws')
	    self.__class__._test_ws = load.getPropertyValue('OutputWorkspace')

    def test_inputs_ws_not_in_wavelength(self):
        run_algorithm("CreateWorkspace", DataX=[0, 1], DataY=[0], DataE=[0], NSpec=1, UnitX="TOF", VerticalAxisUnit="SpectraNumber", OutputWorkspace="in_tof")
        try:
            # This should throw, because input units are in TOF, not wavelength!
            alg = run_algorithm('FindReflectometryLines', InputWorkspace='in_tof' , OutputWorkspace='spectrum_numbers', StartWavelength=10, child=True)
        except ValueError:
            self.assertTrue(True)
        else:
            self.assertTrue(False)

    def test_start_wavlength_too_low(self):
        run_algorithm("CreateWorkspace", DataX=[0, 1], DataY=[0], DataE=[0], NSpec=1, UnitX="TOF", VerticalAxisUnit="SpectraNumber", OutputWorkspace="test_workspace")
        try:
            # Negative value choosen for start wavelength, should throw!
            alg = run_algorithm('FindReflectometryLines', InputWorkspace="test_workspace" , OutputWorkspace='spectrum_numbers', StartWavelength=-1, child=True)
        except ValueError:
            self.assertTrue(True)
        else:
            self.assertTrue(False)
    
    def test_keep_intermeidate_workspaces_off_by_default(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._test_ws , OutputWorkspace='spectrum_numbers', StartWavelength=10)
        keep_workspaces = alg.getPropertyValue("KeepIntermediateWorkspaces")
        # Should have requested that no intermediate workspaces have been kept, these should then have all been deleted too.
        self.assertEquals("0", keep_workspaces)
        self.assertTrue(not mtd.doesExist('cropped_ws'))
        self.assertTrue(not mtd.doesExist('summed_ws'))
        #clean-up
        mtd.remove('spectrum_numbers')
        
    def test_switch_intermediate_workspaces_on(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._test_ws , OutputWorkspace='spectrum_numbers', StartWavelength=10, KeepIntermediateWorkspaces=True)
        self.assertTrue(mtd.doesExist('cropped_ws'))
        self.assertTrue(mtd.doesExist('summed_ws'))
        #clean-up
        mtd.remove('cropped_ws')
        mtd.remove('summed_ws')
        mtd.remove('spectrum_numbers')

    def test_find_no_peaks(self):
        # Fabricate a workspace with no peaks.
        run_algorithm("CreateWorkspace", DataX=[0, 1], DataY=[0], DataE=[0], NSpec=1, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="no_peak_workspace")
        try:
            alg = run_algorithm('FindReflectometryLines', InputWorkspace="no_peak_workspace" , OutputWorkspace='spectrum_numbers', child=True)
        except RuntimeError as error:
            self.assertTrue(True) #We expect the algorithm to fail if no peaks are found.
        else:
            self.assertTrue(False)
        
    def test_fine_one_peaks(self):
        run_algorithm("CropWorkspace", InputWorkspace=self.__class__._test_ws, StartWorkspaceIndex=0, EndWorkspaceIndex=50, OutputWorkspace="one_peak_workspace")
        alg = run_algorithm('FindReflectometryLines', InputWorkspace="one_peak_workspace", OutputWorkspace='spectrum_numbers', StartWavelength=10 )
        spectrum_workspace = mtd.retrieve('spectrum_numbers')
        #For each workspace in the group should create a table with a SINGLE column (reflected spectra #) and one row.
        self.assertEquals(1, spectrum_workspace[0].columnCount())
        self.assertEquals(1, spectrum_workspace[0].rowCount())
        self.assertEquals(1, spectrum_workspace[1].columnCount())
        self.assertEquals(1, spectrum_workspace[1].rowCount())
        self.assertEquals(29, spectrum_workspace[0].cell(0,0)) # Match of exact spectrum that should be found 
        self.assertEquals(29, spectrum_workspace[1].cell(0,0)) # Match of exact spectrum that should be found

    def test_find_two_peaks(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._test_ws, OutputWorkspace='spectrum_numbers', StartWavelength=10 )
        spectrum_workspace = mtd.retrieve('spectrum_numbers')
        #For each workspace in the group should create a table with TWO columns (reflected spectra #, transmission spectra #) and one row.
        self.assertEquals(2, spectrum_workspace[0].columnCount())
        self.assertEquals(1, spectrum_workspace[0].rowCount())
        self.assertEquals(2, spectrum_workspace[1].columnCount())
        self.assertEquals(1, spectrum_workspace[1].rowCount())
        self.assertEquals(29, spectrum_workspace[0].cell(0,0)) # Match of exact spectrum that should be found
        self.assertEquals(29, spectrum_workspace[1].cell(0,0)) # Match of exact spectrum that should be found
        self.assertEquals(75, spectrum_workspace[0].cell(0,1)) # Match of exact spectrum that should be found
        self.assertEquals(74, spectrum_workspace[1].cell(0,1)) # Match of exact spectrum that should be found
    
if __name__ == '__main__':
    unittest.main()