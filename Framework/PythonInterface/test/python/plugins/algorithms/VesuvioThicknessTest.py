# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import VesuvioThickness
from mantid.api import ITableWorkspace


class VesuvioThicknessTest(unittest.TestCase):
    # ----------------------------------Algorithm tests----------------------------------------

    def test_basic_input(self):
        # Original test values from fortran routines
        masses = [1.0079, 27.0, 91.0]
        amplitudes = [0.9301589, 2.9496644e-02, 4.0345035e-02]
        trans_guess = 0.831
        thickness = 5.0
        number_density = 1.0
        dens_table, trans_table = VesuvioThickness(
            Masses=masses, Amplitudes=amplitudes, TransmissionGuess=trans_guess, Thickness=thickness, NumberDensity=number_density
        )
        # Validate shape
        self._validate_shape(dens_table)
        self._validate_shape(trans_table)
        self.assertAlmostEqual(dens_table.cell(0, 1), 22.4062053)
        self.assertAlmostEqual(dens_table.cell(9, 1), 24.4514601)
        self.assertAlmostEqual(trans_table.cell(0, 1), 0.99245745)
        self.assertAlmostEqual(trans_table.cell(9, 1), 0.83100000)

    # ----------------------------------Failure cases------------------------------------------------

    def test_bad_input(self):
        masses = ["test", "bad", "input"]
        amplitudes = ["test", "bad", "input"]
        self.assertRaises(
            TypeError,
            VesuvioThickness,
            Masses=masses,
            Amplitudes=amplitudes,
            DensityWorkspace="dens_tbl",
            TransmissionWorkspace="trans_tbl",
        )

    def test_mismatch_mass_amplitude_inputs(self):
        masses = [1.0, 2.0, 3.0, 4.0]
        amplitudes = [1.0, 2.0]
        self.assertRaisesRegex(
            RuntimeError,
            "The number of masses: 4, is not equal to the number of amplitudes: 2",
            VesuvioThickness,
            Masses=masses,
            Amplitudes=amplitudes,
            DensityWorkspace="dens_tbl",
            TransmissionWorkspace="trans_tbl",
        )

    def test_no_masses_input(self):
        masses = []
        amplitudes = [1.0, 2.0]
        self.assertRaisesRegex(
            RuntimeError,
            "Must have 1 or more Masses defined",
            VesuvioThickness,
            Masses=masses,
            Amplitudes=amplitudes,
            DensityWorkspace="dens_tbl",
            TransmissionWorkspace="trans_tbl",
        )

    def test_no_amplitudes_input(self):
        masses = [1.0, 2.0]
        amplitudes = []
        self.assertRaisesRegex(
            RuntimeError,
            "Must have 1 or more Amplitudes defined",
            VesuvioThickness,
            Masses=masses,
            Amplitudes=amplitudes,
            DensityWorkspace="dens_tbl",
            TransmissionWorkspace="trans_tbl",
        )

    # --------------------------------Validate results------------------------------------------------
    def _validate_shape(self, table_ws):
        self.assertTrue(isinstance(table_ws, ITableWorkspace))
        self.assertEqual(table_ws.columnCount(), 2)
        self.assertEqual(table_ws.rowCount(), 10)
        self.assertEqual(table_ws.cell(0, 0), str(1))
        self.assertEqual(table_ws.cell(9, 0), str(10))


if __name__ == "__main__":
    unittest.main()
