import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class FindReflectometryLinesTest(unittest.TestCase):

    _no_peak_ws = None
    _one_peak_ws = None
    _two_peak_ws = None
    _three_peak_ws = None

    def setUp(self):
        if self.__class__._no_peak_ws == None:
            # Create test workspace with no peaks
            dataX = [0, 1]
            dataY = [0]
            dataE = [0]
            nSpec = 1
            no_peak_ws_alg = run_algorithm("CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="no_peak_ws")
            self.__class__._no_peak_ws = no_peak_ws_alg.getPropertyValue("OutputWorkspace")

            # Create test workspace with a single peak
            dataX = [0, 1, 0, 1, 0, 1] # Setup enough X values {0, 1} to create a histo workspace with a single bin.
            dataY = [0, 1, 0] # One real peak
            dataE = [0, 0, 0] # Errors are not considered in the algorithm
            nSpec = 3
            one_peak_ws_alg = run_algorithm("CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="one_peak_ws")
            self.__class__._one_peak_ws = one_peak_ws_alg.getPropertyValue("OutputWorkspace")

            # Create test workspace with two peaks
            dataX = [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1] # Setup enough X values {0, 1} to create a histo workspace with a single bin.
            dataY = [0.1, 1, 0.1, 0.2, 0.1, 2, 0.1] # Real peaks with values 1, 2, false peak with value 0.2
            dataE = [0, 0, 0, 0, 0, 0, 0] # Errors are not considered in the algorithm
            nSpec = 7
            two_peak_ws_alg = run_algorithm("CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="two_peak_ws")
            self.__class__._two_peak_ws = two_peak_ws_alg.getPropertyValue("OutputWorkspace")

            # Create test workspace with three peaks
            dataX = [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1] # Setup enough X values {0, 1} to create a histo workspace with a single bin.
            dataY = [0, 1, 0, 1, 0, 1, 0, 1, 0] # 3 real peaks
            dataE = [0, 0, 0, 0, 0, 0, 0, 0, 0] # Errors are not considered in the algorithm
            nSpec = 9
            three_peak_ws_alg = run_algorithm("CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="three_peak_ws")
            self.__class__._three_peak_ws = three_peak_ws_alg.getPropertyValue("OutputWorkspace")

    def test_inputs_ws_not_in_wavelength(self):
        units = "TOF"
        run_algorithm("CreateWorkspace", DataX=[0, 1], DataY=[0], DataE=[0], NSpec=1, UnitX=units, VerticalAxisUnit="SpectraNumber", OutputWorkspace="in_tof")
        try:
            # This should throw, because input units are in TOF, not wavelength!
            alg = run_algorithm('FindReflectometryLines', InputWorkspace='in_tof' , OutputWorkspace='spectrum_numbers', child=True)
        except ValueError:
            self.assertTrue(True)
        else:
            self.assertTrue(False)

    def test_start_wavlength_too_low(self):
        try:
            # Negative value choosen for start wavelength, should throw!
            alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._two_peak_ws , OutputWorkspace='spectrum_numbers', StartWavelength=-1, child=True)
        except ValueError:
            self.assertTrue(True)
        else:
            self.assertTrue(False)

    def test_keep_intermeidate_workspaces_off_by_default(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._two_peak_ws , OutputWorkspace='spectrum_numbers')
        keep_workspaces = alg.getPropertyValue("KeepIntermediateWorkspaces")
        # Should have requested that no intermediate workspaces have been kept, these should then have all been deleted too.
        self.assertEquals("0", keep_workspaces)
        self.assertTrue(not mtd.doesExist('cropped_ws'))
        self.assertTrue(not mtd.doesExist('summed_ws'))
        #clean-up
        mtd.remove('spectrum_numbers')

    def test_switch_intermediate_workspaces_on(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._two_peak_ws , OutputWorkspace='spectrum_numbers', KeepIntermediateWorkspaces=True)
        self.assertTrue(mtd.doesExist('spectrum_numbers'))
        self.assertTrue(mtd.doesExist('cropped_ws'))
        self.assertTrue(mtd.doesExist('summed_ws'))
        #clean-up
        mtd.remove('cropped_ws')
        mtd.remove('summed_ws')
        mtd.remove('spectrum_numbers')

    def test_find_no_peaks_throws(self):
        try:
            alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._no_peak_ws , OutputWorkspace='spectrum_numbers', child=True)
        except RuntimeError as error:
            self.assertTrue(True) #We expect the algorithm to fail if no peaks are found.
        else:
            self.assertTrue(False)

    def test_find_three_peaks_throws(self):
        try:
            alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._three_peak_ws , OutputWorkspace='spectrum_numbers', child=True)
        except RuntimeError as error:
            self.assertTrue(True) #We expect the algorithm to fail if three peaks are found.
        else:
            self.assertTrue(False)

    def test_find_one_peaks(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._one_peak_ws, OutputWorkspace='spectrum_numbers')
        spectrum_workspace = mtd.retrieve('spectrum_numbers')

        # Should create a table with a SINGLE column (reflected spectra #) and one row.
        self.assertEquals(1, spectrum_workspace.columnCount())
        self.assertEquals(1, spectrum_workspace.rowCount())
        self.assertEquals(2, spectrum_workspace.cell(0,0)) # Match of exact spectrum that should be found

    def test_find_two_peaks(self):
        alg = run_algorithm('FindReflectometryLines', InputWorkspace=self.__class__._two_peak_ws, OutputWorkspace='spectrum_numbers')
        spectrum_workspace = mtd.retrieve('spectrum_numbers')

        # Should create a table with a TWO columns (reflected spectra #, transmission spectra #) and one row.
        self.assertEquals(2, spectrum_workspace.columnCount())
        self.assertEquals(1, spectrum_workspace.rowCount())
        self.assertEquals(6, spectrum_workspace.cell(0,0)) # Match of exact spectrum that should be found
        self.assertEquals(2, spectrum_workspace.cell(0,1)) # Match of exact spectrum that should be found

if __name__ == '__main__':
    unittest.main()