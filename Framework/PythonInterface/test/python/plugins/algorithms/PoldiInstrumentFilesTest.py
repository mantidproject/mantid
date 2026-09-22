# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Pin which POLDI definition and parameter file is selected for a given run date.

POLDI ships several dated instrument definitions and parameter files.  Which one a run gets is
decided by InstrumentFileFinder from the valid-from/valid-to attributes, and nothing else in the
test suite covers that, so a mistake in those dates is silent.  These tests make it loud.
"""

import unittest
from os.path import basename
from mantid.api import InstrumentFileFinder
from mantid.simpleapi import LoadEmptyInstrument, AnalysisDataService


class PoldiInstrumentFilesTest(unittest.TestCase):
    # (run date, expected definition, expected parameter file)
    EXPECTED = (
        ("2005-06-01 00:00:00", "POLDI_Definition400.xml", "POLDI_Parameters400.xml"),
        ("2013-06-01 00:00:00", "POLDI_Definition_2013.xml", "POLDI_Parameters_2013.xml"),
        # note 2014e precedes 2014d by date - the letters do not follow chronology
        ("2014-09-07 00:00:00", "POLDI_Definition_2014e.xml", "POLDI_Parameters_2014e.xml"),
        ("2014-09-12 00:00:00", "POLDI_Definition_2014d.xml", "POLDI_Parameters_2014d.xml"),
        ("2016-01-01 00:00:00", "POLDI_Definition_2015.xml", "POLDI_Parameters_2015.xml"),
        ("2026-01-01 00:00:00", "POLDI_Definition_896.xml", "POLDI_Parameters.xml"),
    )

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_definition_selected_by_run_date(self):
        for date, expected_idf, _ in self.EXPECTED:
            with self.subTest(date=date):
                self.assertEqual(basename(InstrumentFileFinder.getInstrumentFilename("POLDI", date)), expected_idf)

    def test_parameter_file_selected_by_run_date(self):
        for date, _, expected_ipf in self.EXPECTED:
            with self.subTest(date=date):
                self.assertEqual(basename(InstrumentFileFinder.getParameterFilename("POLDI", date, "")), expected_ipf)

    def test_chopper_parameters_applied_to_current_definition(self):
        ws = LoadEmptyInstrument(
            Filename=InstrumentFileFinder.getInstrumentFilename("POLDI", "2026-01-01 00:00:00"), OutputWorkspace="poldi"
        )
        chopper = ws.getInstrument().getComponentByName("chopper")
        self.assertAlmostEqual(chopper.getNumberParameter("t0")[0], 5.824e-02, places=6)
        self.assertAlmostEqual(chopper.getNumberParameter("t0_const")[0], -12.68, places=3)


if __name__ == "__main__":
    unittest.main()
