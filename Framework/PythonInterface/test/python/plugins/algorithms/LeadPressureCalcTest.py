# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import mtd, LeadPressureCalc


class LeadPressureCalcTest(unittest.TestCase):
    def test_negative_temp_fails_input_validation(self):
        d = 2.1
        temp = -30.0
        self.assertRaises(ValueError, LeadPressureCalc, dSpacing=d, T=temp)

    def test_oor_dspacing_fails_input_validation(self):
        d1 = 1.9
        d2 = 3.1
        temp = 300
        self.assertRaises(ValueError, LeadPressureCalc, dSpacing=d1, T=temp)
        self.assertRaises(ValueError, LeadPressureCalc, dSpacing=d2, T=temp)

    def test_negative_target_pressure_fails_input_validation(self):
        d = 2.1
        temp = 300
        tgt_p = -30
        self.assertRaises(ValueError, LeadPressureCalc, dSpacing=d, T=temp, TargetPressure=tgt_p)

    def test_valid_run_produces_table_ws(self):
        d = 2.1
        temp = 300
        LeadPressureCalc(d, temp)
        self.assertIn('LeadPressureCalcResults', mtd)

    def test_valid_run_no_tgt_pressure_gives_correct_tws(self):
        d = 2.1
        temp = 300
        LeadPressureCalc(d, temp)
        ws = mtd['LeadPressureCalcResults']
        self.assertEqual(len(ws.toDict().keys()), 6)
        self.assertAlmostEqual(ws.row(0)["Calculated Pressure (GPa)"], ws.row(0)["Pressure Target (GPa)"], 7)

    def test_valid_run_with_tgt_pressure_gives_correct_tws(self):
        d = 2.1
        temp = 300
        tgt_p = 0.000123
        LeadPressureCalc(d, temp, tgt_p)
        ws = mtd['LeadPressureCalcResults']
        self.assertEqual(len(ws.toDict().keys()), 6)
        self.assertAlmostEqual(tgt_p, ws.row(0)["Pressure Target (GPa)"], 7)

    def test_valid_run_with_bad_tgt_pressure_gives_correct_tws(self):
        d = 2.1
        temp = 300
        tgt_p = 123456
        LeadPressureCalc(d, temp, tgt_p)
        ws = mtd['LeadPressureCalcResults']
        self.assertEqual(len(ws.toDict().keys()), 3)
        self.assertNotIn("Pressure Target (GPa)", ws.row(0).keys())


if __name__ == '__main__':
    unittest.main()
