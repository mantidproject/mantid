# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint:
# disable=invalid-name,no-init,bad-builtin,attribute-defined-outside-init,protected-access,too-many-arguments

"""
System tests for HFIR SANS reduction.

The following tests were converted from the unittest framework
that is part of python to the systemtesting framework used in Mantid.
"""

from functools import reduce
import math
from pathlib import PurePath
import traceback
import types

import systemtesting
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.hfir_command_interface import *
from reduction_workflow.command_interface import AppendDataFile, Reduce, Reduce1D


def _diff_iq(x, y):
    return x - y


def _add(x, y):
    return x + y


def _read_IGOR(filepath):
    """
    Read in an HFIR IGOR output file with reduced data
    @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Skip first header line
        f.readline()
        for line in f:
            toks = line.split()
            try:
                q = float(toks[0])
                iq = float(toks[1])
                diq = float(toks[2])
                data.append([q, iq, diq])
            except:
                print("_read_IGOR:", sys.exc_info()[1])
                raise
    return data


def _check_result(ws, test_file, tolerance=1e-6):
    """
    Compare the data in two reduced data files.
    @param reduced_file: path of the Mantid-reduced file
    @param test_file: path of the IGOR-reduced file
    """
    passed = True

    # Read mantid data
    x = ws.dataX(0)[: len(ws.dataX(0))]
    y = ws.dataY(0)
    e = ws.dataE(0)
    data_mantid = list(zip(x, y, e))

    # Read the test data to compare with
    data_igor = _read_IGOR(test_file)

    # Check length
    if not len(data_mantid) == len(data_igor):
        print("Incompatible data lengths")
        return False

    # Utility methods for manipulating the lists
    def _diff_chi2(x, y):
        return (x[1] - y[1]) * (x[1] - y[1]) / (x[2] * x[2])

    def _diff_iq(x, y):
        return x[1] - y[1]

    def _diff_err(x, y):
        return x[2] - y[2]

    def _add(x, y):
        return x + y

    # Check that I(q) is the same for both data sets
    deltas = list(map(_diff_iq, data_mantid, data_igor))
    delta = reduce(_add, deltas) / len(deltas)
    if math.fabs(delta) > tolerance or math.isnan(delta):
        passed = False
        print("Sum of I(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta), tolerance))

    # Then compare the errors
    deltas = list(map(_diff_err, data_mantid, data_igor))
    delta_err = reduce(_add, deltas) / len(deltas)
    if math.fabs(delta_err) > tolerance or math.isnan(delta):
        passed = False
        print("Sum of dI(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta_err), tolerance))

    # Compute chi2 of our result relative to IGOR
    deltas = list(map(_diff_chi2, data_mantid, data_igor))
    chi2 = reduce(_add, deltas) / len(data_igor)
    if chi2 > 10.0 * tolerance or math.isnan(delta):
        passed = False
        print("Chi2 is outside tolerance: %g > %g" % (chi2, 10.0 * tolerance))

    return passed


class HFIRTestsAPIv2(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def assertTrue(self, condition):
        if not condition:
            raise RuntimeError("Condition failed")

    def assertEqual(self, a, b):
        if not a == b:
            raise RuntimeError("%s != %s" % (a, b))

    def _assertAlmostEqual(self, first, second, places=None, msg=None, delta=None, rel_delta=None):
        return self.assertAlmostEqual(first, second, places, msg, delta, rel_delta)

    def assertAlmostEqual(self, first, second, places=None, msg=None, delta=None, rel_delta=None):
        if not assertAlmostEqual(first, second, places, msg, delta, rel_delta):
            if msg is None:
                msg = "Failed condition"
            raise RuntimeError(msg)

    def _cleanup(self):
        ws_list = AnalysisDataService.getObjectNames()
        for ws in ws_list:
            AnalysisDataService.remove(ws)

    def runTest(self):
        class TestStub(object):
            def __init__(self, test_method):
                self._test_method = test_method
                self._passed = True

            def run_test(self):
                # Set up the test
                ReductionSingleton.clean()
                # Execute the test
                try:
                    print(self._test_method.__name__)
                    return self._test_method()
                except:
                    print(traceback.format_exc())
                    raise
                return False

        self.all_passed = True
        self.n_tests = 0
        self.n_passed = 0
        self.failed_tests = []
        for current_item in dir(self):
            m = getattr(self, current_item)
            if current_item.startswith("test_") and isinstance(m, types.MethodType):
                self.n_tests += 1
                t = TestStub(m)
                result = t.run_test()
                self._cleanup()
                if result is None or result:
                    self.n_passed += 1
                else:
                    self.failed_tests.append(current_item)
                    self.all_passed = False

    def test_data_path(self):
        self.assertEqual(ReductionSingleton()._data_path, ".")
        DataPath(self._work_dir)
        self.assertEqual(PurePath(ReductionSingleton()._data_path), PurePath(self._work_dir))

    def test_set_detector_distance(self):
        GPSANS()
        DataPath(self._work_dir)
        AppendDataFile("BioSANS_test_data.xml")
        SetSampleDetectorDistance(2500.0)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        sdd = ws.getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 2500.0)

    def test_set_detector_offset(self):
        GPSANS()
        SetSampleDetectorDistance(6500)
        DataPath(self._work_dir)
        AppendDataFile("BioSANS_test_data.xml")
        # SetSampleDetectorOffset(500.0)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        sdd = ws.getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 6500.0)

    def test_set_distance_and_detector_offset(self):
        """
        If both detector distance and offset are set, use only the distance
        """
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        AppendDataFile("BioSANS_test_data.xml")
        SetSampleDetectorDistance(2500.0)
        SetSampleDetectorOffset(500.0)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        sdd = ws.getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 2500.0)

    def test_set_wavelength(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        AppendDataFile("BioSANS_test_data.xml")
        SetWavelength(5.0, 1.2)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        v_x = ws.dataX(0)
        self.assertEqual(v_x[0], 4.4)
        self.assertEqual(v_x[1], 5.6)

    def test_direct_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        center_x = ws.getRun().getProperty("beam_center_x").value
        center_y = ws.getRun().getProperty("beam_center_y").value
        self.assertAlmostEqual(center_x, 16.6038, delta=0.0001)
        self.assertAlmostEqual(center_y, 96.771, delta=0.0001)

        propmng_name = ReductionSingleton().get_reduction_table_name()
        p = PropertyManagerDataService.retrieve(propmng_name)
        center_x = p.getProperty("LatestBeamCenterX").value
        center_y = p.getProperty("LatestBeamCenterY").value
        self.assertAlmostEqual(center_x, 16.6038, delta=0.0001)
        self.assertAlmostEqual(center_y, 96.771, delta=0.0001)

    def test_hand_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        SetBeamCenter(1.1, 2.2)
        Reduce()

        propmng_name = ReductionSingleton().get_reduction_table_name()
        p = PropertyManagerDataService.retrieve(propmng_name)

        center_x = p.getProperty("LatestBeamCenterX").value
        center_y = p.getProperty("LatestBeamCenterY").value

        self.assertAlmostEqual(center_x, 1.1, delta=0.0001)
        self.assertAlmostEqual(center_y, 2.2, delta=0.0001)

    def test_load_run(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        self.assertEqual(len(ReductionSingleton()._data_files), 0)
        AppendDataFile("BioSANS_test_data.xml")
        self.assertEqual(len(ReductionSingleton()._data_files), 1)

    def test_to_steps(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        sdd = ws.getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 6000.0)

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        self.assertTrue(_check_result(ws, FileFinder.getFullPath("reduced_center_calculated.txt"), tolerance=1e-4))

    def test_reduction_1(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        check = [
            0.19472,
            0.204269,
            0.215354,
            0.230114,
            0.238961,
            0.237201,
            0.247843,
            0.248424,
            0.253676,
            0.254327,
            0.254366,
            0.252931,
            0.258339,
            0.259297,
            0.257155,
            0.254059,
            0.252383,
            0.252826,
            0.256604,
            0.256754,
            0.255592,
            0.256813,
            0.248569,
            0.25331,
            0.251032,
            0.246424,
            0.249477,
            0.250939,
            0.251959,
            0.24925,
            0.250372,
            0.246148,
            0.250478,
            0.244621,
            0.247428,
            0.246431,
            0.245041,
            0.241647,
            0.24307,
            0.240096,
            0.242797,
            0.238182,
            0.237548,
            0.239789,
            0.241477,
            0.23456,
            0.237372,
            0.233715,
            0.233789,
            0.232262,
            0.231589,
            0.230986,
            0.231646,
            0.231331,
            0.230484,
            0.2277,
            0.226819,
            0.224341,
            0.227239,
            0.223228,
            0.221232,
            0.222011,
            0.224747,
            0.219533,
            0.216973,
            0.218734,
            0.21668,
            0.218366,
            0.214926,
            0.213985,
            0.214469,
            0.210473,
            0.209867,
            0.209066,
            0.208965,
            0.207498,
            0.204505,
            0.205786,
            0.202186,
            0.200442,
            0.200485,
            0.200554,
            0.200499,
            0.198152,
            0.193945,
            0.192082,
            0.193783,
            0.193787,
            0.190557,
            0.190471,
            0.186827,
            0.190088,
            0.188204,
            0.187547,
            0.182206,
            0.181384,
            0.180358,
            0.182663,
            0.178844,
            0.176556,
        ]

        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.00001)

    def test_reduction_iqxqy(self):
        """
        Test reduction with IQxQy in log binning
        """
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        IQxQy(nbins=100, log_binning=True)
        Reduce()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iqxy")
        data = ws.dataX(0)
        check = [
            -0.11,
            -0.10489929,
            -0.1000351,
            -0.09539647,
            -0.09097293,
            -0.08675451,
            -0.08273169,
            -0.07889542,
            -0.07523703,
            -0.07174829,
            -0.06842131,
            -0.06524861,
            -0.06222303,
            -0.05933774,
            -0.05658625,
            -0.05396234,
            -0.0514601,
            -0.04907389,
            -0.04679833,
            -0.04462829,
            -0.04255887,
            -0.04058542,
            -0.03870347,
            -0.03690878,
            -0.03519732,
            -0.03356522,
            -0.0320088,
            -0.03052455,
            -0.02910912,
            -0.02775933,
            -0.02647213,
            -0.02524461,
            -0.02407402,
            -0.0229577,
            -0.02189315,
            -0.02087797,
            -0.01990985,
            -0.01898663,
            -0.01810622,
            -0.01726663,
            -0.01646598,
            -0.01570245,
            -0.01497433,
            -0.01427997,
            -0.0136178,
            -0.01298634,
            -0.01238417,
            -0.01180991,
            -0.01126229,
            -0.01074005,
            -0.01024204,
            -0.00976711,
            -0.00931421,
            -0.00888231,
            -0.00847044,
            -0.00807766,
            -0.0077031,
            -0.00734591,
            -0.00700528,
            -0.00668044,
            -0.00637067,
            -0.00607526,
            -0.00579355,
            -0.0055249,
            -0.00526871,
            -0.0050244,
            -0.00479142,
            -0.00456924,
            -0.00435737,
            -0.00415531,
            -0.00396263,
            -0.00377888,
            -0.00360366,
            -0.00343656,
            -0.0032772,
            -0.00312524,
            -0.00298032,
            -0.00284212,
            -0.00271033,
            -0.00258465,
            -0.0024648,
            -0.00235051,
            -0.00224152,
            -0.00213758,
            -0.00203846,
            -0.00194394,
            -0.00185379,
            -0.00176783,
            -0.00168586,
            -0.00160769,
            -0.00153314,
            -0.00146205,
            -0.00139425,
            -0.0013296,
            -0.00126795,
            -0.00120915,
            -0.00115308,
            -0.00109961,
            -0.00104862,
            -0.001,
            0.0,
            0.001,
            0.00104862,
            0.00109961,
            0.00115308,
            0.00120915,
            0.00126795,
            0.0013296,
            0.00139425,
            0.00146205,
            0.00153314,
            0.00160769,
            0.00168586,
            0.00176783,
            0.00185379,
            0.00194394,
            0.00203846,
            0.00213758,
            0.00224152,
            0.00235051,
            0.0024648,
            0.00258465,
            0.00271033,
            0.00284212,
            0.00298032,
            0.00312524,
            0.0032772,
            0.00343656,
            0.00360366,
            0.00377888,
            0.00396263,
            0.00415531,
            0.00435737,
            0.00456924,
            0.00479142,
            0.0050244,
            0.00526871,
            0.0055249,
            0.00579355,
            0.00607526,
            0.00637067,
            0.00668044,
            0.00700528,
            0.00734591,
            0.0077031,
            0.00807766,
            0.00847044,
            0.00888231,
            0.00931421,
            0.00976711,
            0.01024204,
            0.01074005,
            0.01126229,
            0.01180991,
            0.01238417,
            0.01298634,
            0.0136178,
            0.01427997,
            0.01497433,
            0.01570245,
            0.01646598,
            0.01726663,
            0.01810622,
            0.01898663,
            0.01990985,
            0.02087797,
            0.02189315,
            0.0229577,
            0.02407402,
            0.02524461,
            0.02647213,
            0.02775933,
            0.02910912,
            0.03052455,
            0.0320088,
            0.03356522,
            0.03519732,
            0.03690878,
            0.03870347,
            0.04058542,
            0.04255887,
            0.04462829,
            0.04679833,
            0.04907389,
            0.0514601,
            0.05396234,
            0.05658625,
            0.05933774,
            0.06222303,
            0.06524861,
            0.06842131,
            0.07174829,
            0.07523703,
            0.07889542,
            0.08273169,
            0.08675451,
            0.09097293,
            0.09539647,
            0.1000351,
            0.10489929,
            0.11,
        ]
        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.00001)
        print(data)

    def test_no_solid_angle(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        NoSolidAngle()
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], 0.1948464330517794, delta=0.00001)
        self.assertAlmostEqual(data[10], 0.25088976280978281, delta=0.00001)
        self.assertAlmostEqual(data[20], 0.252098592791137, delta=0.00001)

    def test_reduction_2(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        check = [
            0.268942,
            0.272052,
            0.269806,
            0.27129,
            0.273852,
            0.271301,
            0.271732,
            0.271103,
            0.270996,
            0.269677,
            0.27098,
            0.266802,
            0.26789,
            0.268222,
            0.266125,
            0.262736,
            0.262752,
            0.263827,
            0.26315,
            0.262775,
            0.261541,
            0.260818,
            0.258955,
            0.257675,
            0.255908,
            0.254088,
            0.256778,
            0.256883,
            0.253568,
            0.25636,
            0.252323,
            0.251833,
            0.251914,
            0.252298,
            0.249375,
            0.247718,
            0.247768,
            0.244636,
            0.245604,
            0.243996,
            0.244332,
            0.244363,
            0.242985,
            0.242234,
            0.241118,
            0.241411,
            0.24084,
            0.239293,
            0.2392,
            0.236565,
            0.234557,
            0.233974,
            0.232905,
            0.231898,
            0.231085,
            0.229586,
            0.22862,
            0.227001,
            0.226783,
            0.225837,
            0.224835,
            0.223807,
            0.222296,
            0.221557,
            0.220464,
            0.219139,
            0.217611,
            0.217049,
            0.21606,
            0.215739,
            0.216233,
            0.213467,
            0.213141,
            0.213275,
            0.219695,
            0.216121,
            0.215502,
            0.21792,
            0.209364,
            0.209368,
            0.2064,
            0.205844,
            0.20431,
            0.203443,
            0.202442,
            0.200195,
            0.199408,
            0.19853,
            0.195654,
            0.195514,
            0.193086,
            0.193388,
            0.19137,
            0.190122,
            0.189119,
            0.18864,
            0.185473,
            0.184958,
            0.183981,
            0.182581,
        ]

        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.00001)

    def test_straight_Q1D(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        check = [
            0.269037,
            0.272176,
            0.269917,
            0.271416,
            0.273988,
            0.271432,
            0.271857,
            0.271232,
            0.271118,
            0.269797,
            0.271095,
            0.266912,
            0.268015,
            0.268356,
            0.266256,
            0.26287,
            0.262888,
            0.263964,
            0.263281,
            0.262905,
            0.261669,
            0.26094,
            0.259081,
            0.257802,
            0.256029,
            0.254228,
            0.256913,
            0.257021,
            0.253692,
            0.256491,
            0.252454,
            0.251969,
            0.25204,
            0.252423,
            0.249516,
            0.247844,
            0.247895,
            0.24476,
            0.245734,
            0.244125,
            0.244474,
            0.244491,
            0.243126,
            0.242359,
            0.241239,
            0.24154,
            0.240976,
            0.239421,
            0.23933,
            0.236688,
            0.234685,
            0.234105,
            0.233034,
            0.232036,
            0.231208,
            0.229714,
            0.228749,
            0.227122,
            0.226918,
            0.225969,
            0.22497,
            0.223933,
            0.222426,
            0.221684,
            0.2206,
            0.219277,
            0.217739,
            0.217173,
            0.216193,
            0.215869,
            0.216354,
            0.213597,
            0.213271,
            0.213407,
            0.219829,
            0.216259,
            0.215635,
            0.218058,
            0.209499,
            0.209503,
            0.206529,
            0.205981,
            0.20445,
            0.203577,
            0.202577,
            0.200334,
            0.199544,
            0.198663,
            0.195786,
            0.195653,
            0.19322,
            0.193537,
            0.191503,
            0.190253,
            0.189253,
            0.188771,
            0.1856,
            0.185099,
            0.184111,
            0.182717,
        ]

        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.00001)

    def test_transmission(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        check = [
            0.514758,
            0.520759,
            0.516451,
            0.51932,
            0.524206,
            0.519275,
            0.520125,
            0.518997,
            0.518729,
            0.516198,
            0.518718,
            0.51072,
            0.512816,
            0.513449,
            0.509453,
            0.502968,
            0.503003,
            0.505098,
            0.503835,
            0.503088,
            0.500716,
            0.499304,
            0.495777,
            0.49332,
            0.489926,
            0.486497,
            0.491656,
            0.491858,
            0.48546,
            0.490808,
            0.483111,
            0.482176,
            0.482359,
            0.483098,
            0.477528,
            0.474279,
            0.474485,
            0.468472,
            0.470305,
            0.467228,
            0.467934,
            0.467971,
            0.465358,
            0.463885,
            0.461762,
            0.462352,
            0.461285,
            0.458322,
            0.458118,
            0.453064,
            0.44927,
            0.448151,
            0.446129,
            0.444207,
            0.442629,
            0.439792,
            0.437958,
            0.434826,
            0.434443,
            0.432655,
            0.430731,
            0.428771,
            0.425893,
            0.424477,
            0.422421,
            0.419886,
            0.416942,
            0.415876,
            0.414037,
            0.41339,
            0.414353,
            0.409062,
            0.408431,
            0.408712,
            0.419282,
            0.412833,
            0.41062,
            0.414427,
            0.400056,
            0.400141,
            0.394724,
            0.393821,
            0.390721,
            0.38932,
            0.387497,
            0.383062,
            0.381603,
            0.380016,
            0.374635,
            0.374214,
            0.369733,
            0.370353,
            0.366464,
            0.364109,
            0.362184,
            0.361299,
            0.355246,
            0.354339,
            0.352412,
            0.349748,
        ]

        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.001)

    def test_spreader_transmission(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        BeamSpreaderTransmission(
            sample_spreader="BioSANS_test_data.xml",
            direct_spreader="BioSANS_empty_cell.xml",
            sample_scattering="BioSANS_test_data.xml",
            direct_scattering="BioSANS_empty_cell.xml",
            spreader_transmission=0.5,
            spreader_transmission_err=0.1,
        )

        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        self.assertAlmostEqual(data[0], 0.00418831, delta=0.00001)
        self.assertAlmostEqual(data[10], 0.0042193, delta=0.00001)

    def test_transmission_by_hand(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        # property_manager = PropertyManagerDataService.retrieve(ReductionSingleton().get_reduction_table_name())
        # _p = property_manager.getProperty("TransmissionAlgorithm")

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        self.assertTrue(_check_result(ws, FileFinder.getFullPath("reduced_transmission.txt"), 0.0001))

    def test_center_by_hand(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        self.assertTrue(_check_result(ws, FileFinder.getFullPath("reduced_center_by_hand.txt"), 0.0001))

    def test_background(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], 0.0, 10)
        self.assertAlmostEqual(data[10], 0.0, 10)
        self.assertAlmostEqual(data[20], 0.0, 10)

    def test_background_multiple_files(self):
        """
        Subtracting background using multiple files should properly take
        into account the normalization.
        """
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml,BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], 0.0, 10)
        self.assertAlmostEqual(data[10], 0.0, 10)
        self.assertAlmostEqual(data[20], 0.0, 10)

    def test_bck_w_transmission(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        SetTransmission(0.6, 0.1)
        SetBckTransmission(0.6, 0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], 0.0, 10)
        self.assertAlmostEqual(data[10], 0.0, 10)
        self.assertAlmostEqual(data[20], 0.0, 10)

    def test_transmission_by_hand_w_sensitivity(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_test_data_Iq")
        data = ws.dataY(0)
        check = [
            0.374914,
            0.393394,
            0.414756,
            0.443152,
            0.460175,
            0.456802,
            0.477264,
            0.478456,
            0.488523,
            0.489758,
            0.489871,
            0.487127,
            0.497585,
            0.499346,
            0.49526,
            0.489273,
            0.486082,
            0.486923,
            0.494208,
            0.494531,
            0.492264,
            0.494608,
            0.478766,
            0.487872,
            0.48357,
            0.474654,
            0.48052,
            0.483367,
            0.485269,
            0.480079,
            0.482254,
            0.47413,
            0.48245,
            0.471207,
            0.476589,
            0.474701,
            0.472014,
            0.465479,
            0.468236,
            0.462524,
            0.46773,
            0.458851,
            0.457653,
            0.461929,
            0.465216,
            0.451887,
            0.45733,
            0.450281,
            0.45045,
            0.447508,
            0.446209,
            0.445063,
            0.446328,
            0.445735,
            0.444096,
            0.438758,
            0.43707,
            0.432302,
            0.437903,
            0.430176,
            0.426317,
            0.427858,
            0.433131,
            0.423087,
            0.418146,
            0.421584,
            0.417606,
            0.420891,
            0.414255,
            0.412448,
            0.413393,
            0.405706,
            0.404541,
            0.403016,
            0.402806,
            0.400023,
            0.394248,
            0.396725,
            0.389808,
            0.386475,
            0.386525,
            0.386674,
            0.386575,
            0.382081,
            0.373986,
            0.370391,
            0.37367,
            0.373686,
            0.367479,
            0.36732,
            0.36031,
            0.366588,
            0.362994,
            0.361712,
            0.351433,
            0.349867,
            0.3479,
            0.352355,
            0.344987,
            0.340605,
        ]

        # Check that I(q) is the same for both data sets
        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.00001)

    def test_SampleGeometry_functions(self):
        print("SKIPPING test_SampleGeometry_functions()")
        return
        # pylint: disable=unreachable
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        AppendDataFile("BioSANS_test_data.xml")
        SampleGeometry("cuboid")
        SampleThickness(2.0)
        SampleHeight(3.0)
        SampleWidth(5.0)

        # we don't need to do a full reduction for this test, do a partial
        # reduction
        ReductionSingleton().pre_process()
        ReductionSingleton()._reduction_steps[0].execute(ReductionSingleton(), "BioSANS_test_data")
        ReductionSingleton().geometry_correcter.execute(ReductionSingleton(), "BioSANS_test_data")

        ws = AnalysisDataService.retrieve("BioSANS_test_data")
        data = [ws.dataY(0)[0], ws.dataY(1)[0], ws.dataY(2)[0], ws.dataY(3)[0], ws.dataY(4)[0], ws.dataY(5)[0]]

        check = [500091.0, 60.0, 40.8333, 13.6333, 13.4667, 13.6667]
        # Check that I(q) is the same for both data sets
        deltas = list(map(_diff_iq, data, check))
        delta = reduce(_add, deltas) / len(deltas)
        self.assertLess(math.fabs(delta), 0.1)

    def test_noDC_eff_with_DC(self):
        ref = [
            28.06525,
            136.94662,
            -16.20412,
            0.00000,
            147.79915,
            146.42713,
            302.00869,
            0.00000,
            0.00000,
            -1869.20724,
            -2190.89681,
            -1892.14939,
            -2140.79608,
            -1980.60037,
            -2096.75974,
            -2221.30118,
            -2263.51541,
            -2264.89989,
            -2364.83528,
            -2420.58152,
            -2444.51906,
            -2418.28886,
            -2606.16991,
            -2556.93660,
            -2623.71380,
            -2547.79671,
            -2670.60962,
            -2714.35237,
            -2717.01692,
            -2730.84974,
            -2768.92925,
            -2753.96396,
            -2732.66316,
            -2795.89687,
            -2780.37320,
            -2755.38910,
            -2814.88120,
            -2830.74081,
            -2803.42030,
            -2815.33244,
            -2754.70444,
            -2718.55136,
            -2740.03811,
            -2754.60415,
            -2815.96387,
            -2754.62039,
            -2781.54596,
            -2765.26282,
            -2676.04665,
            -2762.33751,
            -2722.94832,
            -2707.74990,
            -2730.50371,
            -2721.71272,
            -2682.02439,
            -2703.36446,
            -2679.47677,
            -2658.57573,
            -2669.41871,
            -2618.90655,
            -2638.41601,
            -2614.69128,
            -2583.29713,
            -2589.39730,
            -2567.19209,
            -2535.09328,
            -2539.43296,
            -2489.60117,
            -2500.76844,
            -2456.22248,
            -2444.13734,
            -2392.68589,
            -2410.98591,
            -2348.68064,
            -2334.84651,
            -2310.41426,
            -2250.24085,
            -2220.02192,
            -2184.65990,
            -2154.19638,
            -2099.56797,
            -2058.51585,
            -2004.05601,
            -1966.52356,
            -1910.47283,
            -1876.72098,
            -1817.69045,
            -1768.62167,
            -1721.56444,
            -1666.47199,
            -1608.86707,
            -1544.26178,
            -1492.78389,
            -1438.69256,
            -1358.60437,
            -1299.34476,
            -1221.57010,
            -1080.69421,
            -609.77891,
            -77.72765,
        ]
        BIOSANS()
        # SetSampleDetectorOffset(837.9)
        SetSampleDetectorDistance(6837.9)
        # SolidAngle() # name clash with SolidAngle algorithm
        MonitorNormalization()
        AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=True)
        # IQxQy(nbins=100)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        SensitivityCorrection(
            "BioSANS_flood_data.xml", min_sensitivity=0.5, max_sensitivity=1.5, dark_current="BioSANS_empty_trans.xml", use_sample_dc=False
        )
        DivideByThickness(1)
        SetTransmission(1, 0)
        ThetaDependentTransmission(True)
        DataPath(self._work_dir)
        AppendDataFile(["BioSANS_exp61_scan0004_0001.xml"])
        Background("BioSANS_test_data.xml")
        SetBckTransmission(1, 0)
        BckThetaDependentTransmission(True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_exp61_scan0004_0001_Iq")
        res = ws.dataY(0)
        for i in range(len(res)):
            self._assertAlmostEqual(res[i], ref[i], delta=0.01, rel_delta=0.001, msg="result point %d: %g, found %g" % (i, ref[i], res[i]))

    def test_DC_eff_with_DC(self):
        ref = [
            28.0476,
            136.906,
            -16.3079,
            0,
            147.757,
            146.403,
            301.982,
            0,
            0,
            -1869.21,
            -2190.93,
            -1892.16,
            -2140.81,
            -1980.62,
            -2096.79,
            -2221.34,
            -2263.55,
            -2264.93,
            -2364.87,
            -2420.61,
            -2444.56,
            -2418.32,
            -2606.21,
            -2556.98,
            -2623.75,
            -2547.84,
            -2670.66,
            -2714.39,
            -2717.06,
            -2730.89,
            -2768.96,
            -2754.01,
            -2732.7,
            -2795.93,
            -2780.41,
            -2755.42,
            -2814.92,
            -2830.79,
            -2803.46,
            -2815.38,
            -2754.75,
            -2718.6,
            -2740.08,
            -2754.65,
            -2816.01,
            -2754.66,
            -2781.59,
            -2765.3,
            -2676.09,
            -2762.38,
            -2722.99,
            -2707.8,
            -2730.55,
            -2721.76,
            -2682.07,
            -2703.41,
            -2679.52,
            -2658.62,
            -2669.46,
            -2618.95,
            -2638.46,
            -2614.74,
            -2583.34,
            -2589.44,
            -2567.23,
            -2535.14,
            -2539.48,
            -2489.64,
            -2500.81,
            -2456.26,
            -2444.18,
            -2392.73,
            -2411.03,
            -2348.73,
            -2334.89,
            -2310.46,
            -2250.28,
            -2220.07,
            -2184.7,
            -2154.24,
            -2099.61,
            -2058.56,
            -2004.1,
            -1966.57,
            -1910.52,
            -1876.76,
            -1817.73,
            -1768.67,
            -1721.61,
            -1666.51,
            -1608.91,
            -1544.31,
            -1492.83,
            -1438.74,
            -1358.65,
            -1299.39,
            -1221.61,
            -1080.73,
            -609.821,
            -77.7712,
        ]
        BIOSANS()
        # SetSampleDetectorOffset(837.9)
        SetSampleDetectorDistance(6837.9)
        # SolidAngle()
        DarkCurrent("BioSANS_dark_current.xml")
        MonitorNormalization()
        AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=True)
        # IQxQy(nbins=100)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        SensitivityCorrection(
            "BioSANS_flood_data.xml", min_sensitivity=0.5, max_sensitivity=1.5, dark_current="BioSANS_empty_trans.xml", use_sample_dc=False
        )
        DivideByThickness(1)
        SetTransmission(1, 0)
        ThetaDependentTransmission(True)
        DataPath(self._work_dir)
        AppendDataFile(["BioSANS_exp61_scan0004_0001.xml"])
        Background("BioSANS_test_data.xml")
        SetBckTransmission(1, 0)
        BckThetaDependentTransmission(True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_exp61_scan0004_0001_Iq")
        res = ws.dataY(0)
        for i in range(len(res)):
            self._assertAlmostEqual(res[i], ref[i], delta=0.01, rel_delta=0.001, msg="result point %d: %g, found %g" % (i, ref[i], res[i]))

    def test_DC_eff_noDC(self):
        ref = [
            10.4139,
            124.814,
            25.0443,
            0,
            38.3413,
            133.417,
            0,
            0,
            -1733.56,
            -1627.57,
            -1811.38,
            -1851.58,
            -1888.38,
            -1957.07,
            -2056.47,
            -2117.52,
            -2139.32,
            -2176.94,
            -2239.91,
            -2350.65,
            -2417.75,
            -2406.99,
            -2525.48,
            -2551.45,
            -2566.83,
            -2499.38,
            -2632.35,
            -2662.17,
            -2653.14,
            -2718.65,
            -2740.78,
            -2758.94,
            -2712,
            -2771.35,
            -2761.38,
            -2724.05,
            -2809.97,
            -2815.92,
            -2801.25,
            -2824.54,
            -2726.76,
            -2716.63,
            -2737.83,
            -2752.06,
            -2798.95,
            -2757.7,
            -2787.58,
            -2753.12,
            -2691.47,
            -2759.93,
            -2703.94,
            -2705.55,
            -2722.64,
            -2714.75,
            -2685.28,
            -2693.49,
            -2685.75,
            -2655.65,
            -2662.42,
            -2614.47,
            -2633.12,
            -2602.29,
            -2579.4,
            -2591.17,
            -2565.28,
            -2529.61,
            -2533.85,
            -2491.87,
            -2496.78,
            -2458.25,
            -2437.25,
            -2398.16,
            -2407.29,
            -2350.32,
            -2340.43,
            -2301.5,
            -2254.37,
            -2224.97,
            -2186.64,
            -2146.73,
            -2096.71,
            -2058.12,
            -2006.2,
            -1968.6,
            -1914.93,
            -1874.31,
            -1819.05,
            -1767.14,
            -1722.35,
            -1670.38,
            -1606.61,
            -1544.51,
            -1496.24,
            -1438.21,
            -1360.12,
            -1299.68,
            -1221.61,
            -1080.91,
            -610.638,
            -71.9557,
        ]
        BIOSANS()
        # SetSampleDetectorOffset(837.9)
        SetSampleDetectorDistance(6837.9)
        # SolidAngle()
        DarkCurrent("BioSANS_dark_current.xml")
        MonitorNormalization()
        AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=True)
        # IQxQy(nbins=100)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", min_sensitivity=0.5, max_sensitivity=1.5, use_sample_dc=False)
        DivideByThickness(1)
        SetTransmission(1, 0)
        ThetaDependentTransmission(True)
        DataPath(self._work_dir)
        AppendDataFile(["BioSANS_exp61_scan0004_0001.xml"])
        Background("BioSANS_test_data.xml")
        SetBckTransmission(1, 0)
        BckThetaDependentTransmission(True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("BioSANS_exp61_scan0004_0001_Iq")
        res = ws.dataY(0)
        for i in range(len(res)):
            self._assertAlmostEqual(res[i], ref[i], delta=0.01, rel_delta=0.001, msg="result point %d: %g, found %g" % (i, ref[i], res[i]))

    def test_transmission_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        SetTransmissionBeamCenter(100, 15)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], 0.195821, delta=0.00001)
        self.assertAlmostEqual(data[10], 0.256210, delta=0.00001)
        self.assertAlmostEqual(data[20], 0.257666, delta=0.00001)

    def test_bck_transmission_default_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        SetTransmission(0.6, 0.1)
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        ws = AnalysisDataService.retrieve("test_data_Iq")
        data = ws.dataY(0)
        self.assertAlmostEqual(data[0], -0.0682723, delta=0.00001)
        self.assertAlmostEqual(data[10], -0.068800, delta=0.00001)
        self.assertAlmostEqual(data[20], -0.066403, delta=0.00001)

    def test_bck_transmission_set_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        SetTransmission(0.6, 0.1)
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        SetBckTransmissionBeamCenter(100, 15)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        data = mtd["test_data_Iq"].dataY(0)
        self.assertAlmostEqual(data[0], 0.1787709, delta=0.00001)
        self.assertAlmostEqual(data[10], 0.1801518, delta=0.00001)
        self.assertAlmostEqual(data[20], 0.1738586, delta=0.00001)

    def test_bck_transmission_direct_beam_center(self):
        GPSANS()
        SetSampleDetectorDistance(6000)
        DataPath(self._work_dir)
        # DirectBeamCenter("BioSANS_empty_cell.xml")
        SetBeamCenter(100, 15)
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        SetTransmission(0.6, 0.1)
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        BckTransmissionDirectBeamCenter("BioSANS_empty_cell.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

        data = mtd["test_data_Iq"].dataY(0)
        self.assertAlmostEqual(data[0], -0.046791, delta=0.00001)
        self.assertAlmostEqual(data[10], -0.047874, delta=0.00001)
        self.assertAlmostEqual(data[20], -0.047785, delta=0.00001)

    def validate(self):
        print("HFIRTests: %d / %d tests passed" % (self.n_passed, self.n_tests))
        for items in self.failed_tests:
            print(items)
        return self.all_passed


def assertAlmostEqual(first, second, places=None, _msg=None, delta=None, rel_delta=None):
    """
    Simple test to compare two numbers
    @return: True of the two numbers agree within tolerance
    """
    if first == second:
        # shortcut
        return True

    if delta is not None and places is not None:
        raise TypeError("specify delta or places not both")

    if delta is not None:
        if abs(first - second) <= delta:
            return True
        elif abs(first - second) / abs(second) < rel_delta:
            print("\n-----> %s != %s but within %s percent" % (str(first), str(second), str(rel_delta * 100.0)))
            return True

        standardMsg = "%s != %s within %s delta" % (str(first), str(second), str(delta))
    else:
        if places is None:
            places = 7

        if round(abs(second - first), places) == 0:
            return True

        standardMsg = "%s != %s within %r places" % (str(first), str(second), places)
    print(standardMsg)
    return False
