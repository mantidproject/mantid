# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.simpleapi import LoadEmptyInstrument
import DirectILL_common as common


class DirectILL_commonTest(unittest.TestCase):
    _TEST_WS_NAME = "testWS_"
    _TEST_WS = None
    _facility = None
    _instrument = None

    def _create_instrument(self, instrument_name="IN5"):
        LoadEmptyInstrument(InstrumentName=instrument_name, OutputWorkspace=self._TEST_WS_NAME)
        return mtd[self._TEST_WS_NAME]

    def setUp(self):
        if self._TEST_WS is None:
            self._TEST_WS = self._create_instrument()

    def tearDown(self):
        mtd.clear()

    def test_GroupingPatternVerticalIN5(self):
        pattern = common.get_grouping_pattern(self._TEST_WS, vertical_step=2)
        self.assertTrue(pattern is not None)
        length = 98304 // 2
        first_element = "0-1"
        last_element = "98302-98303"
        self._test_pattern(pattern, length=length, first_element=first_element, last_element=last_element)

    def test_GroupingPatternHorizontalIN5(self):
        pattern = common.get_grouping_pattern(self._TEST_WS, vertical_step=1, horizontal_step=2)
        self.assertTrue(pattern is not None)
        length = 98304 // 2
        first_element = "0+256"
        last_element = "98047+98303"
        self._test_pattern(pattern, length=length, first_element=first_element, last_element=last_element)

    def test_GroupingPatternBothIN5(self):
        pattern = common.get_grouping_pattern(self._TEST_WS, vertical_step=4, horizontal_step=2)
        self.assertTrue(pattern is not None)
        length = 98304 // 8
        first_element = "0+1+2+3+256+257+258+259"
        last_element = "98044+98045+98046+98047+98300+98301+98302+98303"
        self._test_pattern(pattern, length=length, first_element=first_element, last_element=last_element)

    def test_GroupingPatternBothPANTHER(self):
        self._TEST_WS = self._create_instrument("PANTHER")
        pattern = common.get_grouping_pattern(self._TEST_WS, vertical_step=4, horizontal_step=2)
        self.assertTrue(pattern is not None)
        length = 73728 // 8
        first_element = "0+1+2+3+256+257+258+259"
        last_element = "73468+73469+73470+73471+73724+73725+73726+73727"
        self._test_pattern(pattern, length=length, first_element=first_element, last_element=last_element)

    def test_GroupingPatternBothSHARP(self):
        self._TEST_WS = self._create_instrument("SHARP")
        pattern = common.get_grouping_pattern(self._TEST_WS, vertical_step=4, horizontal_step=2)
        self.assertTrue(pattern is not None)
        length = 61440 // 8
        first_element = "0+1+2+3+256+257+258+259"
        last_element = "61180+61181+61182+61183+61436+61437+61438+61439"
        self._test_pattern(pattern, length=length, first_element=first_element, last_element=last_element)

    def _test_pattern(self, pattern, length, first_element, last_element):
        """Helper function to test each pattern in a consistent way.

        Keyword parameters:
        pattern -- string containing the grouping pattern
        length -- expected number of elements of the pattern
        first_element -- expected value for the first element of the pattern
        last_element -- expected value for the final element of the pattern
        """
        pattern_list = pattern.split(",")
        self.assertEqual(len(pattern_list), length)
        self.assertEqual(pattern_list[0], first_element)
        self.assertEqual(pattern_list[-1], last_element)


if __name__ == "__main__":
    unittest.main()
