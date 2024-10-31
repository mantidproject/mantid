# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup, Run
from mantid.simpleapi import config, mtd, PolDiffILLReduction, CreateEmptyTableWorkspace
from mantid.geometry import Instrument


class PolDiffILLReductionTest(unittest.TestCase):
    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir("ILL/D7/")

    def setUp(self):
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]

        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"

    def tearDown(self):
        if self._facility:
            config["default.facility"] = self._facility
        if self._instrument:
            config["default.instrument"] = self._instrument
        mtd.clear()

    def test_absorber_transmission(self):
        PolDiffILLReduction(Run="396991", ProcessAs="BeamWithCadmium", OutputWorkspace="cadmium_ws")
        self._check_output(mtd["cadmium_ws"], 1, 1, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["cadmium_ws"], "BeamWithCadmium")
        self.assertAlmostEqual(mtd["cadmium_ws_1"].readY(0)[0], 0.06, delta=1e-3)

    def test_absorber_transmission_norm_by_time(self):
        PolDiffILLReduction(Run="396991", ProcessAs="BeamWithCadmium", OutputWorkspace="cadmium_ws", NormaliseBy="Time")
        self._check_output(mtd["cadmium_ws"], 1, 1, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["cadmium_ws"], "BeamWithCadmium")
        self.assertAlmostEqual(mtd["cadmium_ws_1"].readY(0)[0], 0.00773, delta=1e-3)

    def test_beam(self):
        PolDiffILLReduction(Run="396983", ProcessAs="EmptyBeam", OutputWorkspace="beam_ws")
        self._check_output(mtd["beam_ws"], 1, 1, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["beam_ws"], "EmptyBeam")
        self.assertAlmostEqual(mtd["beam_ws_1"].readY(0)[0], 5.566, delta=1e-3)

    def test_transmission(self):
        PolDiffILLReduction(Run="396983", ProcessAs="EmptyBeam", OutputWorkspace="beam_ws")
        PolDiffILLReduction(Run="396991", ProcessAs="BeamWithCadmium", OutputWorkspace="cadmium_ws")
        PolDiffILLReduction(
            Run="396985",
            ProcessAs="Transmission",
            OutputWorkspace="quartz_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            EmptyBeamWorkspace="beam_ws",
        )
        self._check_output(mtd["quartz_transmission"], 1, 1, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self.assertAlmostEqual(mtd["quartz_transmission_1"].readY(0)[0], 0.692, delta=1e-3)
        self._check_process_flag(mtd["quartz_transmission"], "Transmission")

    def test_absorber(self):
        PolDiffILLReduction(Run="396928", ProcessAs="Cadmium", OutputWorkspace="absorber_ws")
        self._check_output(mtd["absorber_ws"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["absorber_ws"], "Cadmium")

    def test_container(self):
        PolDiffILLReduction(Run="396917", ProcessAs="Empty", OutputWorkspace="container_ws")
        self._check_output(mtd["container_ws"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["container_ws"], "Empty")

    def test_quartz(self):
        PolDiffILLReduction(Run="396983", ProcessAs="EmptyBeam", OutputWorkspace="beam_ws")
        PolDiffILLReduction(Run="396985", ProcessAs="Transmission", OutputWorkspace="quartz_transmission", EmptyBeamWorkspace="beam_ws")
        PolDiffILLReduction(
            Run="396939", ProcessAs="Quartz", Transmission="quartz_transmission", OutputTreatment="AveragePol", OutputWorkspace="quartz"
        )
        self._check_output(mtd["quartz"], 1, 132, 3, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["quartz"], "Quartz")

    def test_quartz_transmission_as_value(self):
        PolDiffILLReduction(Run="396917", ProcessAs="Empty", OutputWorkspace="container_ws")
        quartz_transmission = "0.95124"
        PolDiffILLReduction(
            Run="396939",
            ProcessAs="Quartz",
            OutputWorkspace="quartz",
            Transmission=quartz_transmission,
            OutputTreatment="AveragePol",
            EmptyContainerWorkspace="container_ws",
        )
        self.assertTrue("quartz_transmission" in mtd)
        self.assertTrue(mtd["quartz_transmission"].readY(0)[0] == float(quartz_transmission))
        self._check_output(mtd["quartz"], 1, 132, 3, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["quartz"], "Quartz")

    def test_vanadium(self):
        sampleProperties = {"SampleMass": 8.54, "FormulaUnitMass": 50.94}
        PolDiffILLReduction(
            Run="396993",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium",
            SampleAndEnvironmentProperties=sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium"], "Vanadium")

    def test_vanadium_tof(self):
        sampleProperties = {"SampleMass": 8.54, "FormulaUnitMass": 50.94, "EPWidth": 15}
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"
        PolDiffILLReduction(
            Run="396016",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_tof",
            SampleAndEnvironmentProperties=sampleProperties,
            SampleGeometry="None",
            OutputTreatment="Sum",
            InstrumentCalibration=yig_calibration_file,
            MeasurementTechnique="TOF",
        )
        self.assertTrue("vanadium_tof" in mtd)
        self.assertTrue("vanadium_tof_elastic" in mtd)
        self._check_output(mtd["vanadium_tof"], 1, 132, 1, "Energy transfer", "DeltaE", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_tof"], "Vanadium")

    def test_vanadium_annulus(self):
        PolDiffILLReduction(Run="396917", ProcessAs="Empty", OutputWorkspace="container_ws")
        sampleProperties = {
            "SampleChemicalFormula": "V",
            "SampleMass": 8.54,
            "FormulaUnitMass": 50.94,
            "SampleInnerRadius": 2,
            "SampleOuterRadius": 2.5,
            "Height": 2,
            "BeamWidth": 2.6,
            "BeamHeight": 2.6,
            "SampleDensity": 1,
            "ContainerChemicalFormula": "Al",
            "ContainerDensity": 2.7,
            "ContainerInnerRadius": 0.1,
            "ContainerOuterRadius": 2.51,
            "EventsPerPoint": 1000,
        }
        PolDiffILLReduction(
            Run="396993",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_annulus",
            EmptyContainerWorkspace="container_ws",
            SampleAndEnvironmentProperties=sampleProperties,
            SelfAttenuationMethod="MonteCarlo",
            SampleGeometry="Annulus",
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_annulus"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_annulus"], "Vanadium")

    def test_sample(self):
        sampleProperties = {"SampleMass": 2.93, "FormulaUnitMass": 182.56}
        PolDiffILLReduction(
            Run="397004",
            ProcessAs="Sample",
            OutputWorkspace="sample",
            SampleAndEnvironmentProperties=sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["sample"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["sample"], "Sample")

    def test_sample_tof(self):
        # creates table workspace with mock elastic peak positions and widths:
        table_ws = CreateEmptyTableWorkspace()
        table_ws.addColumn("float", "PeakCentre")
        table_ws.addColumn("float", "Sigma")
        for row in range(132):
            table_ws.addRow([1645.2, 15.0])

        sampleProperties = {"SampleMass": 2.93, "FormulaUnitMass": 50.94, "EPWidth": 15}
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"
        PolDiffILLReduction(
            Run="395639",
            ProcessAs="Sample",
            OutputWorkspace="sample_tof",
            SampleAndEnvironmentProperties=sampleProperties,
            SampleGeometry="None",
            OutputTreatment="Individual",
            InstrumentCalibration=yig_calibration_file,
            ElasticChannelsWorkspace="table_ws",
            MeasurementTechnique="TOF",
        )
        self._check_output(mtd["sample_tof"], 339, 132, 2, "Energy transfer", "DeltaE", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_tof"], "Sample")

    def _check_process_flag(self, ws, value):
        self.assertEqual(ws[0].getRun().getLogData("ProcessedAs").value, value)

    def _check_output(self, ws, blocksize, spectra, nEntries, x_unit, x_unit_id, y_unit, y_unit_id):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), nEntries)
        for entry in ws:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(entry.isHistogramData())
            self.assertTrue(not entry.isDistribution())
            self.assertEqual(entry.getAxis(0).getUnit().caption(), x_unit)
            self.assertEqual(entry.getAxis(0).getUnit().unitID(), x_unit_id)
            self.assertEqual(entry.getAxis(1).getUnit().caption(), y_unit)
            self.assertEqual(entry.getAxis(1).getUnit().unitID(), y_unit_id)
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(isinstance(entry.getInstrument(), Instrument))
            self.assertTrue(isinstance(entry.getRun(), Run))
            self.assertTrue(entry.getHistory())


if __name__ == "__main__":
    unittest.main()
