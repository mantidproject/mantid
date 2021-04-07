# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import *

"""
This algorithm is still in development
So the final results are likely to change.
These tests will only check the validation
of inputs
"""

class VesuvioAnalysisTest(unittest.TestCase):

    def set_up_alg(self):
        alg = AlgorithmManager.create('VesuvioAnalysis')
        alg.setChild(True)
        alg.initialize()
        return alg


    def generate_table(self):
        table = CreateEmptyTableWorkspace()
        table.addColumn(type="str", name="Symbol")
        table.addColumn(type="double", name="Mass (a.u.)")
        table.addColumn(type="double", name="Intensity lower limit")
        table.addColumn(type="double", name="Intensity value")
        table.addColumn(type="double", name="Intensity upper limit")
        table.addColumn(type="double", name="Width lower limit")
        table.addColumn(type="double", name="Width value")
        table.addColumn(type="double", name="Width upper limit")
        table.addColumn(type="double", name="Centre lower limit")
        table.addColumn(type="double", name="Centre value")
        table.addColumn(type="double", name="Centre upper limit")
        table.addRow(['H', 1.0079,  0.,1.,9.9e9,  3.,  4.5,  6.,  -1.5, 0., 0.5])
        table.addRow(['C', 12.0,    0.,1.,9.9e9,  10., 15.5, 30., -1.5, 0., 0.5])

        return table

    def test_no_elements(self):
        alg = self.set_up_alg()
        alg.setProperty('Spectra', [135, 182])
        errors = alg.validateInputs()
        self.assertTrue("ComptonProfile" in errors)
        self.assertEquals(len(errors),1)


    def test_bad_column_in_table(self):
        table = CreateEmptyTableWorkspace()
        table.addColumn(type="str", name="Symbol")
        table.addColumn(type="double", name="Intensity lower limit")
        table.addColumn(type="double", name="Intensity value")
        table.addColumn(type="double", name="Intensity upper limit")
        table.addColumn(type="double", name="Width lower limit")
        table.addColumn(type="double", name="Width value")
        table.addColumn(type="double", name="Width upper limit")
        table.addColumn(type="double", name="Centre lower limit")
        table.addColumn(type="double", name="Centre value")
        table.addColumn(type="double", name="Centre upper limit")
        table.addRow(['H', 0.,1.,9.9e9,  3.,  4.5,  6.,  -1.5, 0., 0.5])
        table.addRow(['C', 0.,1.,9.9e9,  10., 15.5, 30., -1.5, 0., 0.5])

        alg = self.set_up_alg()
        alg.setProperty('Spectra', [135, 182])
        alg.setProperty('ComptonProfile', table)
        errors = alg.validateInputs()
        self.assertTrue("ComptonProfile" in errors)
        self.assertEquals(len(errors),1)

    def test_case_instensitive_table(self):
        table = CreateEmptyTableWorkspace()
        table.addColumn(type="str", name="Symbol")
        table.addColumn(type="double", name="mAsS (a.u.)")
        table.addColumn(type="double", name="Intensity lower limit")
        table.addColumn(type="double", name="Intensity value")
        table.addColumn(type="double", name="Intensity upper limit")
        table.addColumn(type="double", name="Width lower limit")
        table.addColumn(type="double", name="Width value")
        table.addColumn(type="double", name="Width upper limit")
        table.addColumn(type="double", name="Centre lower limit")
        table.addColumn(type="double", name="Centre value")
        table.addColumn(type="double", name="Centre upper limit")
        table.addRow(['H', 1.0079,  0.,1.,9.9e9,  3.,  4.5,  6.,  -1.5, 0., 0.5])
        table.addRow(['C', 12.0,    0.,1.,9.9e9,  10., 15.5, 30., -1.5, 0., 0.5])

        alg = self.set_up_alg()
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])
        errors = alg.validateInputs()
        self.assertEquals(len(errors),0)

    def test_TOF_range_short(self):
        table = self.generate_table()
        alg = self.set_up_alg()
        alg.setProperty('TOFRangeVector', [1,2])
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])

        errors = alg.validateInputs()
        self.assertEquals(len(errors),1)
        self.assertTrue("TOFRangeVector" in errors)

    def test_TOF_range_long(self):
        table = self.generate_table()
        alg = self.set_up_alg()
        alg.setProperty('TOFRangeVector', [1,2,3,4])
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])

        errors = alg.validateInputs()
        self.assertEquals(len(errors),1)
        self.assertTrue("TOFRangeVector" in errors)

    def test_constraints_short(self):
        table = self.generate_table()
        alg = self.set_up_alg()
        alg.setProperty('ConstraintsProfileNumbers', [1])
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])

        errors = alg.validateInputs()
        self.assertEquals(len(errors),1)
        self.assertTrue("ConstraintsProfileNumbers" in errors)

    def test_constraints_long(self):
        table = self.generate_table()
        alg = self.set_up_alg()
        alg.setProperty('ConstraintsProfileNumbers', [1,2,3])
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])

        errors = alg.validateInputs()
        self.assertEquals(len(errors),1)
        self.assertTrue("ConstraintsProfileNumbers" in errors)

    def test_maths_is_safe_fails(self):
        table = self.generate_table()
        bad_expressions = [ "2*r", "2,3", "4£", "rm -r *"]
        for expression in bad_expressions:
            alg = self.set_up_alg()
            alg.setProperty('ConstraintsProfileScatteringCrossSection',expression )
            alg.setProperty('ComptonProfile', table)
            alg.setProperty('Spectra', [135, 182])
            errors = alg.validateInputs()
            self.assertEquals(len(errors),1)
            self.assertTrue("ConstraintsProfileScatteringCrossSection" in errors)

    def test_maths_is_safe_pass(self):
        table = self.generate_table()
        good_expressions = [ "2*3", "2+3", "4-1", "5/2", "(3+2)", "2.3+4.5"]
        for expression in good_expressions:
            alg = self.set_up_alg()
            alg.setProperty('ConstraintsProfileScatteringCrossSection',expression )
            alg.setProperty('ComptonProfile', table)
            alg.setProperty('Spectra', [135, 182])
            errors = alg.validateInputs()
            self.assertEquals(len(errors),0)

    def test_run_string_correct(self):
        table = self.generate_table()
        good_runs = ["12-40", "1,2,3", "12-50,34"]
        for run in good_runs:
            alg = self.set_up_alg()
            alg.setProperty('ComptonProfile', table)
            alg.setProperty('Spectra', [135, 182])
            alg.setProperty("Runs", run)
            errors = alg.validateInputs()
            self.assertEquals(len(errors),0)

    def test_run_string_incorrect(self):
        table = self.generate_table()
        bad_runs = ["12.4","five"]
        for run in bad_runs:
            alg = self.set_up_alg()
            alg.setProperty('ComptonProfile', table)
            alg.setProperty('Spectra', [135, 182])
            alg.setProperty("Runs",run)
            errors = alg.validateInputs()
            self.assertEquals(len(errors),1)
            self.assertTrue("Runs" in errors)

    def test_masked_sapectra_correct(self):
        table = self.generate_table()
        alg = self.set_up_alg()
        alg.setProperty('ComptonProfile', table)
        alg.setProperty('Spectra', [135, 182])
        alg.setProperty('SpectraToBeMasked', [135, 165, 182])
        errors = alg.validateInputs()
        self.assertEquals(len(errors),0)

    def test_masked_sapectra_incorrect(self):
        table = self.generate_table()
        bad_spec = ["1,135","135,182,190", "135,183"]
        for spec in bad_spec:
            alg = self.set_up_alg()
            alg.setProperty('ComptonProfile', table)
            alg.setProperty('Spectra', [135, 182])
            alg.setProperty('SpectraToBeMasked', spec)
            errors = alg.validateInputs()
            self.assertEquals(len(errors),1)
            self.assertTrue("SpectraToBeMasked" in errors)

if __name__ == '__main__':
    unittest.main()
