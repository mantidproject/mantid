# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import math
from testhelpers import create_algorithm, run_algorithm, can_be_instantiated, WorkspaceCreationHelper
from mantid.api import (
    MatrixWorkspace,
    Workspace,
    ExperimentInfo,
    AnalysisDataService,
    WorkspaceFactory,
    NumericAxis,
)
from mantid.geometry import Detector
from mantid.kernel import V3D
from mantid.simpleapi import CreateSampleWorkspace, Rebin
import numpy as np


class MatrixWorkspaceTest(unittest.TestCase):
    _test_ws_prop = None
    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 102, False)  # no monitors

    def test_that_one_cannot_be_instantiated_directly(self):
        self.assertFalse(can_be_instantiated(MatrixWorkspace))

    def test_hierarchy_is_as_expected(self):
        self.assertTrue(issubclass(MatrixWorkspace, ExperimentInfo))
        self.assertTrue(issubclass(MatrixWorkspace, Workspace))

    def test_meta_information(self):
        self.assertEqual(self._test_ws.id(), "Workspace2D")
        self.assertEqual(self._test_ws.name(), "")
        self.assertEqual(self._test_ws.getTitle(), "Test histogram")
        self.assertEqual(self._test_ws.getPlotType(), "plot")
        self.assertEqual(self._test_ws.getComment(), "")
        self.assertEqual(self._test_ws.isDirty(), False)
        self.assertGreater(self._test_ws.getMemorySize(), 0.0)
        self.assertEqual(self._test_ws.threadSafe(), True)
        self.assertEqual(self._test_ws.isGroup(), False)

    def test_workspace_data_information(self):
        self.assertEqual(self._test_ws.getNumberHistograms(), 2)
        self.assertEqual(self._test_ws.blocksize(), 102)
        self.assertEqual(self._test_ws.YUnit(), "Counts")
        self.assertEqual(self._test_ws.YUnitLabel(), "Counts")

    def test_axes(self):
        # Workspace axes
        self.assertEqual(self._test_ws.axes(), 2)
        xaxis = self._test_ws.getAxis(0)
        yaxis = self._test_ws.getAxis(1)

        self.assertTrue(xaxis.isNumeric())
        self.assertTrue(yaxis.isSpectra())

        self.assertEqual(xaxis.length(), 103)
        self.assertEqual(yaxis.length(), 2)

        xunit = xaxis.getUnit()
        self.assertEqual(xunit.caption(), "Time-of-flight")
        self.assertEqual(str(xunit.symbol()), "microsecond")
        self.assertEqual(xunit.unitID(), "TOF")

        yunit = yaxis.getUnit()
        self.assertEqual(yunit.caption(), "Spectrum")
        self.assertEqual(str(yunit.symbol()), "")
        self.assertEqual(yunit.unitID(), "Label")

    def test_replace_axis(self):
        x_axis = NumericAxis.create(1)
        x_axis.setValue(0, 0)
        ws1 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False)
        ws1.replaceAxis(0, x_axis)
        ws2 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False)
        ws2.replaceAxis(0, x_axis)
        try:
            del ws1, ws2
        except:
            self.fail("Segmentation violation when deleting the same axis twice")

    def test_detector_retrieval(self):
        det = self._test_ws.getDetector(0)
        self.assertTrue(isinstance(det, Detector))
        self.assertEqual(det.getID(), 1)
        self.assertAlmostEqual(math.pi, det.getTwoTheta(V3D(0, 0, 11), V3D(0, 0, 1)))

    def test_spectrum_retrieval(self):
        # Spectrum
        spec = self._test_ws.getSpectrum(1)
        self.assertEqual(spec.getSpectrumNo(), 2)
        self.assertTrue(spec.hasDetectorID(2))
        ids = spec.getDetectorIDs()
        expected = [2]
        self.assertEqual(len(expected), len(ids))
        for i in range(len(ids)):
            self.assertEqual(expected[i], ids[i])

    def test_spectrum_numbers_returned(self):
        num_vec = 11
        test_ws = WorkspaceFactory.create("Workspace2D", num_vec, 1, 1)

        spec_nums = test_ws.getSpectrumNumbers()
        self.assertEqual([x for x in range(1, num_vec + 1)], spec_nums)

    def test_detector_two_theta(self):
        det = self._test_ws.getDetector(1)
        two_theta = self._test_ws.detectorTwoTheta(det)
        self.assertAlmostEqual(two_theta, 0.01999733, places=8)
        signed_two_theta = self._test_ws.detectorSignedTwoTheta(det)
        self.assertAlmostEqual(signed_two_theta, 0.01999733, places=8)

    def test_that_a_histogram_workspace_is_returned_as_a_MatrixWorkspace_from_a_property(self):
        wsname = "MatrixWorkspaceTest_Property"
        AnalysisDataService.add(wsname, self._test_ws)

        alg = create_algorithm("Rebin", InputWorkspace=wsname)
        propValue = alg.getProperty("InputWorkspace").value
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(propValue, Workspace))
        # Have got a MatrixWorkspace back and not just the generic interface
        self.assertTrue(isinstance(propValue, MatrixWorkspace))
        mem = propValue.getMemorySize()
        self.assertGreater(mem, 0)

        AnalysisDataService.remove(wsname)

    def test_that_a_histogram_workspace_is_returned_as_a_MatrixWorkspace_from_ADS(self):
        wsname = "MatrixWorkspaceTest_ADS"
        AnalysisDataService.add(wsname, self._test_ws)

        value = AnalysisDataService[wsname]
        self.assertTrue(isinstance(value, Workspace))
        # Have got a MatrixWorkspace back and not just the generic interface
        self.assertTrue(isinstance(value, MatrixWorkspace))
        mem = value.getMemorySize()
        self.assertGreater(mem, 0)

        AnalysisDataService.remove(wsname)

    def test_read_data_members_give_readonly_numpy_array(self):
        def do_numpy_test(arr):
            self.assertEqual(type(arr), np.ndarray)
            self.assertFalse(arr.flags.writeable)

        x = self._test_ws.readX(0)
        y = self._test_ws.readY(0)
        e = self._test_ws.readE(0)
        dx = self._test_ws.readDx(0)

        for attr in [x, y, e, dx]:
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

        values = np.linspace(0, 1, xlength)
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

    def test_setxy_data_coerced_correctly_to_float64(self):
        nbins = 10
        nspec = 2
        xdata = np.arange(nbins + 1)
        ydata = np.arange(nbins)
        ws = WorkspaceFactory.create("Workspace2D", NVectors=nspec, XLength=nbins + 1, YLength=nbins)
        for i in range(nspec):
            ws.setX(i, xdata)
            ws.setY(i, ydata)

        # Verify
        x_expected, y_expected = np.vstack((xdata, xdata)), np.vstack((ydata, ydata))
        x_extracted, y_extracted = ws.extractX(), ws.extractY()
        self.assertTrue(np.array_equal(x_expected, x_extracted))
        self.assertTrue(np.array_equal(y_expected, y_extracted))

    def test_setxye_accepts_python_list(self):
        nbins = 10
        nspec = 2
        xdata = list(range(nbins + 1))
        ydata = list(range(nbins))
        edata = [0.1] * nbins
        ws = WorkspaceFactory.create("Workspace2D", NVectors=nspec, XLength=nbins + 1, YLength=nbins)
        for i in range(nspec):
            ws.setX(i, xdata)
            ws.setY(i, ydata)
            ws.setE(i, edata)
        # Verify
        xdata, ydata, edata = np.array(xdata), np.array(ydata), np.array(edata)
        x_expected, y_expected, e_expected = np.vstack((xdata, xdata)), np.vstack((ydata, ydata)), np.vstack((edata, edata))
        x_extracted, y_extracted, e_extracted = ws.extractX(), ws.extractY(), ws.extractE()
        self.assertTrue(np.array_equal(x_expected, x_extracted))
        self.assertTrue(np.array_equal(y_expected, y_extracted))
        self.assertTrue(np.array_equal(e_expected, e_extracted))

    def test_data_can_be_extracted_to_numpy_successfully(self):
        x = self._test_ws.extractX()
        y = self._test_ws.extractY()
        e = self._test_ws.extractE()
        dx = self._test_ws.extractDx()

        self.assertTrue(len(dx), 0)
        self._do_numpy_comparison(self._test_ws, x, y, e)

    def _do_numpy_comparison(self, workspace, x_np, y_np, e_np, index=None):
        if index is None:
            nhist = workspace.getNumberHistograms()
            start = 0
            end = nhist
        else:
            nhist = 1
            start = index
            end = index + nhist

        blocksize = workspace.blocksize()
        for arr in (x_np, y_np, e_np):
            self.assertEqual(type(arr), np.ndarray)

        if nhist > 1:
            self.assertEqual(x_np.shape, (nhist, blocksize + 1))  # 2 rows, 103 columns
            self.assertEqual(y_np.shape, (nhist, blocksize))  # 2 rows, 102 columns
            self.assertEqual(e_np.shape, (nhist, blocksize))  # 2 rows, 102 columns
        else:
            self.assertEqual(x_np.shape, (blocksize + 1,))  # 2 rows, 103 columns
            self.assertEqual(y_np.shape, (blocksize,))  # 2 rows, 102 columns
            self.assertEqual(e_np.shape, (blocksize,))  # 2 rows, 102 columns

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
                self.assertEqual(x_arr[j], workspace.readX(i)[j])
                self.assertEqual(y_arr[j], workspace.readY(i)[j])
                self.assertEqual(e_arr[j], workspace.readE(i)[j])
            # Extra X boundary
            self.assertEqual(x_arr[blocksize], workspace.readX(i)[blocksize])

    def test_data_members_give_writable_numpy_array(self):
        def do_numpy_test(arr):
            self.assertEqual(type(arr), np.ndarray)
            self.assertTrue(arr.flags.writeable)

        x = self._test_ws.dataX(0)
        y = self._test_ws.dataY(0)
        e = self._test_ws.dataE(0)
        dx = self._test_ws.dataDx(0)

        for attr in [x, y, e, dx]:
            do_numpy_test(attr)

        self.assertTrue(len(dx), 0)
        self._do_numpy_comparison(self._test_ws, x, y, e, 0)

        # Can we change something
        ynow = y[0]
        ynow *= 2.5
        y[0] = ynow
        self.assertEqual(self._test_ws.readY(0)[0], ynow)

    def test_operators_with_workspaces_in_ADS(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="a", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        ads = AnalysisDataService
        A = ads["a"]
        run_algorithm("CreateWorkspace", OutputWorkspace="b", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        B = ads["b"]

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

        ads.remove("C")
        self.assertTrue("C" not in ads)
        run_algorithm("CreateWorkspace", OutputWorkspace="ca", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        C = ads["ca"]

        C *= B
        self.assertTrue("C" not in ads)
        C -= B
        self.assertTrue("C" not in ads)
        C += B
        self.assertTrue("C" not in ads)
        C /= B
        self.assertTrue("C" not in ads)
        # Check correct in place ops have been used
        self.assertTrue("ca" in ads)
        ads.remove("ca")

        # Commutative: double + workspace
        C = B * A
        C = B + A

        ads.remove("A")
        ads.remove("B")
        ads.remove("C")

    def test_complex_binary_ops_do_not_leave_temporary_workspaces_behind(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="ca", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        ads = AnalysisDataService
        w1 = (ads["ca"] * 0.0) + 1.0  # noqa: F841

        self.assertTrue("w1" in ads)
        self.assertTrue("ca" in ads)
        self.assertTrue("__python_op_tmp0" not in ads)

    def test_history_access(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="raw", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        run_algorithm("Rebin", InputWorkspace="raw", Params=[1.0, 0.5, 3.0], OutputWorkspace="raw")
        raw = AnalysisDataService["raw"]
        history = raw.getHistory()
        last = history.lastAlgorithm()
        self.assertEqual(last.name(), "Rebin")
        self.assertEqual(last.getPropertyValue("InputWorkspace"), "raw")
        first = history[0]
        self.assertEqual(first.name(), "CreateWorkspace")
        self.assertEqual(first.getPropertyValue("OutputWorkspace"), "raw")
        AnalysisDataService.remove("raw")

    def test_setTitleAndComment(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="ws1", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        ws1 = AnalysisDataService["ws1"]
        title = "test_title"
        ws1.setTitle(title)
        self.assertEqual(title, ws1.getTitle())
        comment = "Some comment on this workspace."
        ws1.setComment(comment)
        self.assertEqual(comment, ws1.getComment())
        AnalysisDataService.remove(ws1.name())

    def test_setPlotType(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="ws1", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        ws1 = AnalysisDataService["ws1"]

        # test default
        self.assertEqual("plot", ws1.getPlotType())

        # test invalid doesn't take
        self.assertRaisesRegex(ValueError, "Invalid plot type", ws1.setPlotType, "invalid")
        self.assertEqual("plot", ws1.getPlotType())

        # test valid takes
        ws1.setPlotType("marker")
        self.assertEqual("marker", ws1.getPlotType())

    def test_setMarkerStyle(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="ws1", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        ws1 = AnalysisDataService["ws1"]

        # test default
        self.assertEqual("vline", ws1.getMarkerStyle())

        # test invalid doesn't take
        self.assertRaisesRegex(ValueError, "Invalid marker type", ws1.setMarkerStyle, "invalid")
        self.assertEqual("vline", ws1.getMarkerStyle())

        # test valid takes
        ws1.setMarkerStyle("square")
        self.assertEqual("square", ws1.getMarkerStyle())

    def test_setGetMonitorWS(self):
        run_algorithm("CreateWorkspace", OutputWorkspace="ws1", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")
        run_algorithm("CreateWorkspace", OutputWorkspace="ws_mon", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")

        ws1 = AnalysisDataService.retrieve("ws1")
        try:
            monWs = ws1.getMonitorWorkspace()
            GotIt = True
        except RuntimeError:
            GotIt = False
        self.assertFalse(GotIt)

        monWs = AnalysisDataService.retrieve("ws_mon")
        ws1.setMonitorWorkspace(monWs)
        monWs.setTitle("My Fake Monitor workspace")

        monWs1 = ws1.getMonitorWorkspace()
        self.assertEqual(monWs.getTitle(), monWs1.getTitle())

        ws1.clearMonitorWorkspace()
        try:
            monWs1 = ws1.getMonitorWorkspace()
            GotIt = True
        except RuntimeError:
            GotIt = False
        self.assertFalse(GotIt)

        # Check weak pointer issues
        ws1.setMonitorWorkspace(monWs)
        wms = ws1.getMonitorWorkspace()

        allFine = False
        try:
            ws1.setMonitorWorkspace(wms)
            allFine = True
        except ValueError:
            pass
        self.assertTrue(allFine)

    def test_spectrumInfo(self):
        specInfo = self._test_ws.spectrumInfo()
        self.assertEqual(specInfo.isMasked(0), False)
        self.assertEqual(specInfo.isMasked(1), False)

    def test_isCommonBins(self):
        self.assertTrue(self._test_ws.isCommonBins())

    def test_isCommonLogBins(self):
        self.assertFalse(self._test_ws.isCommonLogBins())
        ws = CreateSampleWorkspace("Event")
        ws = Rebin(ws, "1,-1,10000")
        self.assertTrue(ws.isCommonLogBins())
        ws = Rebin(ws, "1,-0.1,10000")
        self.assertTrue(ws.isCommonLogBins())

    def test_hasMaskedBins(self):
        numBins = 10
        numHist = 11
        ws = WorkspaceCreationHelper.create2DWorkspace123WithMaskedBin(numHist, numBins, 0, 1)
        self.assertTrue(ws.hasMaskedBins(0))
        self.assertFalse(ws.hasMaskedBins(1))

    def test_hasAnyMaskedBins(self):
        numBins = 10
        numHist = 11
        ws = WorkspaceCreationHelper.create2DWorkspace123WithMaskedBin(numHist, numBins, 0, 1)
        self.assertTrue(ws.hasAnyMaskedBins())

    def test_maskedBinsIndices(self):
        numBins = 10
        numHist = 11
        ws = WorkspaceCreationHelper.create2DWorkspace123WithMaskedBin(numHist, numBins, 0, 1)
        maskedBinsIndices = ws.maskedBinsIndices(0)
        self.assertEqual(1, len(maskedBinsIndices))
        for index in maskedBinsIndices:
            self.assertEqual(1, index)

    def test_getIndicesFromDetectorIDs(self):
        detinfo = self._test_ws.detectorInfo()
        detIDs = detinfo.detectorIDs()  # [1, 2]
        # from test_spectrum_retrieval we know
        # self._test_ws.getSpectrum(1).hasDetectorID(2) == True
        index = self._test_ws.getIndicesFromDetectorIDs([int(detIDs[1])])
        self.assertEqual(1, index[0])
        # index == None!

    def test_rebinnedOutput(self):
        rebin = WorkspaceFactory.create("RebinnedOutput", 2, 3, 2)
        self.assertFalse(rebin.nonZeroF())
        fv = rebin.readF(1)
        rebin.dataY(1)[:] = 10.0
        rebin.dataE(1)[:] = 1.0
        twos = np.ones(len(fv)) * 2.0
        rebin.setF(1, twos)
        self.assertTrue(rebin.nonZeroF())
        rebin.setFinalized(False)
        rebin.setSqrdErrors(False)
        rebin.unfinalize()
        self.assertFalse(rebin.isFinalized())
        yv = rebin.readY(1)
        ev = rebin.readE(1)
        self.assertAlmostEqual(yv[0], 10.0)
        self.assertAlmostEqual(ev[0], 1.0)

        rebin.finalize(True)
        self.assertTrue(rebin.isFinalized())
        self.assertTrue(rebin.hasSqrdErrors())
        yv = rebin.readY(1)
        ev = rebin.readE(1)
        self.assertAlmostEqual(yv[0], 5.0)
        self.assertAlmostEqual(ev[0], 0.25)

        rebin.finalize(False)
        self.assertTrue(rebin.isFinalized())
        self.assertFalse(rebin.hasSqrdErrors())
        yv = rebin.readY(1)
        ev = rebin.readE(1)
        self.assertAlmostEqual(yv[0], 5.0)
        self.assertAlmostEqual(ev[0], 0.5)

        rebin.unfinalize()
        self.assertFalse(rebin.isFinalized())
        yv = rebin.readY(1)
        ev = rebin.readE(1)
        self.assertAlmostEqual(yv[0], 10.0)
        self.assertAlmostEqual(ev[0], 1.0)

        rebin.scaleF(2.0)
        fv = rebin.readF(1)
        self.assertAlmostEqual(fv[0], 4.0)

    def test_findY(self):
        # Check that zero is not present
        idx = self._test_ws.findY(0.0)
        self.assertEqual(idx[0], -1)
        self.assertEqual(idx[1], -1)
        # Check that 5. is the first element
        idx = self._test_ws.findY(5.0)
        self.assertEqual(idx[0], 0)
        self.assertEqual(idx[1], 0)
        # Check that no other elements are 5
        idx = self._test_ws.findY(5.0, (0, 1))
        self.assertEqual(idx[0], -1)
        self.assertEqual(idx[1], -1)
        # Check that 2. is the next element
        idx = self._test_ws.findY(2.0)
        self.assertEqual(idx[0], 0)
        self.assertEqual(idx[1], 1)
        # Check that 2. is the next element
        idx = self._test_ws.findY(2.0, (0, 2))
        self.assertEqual(idx[0], 0)
        self.assertEqual(idx[1], 2)


if __name__ == "__main__":
    unittest.main()
    # Testing particular test from Mantid
    # tester=MatrixWorkspaceTest('test_setGetMonitorWS')
    # tester.test_setGetMonitorWS()
