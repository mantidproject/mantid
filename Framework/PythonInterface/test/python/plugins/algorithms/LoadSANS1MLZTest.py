import unittest
import os
import numpy as np
from mantid.api import AnalysisDataService
from testhelpers import run_algorithm
from mantid.simpleapi import LoadSANS1MLZ
from mantid import config
from plugins.algorithms.SANS1DataMLZ import SANSdata


class LoadSANSMLZTest(unittest.TestCase):
    filename_001: str = "D0122881.001"
    filename_incomplete: str = "sans-incomplete.001"

    def test_LoadValidData001(self):
        """
        test: whether the workspace has been created, is the instrument name correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test1"
        alg_test = run_algorithm("LoadSANS1MLZ", Filename=self.filename_001, OutputWorkspace=output_ws_name)

        self.assertTrue(alg_test.isExecuted())
        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual("SANS-1_MLZ", ws.getInstrument().getName())
        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_VerifyValues001(self):
        """
        test: whether the values are correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test2"
        run_algorithm("LoadSANS1MLZ", Filename=self.filename_001, OutputWorkspace=output_ws_name)
        ws = AnalysisDataService.retrieve(output_ws_name)
        # dimensions
        self.assertEqual(16386, ws.getNumberHistograms())
        self.assertEqual(2, ws.getNumDims())
        # data array
        self.assertEqual(1109, ws.readY(8502))
        self.assertEqual(1160, ws.readY(8629))
        # sample logs
        run = ws.getRun()

        self.assertEqual(4.9, run.getProperty("wavelength").value)

        self.assertEqual("D0122881/1", ws.getTitle())
        self.assertEqual(output_ws_name, ws.name())

        self.assertEqual(22, run.getProperty("position").value)
        self.assertEqual(0.0, run.getProperty("thickness").value)
        self.assertEqual(1.4904, run.getProperty("l2").value)
        self.assertEqual(3600.688081, run.getProperty("duration").value)
        self.assertEqual(6392861, run.getProperty("monitor1").value)
        self.assertEqual(14902342, run.getProperty("monitor2").value)
        self.assertEqual(0.0, run.getProperty("scaling").value)
        self.assertEqual(0.0, run.getProperty("transmission").value)

        det = ws.getDetector(0)
        self.assertAlmostEqual(25.9118, -ws.detectorSignedTwoTheta(det) * 180 / np.pi, 4)

        instrument = ws.getInstrument()
        self.assertEqual(8.0, instrument.getNumberParameter("x-pixel-size")[0])
        self.assertEqual(8.0, instrument.getNumberParameter("y-pixel-size")[0])
        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_LoadInvalidData001(self):
        """
        test: trying to process incorrect data file;
        check is exception definition is correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test3"

        self._create_incomplete_dataFile(self.filename_incomplete, "section amount")
        self.assertRaisesRegex(
            RuntimeError, "Failed to find 'File' section", LoadSANS1MLZ, Filename=self.filename_incomplete, OutputWorkspace=output_ws_name
        )

    def test_LoadValidData_noMonitors001(self):
        """
        test: create workspace with no monitors
        """
        output_ws_name = "LoadSANS1MLZTest_Test4"
        current_paths = config.getDataSearchDirs()[0]
        filename_path = current_paths + self.filename_001
        with open(self.filename_incomplete, "w") as f:
            with open(filename_path, "r") as fs:
                t = fs.readlines()
                f.writelines(t[:150])
                f.writelines(t[153:])
        alg_test = run_algorithm("LoadSANS1MLZ", Filename=self.filename_incomplete, OutputWorkspace=output_ws_name, Wavelength=3.2)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual("SANS-1_MLZ", ws.getInstrument().getName())
        # dimensions
        self.assertEqual(16384, ws.getNumberHistograms())
        # data array
        self.assertEqual(1109, ws.readY(8502))
        self.assertEqual(1160, ws.readY(8629))

        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_LoadValidData_sectionIndependence(self):
        """
        test: process incomplete data file;
        """
        output_ws_name = "LoadSANS1MLZTest_Test7"

        self._create_incomplete_dataFile(self.filename_incomplete, "independence ")
        alg_test = run_algorithm("LoadSANS1MLZ", Filename=self.filename_incomplete, Wavelength=4.6, OutputWorkspace=output_ws_name)

        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual("SANS-1_MLZ", ws.getInstrument().getName())

        self.assertTrue(alg_test.isExecuted())

    @staticmethod
    def _create_incomplete_dataFile(filename, param):
        """
        creates an incomplete data file
        """
        prm = param.split(" ")
        if prm[0] == "section":
            with open(filename, "w") as f:
                f.write("\n\n\n\n")
                if prm[1] == "name":
                    f.write("%Fle\n\n")
                elif prm[1] == "amount":
                    pass
                f.write("%Sample\n\n")
                f.write("%Setup\n\n")
                f.write("%Counter\n\n")
                f.write("%History\n\n")
                f.write("%Comment\n\n")
                f.write("%Counts\n\n")
                s = ("1, " * 127 + "1\n") * 128
                f.write(s)
        elif prm[0] == "independence":
            with open(filename, "w") as f:
                f.write("\n\n\n\n")
                f.write("%Setup\n\n")
                f.write("%File\n\n")
                f.write("DataSizeY=128\n")
                f.write("DataSizeX=128\n")
                f.write("FromDate=01/23/2018\n")
                f.write("FromTime=03:33:20 PM\n")
                f.write("ToDate=01/23/2018\n")
                f.write("ToTime=03:33:51 PM\n")
                f.write("FileName=data.001\n")
                f.write("%Comment 1\n\n")
                f.write("%History\n\n")
                f.write("%Counts\n\n")
                s = ("1, " * 127 + "1\n") * 128
                f.write(s)
                f.write("%Sample\n\n")
                f.write("%Counter\n\n")

    @staticmethod
    def remove_incomplete_dataFile(filename):
        os.remove(filename)

    @classmethod
    def tearDownClass(cls):
        cls.remove_incomplete_dataFile(cls.filename_incomplete)


class SANS1DataClassTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        filename = "D0122881.001"
        metadata = SANSdata()
        metadata.analyze_source(cls.get_file_absolute_path(filename), comment=True)
        cls.metadata = metadata

    def test_StartTime(self):
        date = np.datetime64("2015-01-13T11:10:28")
        self.assertEqual(date, self.metadata.file.run_start())
        date = np.datetime64("2010-04-27T07:30:25")
        self.metadata.file.info["FromDate"] = "04/27/2010"
        self.metadata.file.info["FromTime"] = "07:30:25 AM"
        self.assertEqual(date, self.metadata.file.run_start())

    def test_EndTime(self):
        date = np.datetime64("2015-01-13T12:10:29")
        self.assertEqual(date, self.metadata.file.run_end())
        date = np.datetime64("2010-04-27T07:30:25")
        self.metadata.file.info["ToDate"] = "04/27/2010"
        self.metadata.file.info["ToTime"] = "07:30:25 AM"
        self.assertEqual(date, self.metadata.file.run_end())

    def test_GetTitleName(self):
        title = "D0122881/1"
        self.assertEqual(title, self.metadata.file.get_title())

    def test_CheckFileSectionValues(self):
        self.assertEqual("SANSDRaw", self.metadata.file.info["Type"])
        self.assertEqual("p8195", self.metadata.file.info["Proposal"])
        self.assertEqual(128, self.metadata.file.info["DataSizeX"])
        self.assertEqual("16384", self.metadata.file.info["DataSize"])

    def test_CheckSampleSectionValues(self):
        self.assertEqual("-0.00", self.metadata.sample.info["Omega"])
        self.assertEqual("10.00", self.metadata.sample.info["BTableX"])
        self.assertEqual("0.00", self.metadata.sample.info["BTableY"])
        self.assertEqual("28.00", self.metadata.sample.info["BTableZ"])
        self.assertEqual("", self.metadata.sample.info["Magnet"])

    def test_CheckSetupSectionValues(self):
        self.assertEqual("0.000000", self.metadata.setup.info["DetHAngle"])
        self.assertEqual("495.00", self.metadata.setup.info["BeamstopX"])
        self.assertEqual("497.00", self.metadata.setup.info["BeamstopY"])
        self.assertEqual("0.000", self.metadata.setup.info["Polarization_m"])
        self.assertEqual("26.554", self.metadata.setup.info["Polarization_c"])

    def test_MonitorExist(self):
        self.assertEqual([6392861.0, 14902342.0], self.metadata.counter.get_monitors())

    def test_CheckCounterSectionValues(self):
        self.assertEqual(6392861, self.metadata.counter.monitor1)
        self.assertEqual(14902342, self.metadata.counter.monitor2)
        self.assertEqual(3600.688081, self.metadata.counter.duration)
        self.assertEqual(18234082, self.metadata.counter.sum_all_counts)

    def test_CheckCommentSectionValues(self):
        self.assertEqual("'4.0 mm'", self.metadata.comment.info["det1_x_value"])
        self.assertEqual("'10.00 mm'", self.metadata.comment.info["st1_x_value"])
        self.assertEqual("'0.0 deg'", self.metadata.comment.info["det1_omg_value"])

    def test_Dimensions(self):
        dim = self.metadata.counts.data.shape
        self.assertEqual((128, 128), dim)

    @staticmethod
    def get_file_absolute_path(filename):
        path = next(i for i in config.getDataSearchDirs() if os.path.isfile(os.path.join(i, filename)))
        path = os.path.join(path, filename)
        return path


if __name__ == "__main__":
    unittest.main()
