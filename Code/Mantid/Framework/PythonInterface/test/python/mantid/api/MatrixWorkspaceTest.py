import unittest
import sys
import math
from testhelpers import create_algorithm, run_algorithm, can_be_instantiated, WorkspaceCreationHelper
from mantid.api import (MatrixWorkspace, MatrixWorkspaceProperty, WorkspaceProperty, Workspace,
                        ExperimentInfo, AnalysisDataService, WorkspaceFactory)
from mantid.geometry import Detector
from mantid.kernel import Direction, V3D

import numpy as np

class MatrixWorkspaceTest(unittest.TestCase):

    _test_ws_prop = None
    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2,102,False) # no monitors

    def test_that_one_cannot_be_instantiated_directly(self):
        self.assertFalse(can_be_instantiated(MatrixWorkspace))

    def test_hierarchy_is_as_expected(self):
        self.assertTrue(issubclass(MatrixWorkspace, ExperimentInfo))
        self.assertTrue(issubclass(MatrixWorkspace, Workspace))

    def test_meta_information(self):
        self.assertEquals(self._test_ws.id(), "Workspace2D")
        self.assertEquals(self._test_ws.name(), "")
        self.assertEquals(self._test_ws.getTitle(), "Test histogram")
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
        self.assertEquals(yunit.caption(), "Spectrum")
        self.assertEquals(yunit.label(), "")
        self.assertEquals(yunit.unitID(), "Label")


    def test_detector_retrieval(self):
        det = self._test_ws.getDetector(0)
        self.assertTrue(isinstance(det, Detector))
        self.assertEquals(det.getID(), 1)
        self.assertFalse(det.isMasked())
        self.assertAlmostEqual(2.71496516, det.getTwoTheta(V3D(0,0,11), V3D(0,0,1)))

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

    def test_detector_two_theta(self):
        det = self._test_ws.getDetector(1)
        two_theta = self._test_ws.detectorTwoTheta(det)
        self.assertAlmostEquals(two_theta,0.01999733,places=8)
        signed_two_theta = self._test_ws.detectorSignedTwoTheta(det)
        self.assertAlmostEquals(signed_two_theta,0.01999733,places=8)

    def test_that_a_histogram_workspace_is_returned_as_a_MatrixWorkspace_from_a_property(self):
        wsname = "MatrixWorkspaceTest_Property"
        AnalysisDataService.add(wsname,self._test_ws)

        alg = create_algorithm("Rebin", InputWorkspace=wsname)
        propValue = alg.getProperty("InputWorkspace").value
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(propValue, Workspace))
        # Have got a MatrixWorkspace back and not just the generic interface
        self.assertEquals(type(propValue), MatrixWorkspace)
        mem = propValue.getMemorySize()
        self.assertTrue( (mem > 0) )

        AnalysisDataService.remove(wsname)

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

    def test_setting_spectra_from_array_of_incorrect_length_raises_error(self):
        nvectors = 2
        xlength = 11
        ylength = 10
        test_ws = WorkspaceFactory.create("Workspace2D", nvectors, xlength, ylength)

        values = np.arange(xlength + 1)
        self.assertRaises(ValueError, test_ws.setX, 0, values)
        self.assertRaises(ValueError, test_ws.setY, 0, values)
        self.assertRaises(ValueError, test_ws.setE, 0, values)

    def test_setting_spectra_from_array_of_incorrect_shape_raises_error(self):
        nvectors = 2
        xlength = 11
        ylength = 10
        test_ws = WorkspaceFactory.create("Workspace2D", nvectors, xlength, ylength)

        values = np.linspace(0,1,num=xlength-1)
        values = values.reshape(5,2)
        self.assertRaises(ValueError, test_ws.setX, 0, values)
        self.assertRaises(ValueError, test_ws.setY, 0, values)
        self.assertRaises(ValueError, test_ws.setE, 0, values)

    def test_setting_spectra_from_array_using_incorrect_index_raises_error(self):
        nvectors = 2
        xlength = 11
        ylength = 10

        test_ws = WorkspaceFactory.create("Workspace2D", nvectors, xlength, ylength)
        xvalues = np.arange(xlength)
        self.assertRaises(RuntimeError, test_ws.setX, 3, xvalues)

    def test_setting_spectra_from_array_sets_expected_values(self):
        nvectors = 2
        xlength = 11
        ylength = 10

        test_ws = WorkspaceFactory.create("Workspace2D", nvectors, xlength, ylength)
        ws_index = 1

        values = np.linspace(0,1,xlength)
        test_ws.setX(ws_index, values)
        ws_values = test_ws.readX(ws_index)
        self.assertTrue(np.array_equal(values, ws_values))

        values = np.ones(ylength)
        test_ws.setY(ws_index, values)
        ws_values = test_ws.readY(ws_index)
        self.assertTrue(np.array_equal(values, ws_values))

        values = np.sqrt(values)
        test_ws.setE(ws_index, values)
        ws_values = test_ws.readE(ws_index)
        self.assertTrue(np.array_equal(values, ws_values))

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
        ads = AnalysisDataService
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
        self.assertTrue(isinstance(C, MatrixWorkspace))
        C += B
        self.assertTrue(isinstance(C, MatrixWorkspace))
        C *= B
        self.assertTrue(isinstance(C, MatrixWorkspace))
        C /= B
        self.assertTrue(isinstance(C, MatrixWorkspace))

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

    def test_complex_binary_ops_do_not_leave_temporary_workspaces_behind(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='ca', DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        ads = AnalysisDataService
        w1=(ads['ca']*0.0)+1.0

        self.assertTrue('w1' in ads)
        self.assertTrue('ca' in ads)
        self.assertTrue('__python_op_tmp0' not in ads)

    def test_history_access(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='raw',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        run_algorithm('Rebin', InputWorkspace='raw', Params=[1.,0.5,3.],OutputWorkspace='raw')
        raw = AnalysisDataService['raw']
        history = raw.getHistory()
        last = history.lastAlgorithm()
        self.assertEquals(last.name(), "Rebin")
        self.assertEquals(last.getPropertyValue("InputWorkspace"), "raw")
        first = history[0]
        self.assertEquals(first.name(), "CreateWorkspace")
        self.assertEquals(first.getPropertyValue("OutputWorkspace"), "raw")
        AnalysisDataService.remove('raw')

    def test_setTitleAndComment(self):
        run_algorithm('CreateWorkspace', OutputWorkspace='ws1',DataX=[1.,2.,3.], DataY=[2.,3.], DataE=[2.,3.],UnitX='TOF')
        ws1 = AnalysisDataService['ws1']
        title = 'test_title'
        ws1.setTitle(title)
        self.assertEquals(title, ws1.getTitle())
        comment = 'Some comment on this workspace.'
        ws1.setComment(comment)
        self.assertEquals(comment, ws1.getComment())
        AnalysisDataService.remove(ws1.getName())

if __name__ == '__main__':
    unittest.main()

