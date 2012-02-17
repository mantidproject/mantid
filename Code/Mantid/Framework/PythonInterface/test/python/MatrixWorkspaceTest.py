import unittest,sys

from testhelpers import run_algorithm, can_be_instantiated

from mantid.api import (MatrixWorkspace, WorkspaceProperty_Workspace, Workspace,
                        ExperimentInfo, AnalysisDataService)
from mantid.geometry import Detector

import numpy as np

class MatrixWorkspaceTest(unittest.TestCase):

    _test_ws_prop = None
    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            wsname = 'LOQ48127'
            alg = run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=2, child=True)
            self.__class__._test_ws_prop = alg.getProperty('OutputWorkspace')
            self.__class__._test_ws = alg.getProperty('OutputWorkspace').value

    def test_that_one_cannot_be_instantiated_directly(self):
        self.assertFalse(can_be_instantiated(MatrixWorkspace))
        
    def test_hierarchy_is_as_expected(self):
        self.assertTrue(issubclass(MatrixWorkspace, ExperimentInfo))
        self.assertTrue(issubclass(MatrixWorkspace, Workspace))

    def test_meta_information(self):
        self.assertEquals(self._test_ws.id(), "Workspace2D")
        self.assertEquals(self._test_ws.name(), "")
        self.assertEquals(self._test_ws.getTitle().rstrip(), "direct beam") #The title seems to have a newline in it
        self.assertEquals(self._test_ws.getComment(), "")
        self.assertEquals(self._test_ws.isDirty(), False)
        self.assertTrue(self._test_ws.getMemorySize() > 0.0)
        self.assertEquals(self._test_ws.threadSafe(), True)
        
    def test_workspace_data_information(self):
        self.assertEquals(self._test_ws.getNumberHistograms(), 2)
        self.assertEquals(self._test_ws.blocksize(), 102)
        self.assertEquals(self._test_ws.YUnit(), "Counts")
        self.assertEquals(self._test_ws.YUnitLabel(), "Counts")

    def test_axes(self):
        # Workspace axes
        self.assertEquals(self._test_ws.axes(), 2)
        xaxis = self._test_ws.getAxis(0)
        yaxis = self._test_ws.getAxis(1)
        
        self.assertTrue(xaxis.isNumeric())
        self.assertTrue(yaxis.isSpectra())
        
        self.assertEquals(xaxis.length(), 103)
        self.assertEquals(yaxis.length(), 2)
        
        xunit = xaxis.getUnit()
        self.assertEquals(xunit.caption(), "Time-of-flight")
        self.assertEquals(xunit.label(), "microsecond")
        self.assertEquals(xunit.unitID(), "TOF")

        yunit = yaxis.getUnit()
        self.assertEquals(yunit.caption(), "")
        self.assertEquals(yunit.label(), "")
        self.assertEquals(yunit.unitID(), "Empty")

        
    def test_detector_retrieval(self):
        det = self._test_ws.getDetector(0)
        self.assertTrue(isinstance(det, Detector))
        self.assertEquals(det.getID(), 1)
        self.assertFalse(det.isMasked())

    def test_spectrum_retrieval(self):
        # Spectrum
        spec = self._test_ws.getSpectrum(1)
        self.assertEquals(spec.getSpectrumNo(), 2)
        self.assertTrue(spec.hasDetectorID(2))
        ids = spec.getDetectorIDs()
        expected = [2]
        self.assertEquals(len(expected), len(ids))
        for i in range(len(ids)):
            self.assertEquals(expected[i], ids[i])

    def test_that_a_histogram_workspace_is_returned_as_a_MatrixWorkspace_from_a_property(self):
        self.assertEquals(type(self._test_ws_prop), WorkspaceProperty_Workspace)
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(self._test_ws, Workspace))
        # Have got a MatrixWorkspace back and not just the generic interface
        self.assertEquals(type(self._test_ws), MatrixWorkspace)
        mem = self._test_ws.getMemorySize()
        self.assertTrue( (mem > 0) )

    def test_read_data_members_give_readonly_numpy_array(self):
        def do_numpy_test(arr):
            self.assertEquals(type(arr), np.ndarray)
            self.assertFalse(arr.flags.writeable)

        x = self._test_ws.readX(0)
        y = self._test_ws.readY(0)
        e = self._test_ws.readE(0)
        dx = self._test_ws.readDx(0)
        
        for attr in [x,y,e,dx]:
            do_numpy_test(attr)

    def test_data_can_be_extracted_to_numpy_successfully(self):
        x = self._test_ws.extractX()
        y = self._test_ws.extractY()
        e = self._test_ws.extractE()
        dx = self._test_ws.extractDx()
        
        self.assertTrue(len(dx), 0)
        self._do_numpy_comparison(self._test_ws, x, y, e)

    def _do_numpy_comparison(self, workspace, x_np, y_np, e_np, index = None):
        if index is None:
            nhist = workspace.getNumberHistograms()
            start = 0
            end = nhist
        else:
            nhist = 1
            start = index
            end = index+nhist
            
        blocksize = workspace.blocksize()
        for arr in (x_np, y_np, e_np):
            self.assertEquals(type(arr), np.ndarray)
        
        if nhist > 1:
            self.assertEquals(x_np.shape, (nhist, blocksize + 1)) # 2 rows, 103 columns
            self.assertEquals(y_np.shape, (nhist, blocksize)) # 2 rows, 102 columns
            self.assertEquals(e_np.shape, (nhist, blocksize)) # 2 rows, 102 columns
        else:
            self.assertEquals(x_np.shape, (blocksize + 1,)) # 2 rows, 103 columns
            self.assertEquals(y_np.shape, (blocksize,)) # 2 rows, 102 columns
            self.assertEquals(e_np.shape, (blocksize,)) # 2 rows, 102 columns
            
        
        for i in range(start, end):
            if nhist > 1:
                x_arr = x_np[i] 
                y_arr = y_np[i]
                e_arr = e_np[i]
            else:
                x_arr = x_np
                y_arr = y_np
                e_arr = e_np
            for j in range(blocksize):
                self.assertEquals(x_arr[j], workspace.readX(i)[j])
                self.assertEquals(y_arr[j], workspace.readY(i)[j])
                self.assertEquals(e_arr[j], workspace.readE(i)[j])
            # Extra X boundary
            self.assertEquals(x_arr[blocksize], workspace.readX(i)[blocksize])
            
    def test_data_members_give_writable_numpy_array(self):
        def do_numpy_test(arr):
            self.assertEquals(type(arr), np.ndarray)
            self.assertTrue(arr.flags.writeable)
            
        x = self._test_ws.dataX(0)
        y = self._test_ws.dataY(0)
        e = self._test_ws.dataE(0)
        dx = self._test_ws.dataDx(0)
        
        for attr in [x,y,e,dx]:
            do_numpy_test(attr)
        
        self.assertTrue(len(dx), 0)
        self._do_numpy_comparison(self._test_ws, x, y, e, 0)
        
        # Can we change something
        ynow = y[0]
        ynow *= 2.5
        y[0] = ynow
        self.assertEquals(self._test_ws.readY(0)[0], ynow)
        
            
    def test_operators_with_workspaces_in_ADS(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='a',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        ads = AnalysisDataService.Instance()
        A = ads['a']
        run_algorithm('CreateWorkspace', OutputWorkspace='b', DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        B = ads['b']
        
        # Equality
        self.assertTrue(A.equals(B, 1e-8))
        # Two workspaces
        C = A + B
        C = A - B
        C = A * B
        C = A / B
        C -= B
        C += B
        C *= B
        C /= B
        
        # Workspace + double
        B = 123.456
        C = A + B
        C = A - B
        C = A * B
        C = A / B

        ads.remove('C')
        self.assertTrue('C' not in ads)
        run_algorithm('CreateWorkspace', OutputWorkspace='ca', DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        C = ads['ca']

        C *= B
        self.assertTrue('C' not in ads)
        C -= B
        self.assertTrue('C' not in ads)
        C += B
        self.assertTrue('C' not in ads)
        C /= B
        self.assertTrue('C' not in ads)
        # Check correct in place ops have been used
        self.assertTrue('ca' in ads)
        ads.remove('ca')

        # Commutative: double + workspace
        C = B * A
        C = B + A
        
        ads.remove('A')
        ads.remove('B')
        ads.remove('C')
        
    def test_history_access(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='raw',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        run_algorithm('Rebin', InputWorkspace='raw', Params=[1.,0.5,3.],OutputWorkspace='raw')
        ads = AnalysisDataService.Instance()
        raw = ads['raw']
        history = raw.getHistory()
        last = history.lastAlgorithm()
        self.assertEquals(last.name(), "Rebin")
        self.assertEquals(last.getPropertyValue("InputWorkspace"), "raw")
        first = history[0]
        self.assertEquals(first.name(), "CreateWorkspace")
        self.assertEquals(first.getPropertyValue("OutputWorkspace"), "raw")
        ads.remove('raw')

if __name__ == '__main__':
    unittest.main()
            
