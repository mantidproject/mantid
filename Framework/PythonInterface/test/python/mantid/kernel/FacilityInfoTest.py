# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import FacilityInfo, InstrumentInfo, ConfigService
import pytz


class FacilityInfoTest(unittest.TestCase):
    def test_construction_raies_an_error(self):
        self.assertRaises(RuntimeError, FacilityInfo)

    def _get_test_facility(self):
        return ConfigService.getFacility("ISIS")

    def test_attributes_are_as_expected(self):
        test_facility = self._get_test_facility()

        self.assertEqual(test_facility.name(), "ISIS")
        self.assertEqual(test_facility.zeroPadding(), 5)
        self.assertEqual(test_facility.delimiter(), "")
        self.assertEqual(len(test_facility.extensions()), 7)
        self.assertEqual(test_facility.preferredExtension(), ".nxs")
        self.assertEqual(len(test_facility.archiveSearch()), 1)
        self.assertGreater(len(test_facility.instruments()), 30)
        self.assertTrue(len(test_facility.instruments("Neutron Diffraction")) > 10)
        self.assertTrue(isinstance(test_facility.instrument("WISH"), InstrumentInfo))
        self.assertEqual(test_facility.timezone(), "Europe/London")

    def test_timezones(self):
        # verify that all of the timezones can get converted by pytz
        for facility in ConfigService.getFacilities():
            if len(facility.timezone()) == 0:
                continue  # don't test empty strings
            tz = pytz.timezone(facility.timezone())
            print(facility.name(), tz)
            self.assertEqual(str(tz), facility.timezone())


if __name__ == "__main__":
    unittest.main()
