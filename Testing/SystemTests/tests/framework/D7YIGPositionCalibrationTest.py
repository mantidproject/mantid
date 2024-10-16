# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd, ITableWorkspace
from mantid.kernel import config, ConfigService
from mantid.simpleapi import CloneWorkspace, D7YIGPositionCalibration, DeleteWorkspace, Load, LoadILLPolarizedDiffraction

import math
import os.path
from os import path
import xml.etree.ElementTree as ET


class D7YIGPositionCalibrationTest(systemtesting.MantidSystemTest):
    _pixels_per_bank = 44

    def __init__(self):
        super(D7YIGPositionCalibrationTest, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"
        config.appendDataSearchSubDir("ILL/D7/")
        Load("402652_403041.nxs", OutputWorkspace="shortWavelengthScan")
        Load("396442_396831.nxs", OutputWorkspace="intermediateWavelengthScan")
        Load("394458_394882.nxs", OutputWorkspace="longWavelengthScan")

    def cleanup(self):
        mtd.clear()
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_shortWavelength.xml"))
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_shortWavelength_global.xml"))
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_intermediateWavelength.xml"))
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_intermediateWavelength_global.xml"))
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_longWavelength.xml"))
        os.remove(os.path.join(ConfigService.Instance().getString("defaultsave.directory"), "test_longWavelength_global.xml"))

    def d7_short_wavelength_test(self):
        approximate_wavelength = "3.14"  # Angstrom
        fit_output_workspace = "test_shortWavelength"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="shortWavelengthScan", OutputWorkspace="shortWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="shortWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MinimalDistanceBetweenPeaks=1.75,
            BankOffsets=[3, 3, -1],
            ClearCache=True,
            FitOutputWorkspace=fit_output_workspace,
            FittingMethod="Individual",
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_shortWavelength"])
        self.assertTrue(isinstance(mtd["test_shortWavelength"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_load_data_with_calibration(calibration_output_path)
        self._check_absolute_positions("short", calibration_output_path)

    def d7_short_wavelength_global_test(self):
        approximate_wavelength = "3.14"  # Angstrom
        fit_output_workspace = "test_shortWavelength_global"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="shortWavelengthScan", OutputWorkspace="shortWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="shortWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MinimalDistanceBetweenPeaks=1.75,
            BankOffsets=[3, 3, -1],
            ClearCache=True,
            FitOutputWorkspace=fit_output_workspace,
            FittingMethod="Global",
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_shortWavelength_global"])
        self.assertTrue(isinstance(mtd["test_shortWavelength_global"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_absolute_positions("short", calibration_output_path)

    def d7_intermediate_wavelength_test(self):
        approximate_wavelength = "4.8"  # Angstrom
        fit_output_workspace = "test_intermediateWavelength"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="intermediateWavelengthScan", OutputWorkspace="intermediateWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="intermediateWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MaskedBinsRange=[-50, -25, 15],
            MinimalDistanceBetweenPeaks=1.75,
            BankOffsets=[2, 3, 1],
            ClearCache=True,
            FittingMethod="Individual",
            FitOutputWorkspace=fit_output_workspace,
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_intermediateWavelength"])
        self.assertTrue(isinstance(mtd["test_intermediateWavelength"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_load_data_with_calibration(calibration_output_path)
        self._check_absolute_positions("intermediate", calibration_output_path)

    def d7_intermediate_wavelength_global_test(self):
        approximate_wavelength = "4.8"  # Angstrom
        fit_output_workspace = "test_intermediateWavelength_global"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="intermediateWavelengthScan", OutputWorkspace="intermediateWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="intermediateWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MaskedBinsRange=[-50, -25, 15],
            MinimalDistanceBetweenPeaks=1.5,
            BankOffsets=[2, 3, 1],
            ClearCache=True,
            FittingMethod="Global",
            FitOutputWorkspace=fit_output_workspace,
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_intermediateWavelength_global"])
        self.assertTrue(isinstance(mtd["test_intermediateWavelength_global"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_load_data_with_calibration(calibration_output_path)
        self._check_absolute_positions("intermediate", calibration_output_path)

    def d7_long_wavelength_test(self):
        approximate_wavelength = "5.7"  # Angstrom
        fit_output_workspace = "test_longWavelength"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="longWavelengthScan", OutputWorkspace="longWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="longWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MinimalDistanceBetweenPeaks=1.5,
            BankOffsets=[2, 2, -1],
            ClearCache=True,
            FitOutputWorkspace=fit_output_workspace,
            FittingMethod="Individual",
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_longWavelength"])
        self.assertTrue(isinstance(mtd["test_longWavelength"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_load_data_with_calibration(calibration_output_path)
        self._check_absolute_positions("long", calibration_output_path)

    def d7_long_wavelength_global_test(self):
        approximate_wavelength = "5.7"  # Angstrom
        fit_output_workspace = "test_longWavelength_global"
        calibration_output_path = os.path.join(
            ConfigService.Instance().getString("defaultsave.directory"), "{}.xml".format(fit_output_workspace)
        )
        CloneWorkspace(InputWorkspace="longWavelengthScan", OutputWorkspace="longWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="longWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=calibration_output_path,
            MinimalDistanceBetweenPeaks=1.5,
            BankOffsets=[2, 2, -1],
            ClearCache=True,
            FitOutputWorkspace=fit_output_workspace,
            FittingMethod="Global",
        )
        self.assertTrue(path.exists(calibration_output_path))
        self.assertTrue(mtd["test_longWavelength_global"])
        self.assertTrue(isinstance(mtd["test_longWavelength_global"], ITableWorkspace))
        self._check_fit_output(fit_output_workspace)
        self._check_load_data_with_calibration(calibration_output_path)
        self._check_absolute_positions("long", calibration_output_path)

    def _check_fit_output(self, fitTableName):
        """Checks the TableWorkspace if the output values are reasonable,
        then check if the output IPF can be read by the Loader"""

        self.assertNotEqual(mtd[fitTableName], None)
        self.assertTrue(isinstance(mtd[fitTableName], ITableWorkspace))

        wavelength = float(mtd[fitTableName].column(1)[1])
        self.assertAlmostEqual(wavelength, 1.0, delta=5e-2)  # +/- 5 %
        bank2_slope = 1.0 / float(mtd[fitTableName].column(1)[0])
        self.assertAlmostEqual(bank2_slope, 1.0, delta=2e-2)  # +/- 1 %
        bank3_slope = 1.0 / float(mtd[fitTableName].column(1)[4 * self._pixels_per_bank])
        self.assertAlmostEqual(bank3_slope, 1.0, delta=2e-2)  # +/- 1 %
        bank4_slope = 1.0 / float(mtd[fitTableName].column(1)[8 * self._pixels_per_bank])
        self.assertAlmostEqual(bank4_slope, 1.0, delta=2e-2)  # +/- 1 %

        for row_no in range(mtd[fitTableName].rowCount()):
            row_data = mtd[fitTableName].row(row_no)
            if ".offset" in row_data["Name"]:
                offset = row_data["Value"]
                self.assertAlmostEqual(offset, 0.0, delta=24.0)  # +- 24 degrees

    def _check_load_data_with_calibration(self, ipf_name):
        tree = ET.parse(ipf_name)
        self.assertNotEqual(tree, None)
        root = tree.getroot()
        bank_offsets = []
        bank_gradients = []
        pixel_offsets = []
        for elem in root:
            for subelem in elem:
                for value in subelem:
                    value = float(value.attrib["val"])
                    if "pixel" in subelem.attrib["name"]:
                        pixel_offsets.append(value)
                    if "gradient" in subelem.attrib["name"]:
                        bank_gradients.append(value)
                    if "offset" in subelem.attrib["name"]:
                        bank_offsets.append(value)
        positionCalibration = [
            pixel_offset + bank_offsets[math.floor(pixel_no / self._pixels_per_bank)] for pixel_no, pixel_offset in enumerate(pixel_offsets)
        ]
        self.assertEqual(len(positionCalibration), 132)  # number of pixels
        LoadILLPolarizedDiffraction(
            "401800",
            OutputWorkspace="output",
            PositionCalibration="YIGFile",
            YIGFilename=ipf_name,
            ConvertToScatteringAngle=True,
            TransposeMonochromatic=True,
        )
        self.assertNotEqual("output", None)
        nexus_bank_offsets = [
            mtd["output"].getItem(0).getRun().getLogData("2theta.actual_bank{}".format(bank_no + 2)).value for bank_no in range(3)
        ]

        xAxisValues = mtd["output"].getItem(0).readX(0)
        self.assertAlmostEqual(xAxisValues[0], bank_gradients[0] * nexus_bank_offsets[0] - positionCalibration[0], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[43], bank_gradients[0] * nexus_bank_offsets[0] - positionCalibration[43], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[44], bank_gradients[1] * nexus_bank_offsets[1] - positionCalibration[44], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[87], bank_gradients[1] * nexus_bank_offsets[1] - positionCalibration[87], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[88], bank_gradients[2] * nexus_bank_offsets[2] - positionCalibration[88], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[131], bank_gradients[2] * nexus_bank_offsets[2] - positionCalibration[131], delta=1e-2)
        DeleteWorkspace(Workspace="output")  # clean-up

    def _check_absolute_positions(self, wavelength_mode, ipf_name):
        """Checks whether the calibration provides expected detector positions within margin,
        and indirectly checks whether individual and global methods provide consistent output."""

        if wavelength_mode == "short":
            LoadILLPolarizedDiffraction(
                "403041",
                OutputWorkspace="calibration_test",
                PositionCalibration="YIGFile",
                YIGFilename=ipf_name,
                ConvertToScatteringAngle=True,
                TransposeMonochromatic=True,
            )
        elif wavelength_mode == "intermediate":
            LoadILLPolarizedDiffraction(
                "396831",
                OutputWorkspace="calibration_test",
                PositionCalibration="YIGFile",
                YIGFilename=ipf_name,
                ConvertToScatteringAngle=True,
                TransposeMonochromatic=True,
            )
        elif wavelength_mode == "long":
            LoadILLPolarizedDiffraction(
                "394882",
                OutputWorkspace="calibration_test",
                PositionCalibration="YIGFile",
                YIGFilename=ipf_name,
                ConvertToScatteringAngle=True,
                TransposeMonochromatic=True,
            )
        xAxisValues = mtd["calibration_test"].getItem(0).readX(0)
        self.assertAlmostEqual(xAxisValues[0], 14.3, delta=2e0)
        self.assertAlmostEqual(xAxisValues[43], 56.8, delta=6e0)
        self.assertAlmostEqual(xAxisValues[44], 60.3, delta=5e0)
        self.assertAlmostEqual(xAxisValues[87], 102.6, delta=1e0)
        self.assertAlmostEqual(xAxisValues[88], 105.9, delta=1e0)
        self.assertAlmostEqual(xAxisValues[131], 148.6, delta=1e0)
        DeleteWorkspace(Workspace="calibration_test")  # clean-up

    def runTest(self):
        self.d7_short_wavelength_test()
        self.d7_short_wavelength_global_test()
        self.d7_intermediate_wavelength_test()
        self.d7_intermediate_wavelength_global_test()
        self.d7_long_wavelength_test()
        self.d7_long_wavelength_global_test()
