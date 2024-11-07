# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup, Run
from mantid.simpleapi import config, mtd, D4ILLReduction, Integration
from mantid.geometry import Instrument
import numpy as np
from os import remove, path


class D4ILLReductionTest(unittest.TestCase):
    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir("ILL/D4/")

    def setUp(self):
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]

        config["default.facility"] = "ILL"
        config["default.instrument"] = "D4C"

    def tearDown(self):
        if self._facility:
            config["default.facility"] = self._facility
        if self._instrument:
            config["default.instrument"] = self._instrument
        mtd.clear()

    def test_single_run_default(self):
        output_ws = "single_run_default"
        D4ILLReduction(Run="387230", OutputWorkspace=output_ws, ExportAscii=False)
        self._check_output(mtd[output_ws], 256, 1, 2, ["Scattering Angle", "q"], ["Label", "MomentumTransfer"], "Height", "Label")

    def test_multi_run_default(self):
        output_ws = "multi_run_default"
        D4ILLReduction(Run="387229:387230", OutputWorkspace=output_ws, ExportAscii=False)
        self._check_output(mtd[output_ws], 258, 1, 2, ["Scattering Angle", "q"], ["Label", "MomentumTransfer"], "Height", "Label")

    def test_save_ascii(self):
        output_ws = "multi_run_default"
        D4ILLReduction(Run="387229:387230", OutputWorkspace=output_ws, ExportAscii=True)
        self.assertTrue(path.exists(path.join(config["defaultsave.directory"], "{}_diffractogram_2theta.dat".format(output_ws))))
        self.assertTrue(path.exists(path.join(config["defaultsave.directory"], "{}_diffractogram_q.dat".format(output_ws))))
        remove(path.join(config["defaultsave.directory"], "{}_diffractogram_2theta.dat".format(output_ws)))  # clean up the temporary file
        remove(path.join(config["defaultsave.directory"], "{}_diffractogram_q.dat".format(output_ws)))  # clean up the temporary file

    def test_rotation(self):
        output_ws = "single_run_rotation"
        D4ILLReduction(Run="387230", OutputWorkspace=output_ws, ZeroPositionAngle=10, ExportAscii=True)
        min_angular_range = mtd[output_ws][0].readX(0)[0]
        max_angular_range = mtd[output_ws][0].readX(0)[256]
        self.assertAlmostEqual(min_angular_range, 19.70, delta=1e-2)
        self.assertAlmostEqual(max_angular_range, 147.70, delta=1e-2)

    def test_normalise_to_monitor(self):
        output_ws = "norm_to_monitor"
        D4ILLReduction(Run="387230", OutputWorkspace=output_ws, NormaliseBy="Monitor", ExportAscii=True)
        self._check_output(mtd[output_ws], 256, 1, 2, ["Scattering Angle", "q"], ["Label", "MomentumTransfer"], "Height", "Label")
        integrated_ws = "integration_ws"
        Integration(InputWorkspace=output_ws, OutputWorkspace=integrated_ws)
        self.assertEqual(mtd[integrated_ws][0].readY(0)[0], mtd[integrated_ws][1].readY(0)[0])
        self.assertAlmostEqual(mtd[integrated_ws][0].readY(0)[0], 2769365.5, delta=1e-1)

    def test_normalise_to_time(self):
        output_ws = "norm_to_time"
        D4ILLReduction(Run="387230", OutputWorkspace=output_ws, NormaliseBy="Time", NormalisationStandard=80, ExportAscii=True)
        self._check_output(mtd[output_ws], 256, 1, 2, ["Scattering Angle", "q"], ["Label", "MomentumTransfer"], "Height", "Label")
        integrated_ws = "integration_ws"
        Integration(InputWorkspace=output_ws, OutputWorkspace=integrated_ws)
        self.assertEqual(mtd[integrated_ws][0].readY(0)[0], mtd[integrated_ws][1].readY(0)[0])
        self.assertAlmostEqual(mtd[integrated_ws][0].readY(0)[0], 2762649.6, delta=1e-1)

    def test_calibrate_bank_positions(self):
        bank_positions = np.linspace(0, 9, 9)
        calibration_file = "calibration.dat"
        with open(calibration_file, "w") as f:
            for bank_no, bank_pos in enumerate(bank_positions):
                f.write("{}\t{}\n".format(bank_no, bank_pos))
        output_ws = "calibrate_positions"
        D4ILLReduction(Run="387230", OutputWorkspace=output_ws, BankPositionOffsetsFile=calibration_file, ExportAscii=True)
        remove(calibration_file)  # clean up the temporary file
        min_angular_range = mtd[output_ws][0].readX(0)[0]
        max_angular_range = mtd[output_ws][0].readX(0)[256]
        self.assertAlmostEqual(min_angular_range, 9.68, delta=1e-2)
        self.assertAlmostEqual(max_angular_range, 137.68, delta=1e-2)

    def _check_output(self, ws, blocksize, spectra, n_entries, x_unit, x_unit_id, y_unit, y_unit_id):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), n_entries)
        for entry_no, entry in enumerate(ws):
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(entry.isHistogramData())
            self.assertTrue(not entry.isDistribution())
            self.assertEqual(entry.getAxis(0).getUnit().caption(), x_unit[entry_no])
            self.assertEqual(entry.getAxis(0).getUnit().unitID(), x_unit_id[entry_no])
            self.assertEqual(entry.getAxis(1).getUnit().caption(), y_unit)
            self.assertEqual(entry.getAxis(1).getUnit().unitID(), y_unit_id)
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(isinstance(entry.getInstrument(), Instrument))
            self.assertTrue(isinstance(entry.getRun(), Run))
            self.assertTrue(entry.getHistory())


if __name__ == "__main__":
    unittest.main()
