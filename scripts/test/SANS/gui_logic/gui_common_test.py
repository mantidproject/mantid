# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

import mantid

from sans.gui_logic.gui_common import (get_reduction_mode_strings_for_gui, get_reduction_selection,
                                       get_string_for_gui_from_reduction_mode)
from sans.common.enums import (SANSInstrument, ISISReductionMode)


class GuiCommonTest(unittest.TestCase):
    def _assert_same(self, collection1, collection2):
        for element1, element2 in zip(collection1, collection2):
            self.assertTrue(element1 == element2)

    def _assert_same_map(self, map1, map2):
        self.assertTrue(len(map1) == len(map2))

        for key, value in map1.items():
            self.assertTrue(key in map2)
            self.assertTrue(map1[key] == map2[key])

    def do_test_reduction_mode_string(self, instrument, reduction_mode, reduction_mode_string):
        setting = get_string_for_gui_from_reduction_mode(reduction_mode, instrument)
        self.assertTrue(setting == reduction_mode_string)

    def test_that_gets_reduction_mode_string_for_gui(self):
        sans_settings = get_reduction_mode_strings_for_gui(SANSInstrument.SANS2D)
        self._assert_same(sans_settings, ["rear", "front", "Merged", "All"])

        loq_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LOQ)
        self._assert_same(loq_settings, ["main-detector", "Hab", "Merged", "All"])

        larmor_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LARMOR)
        self._assert_same(larmor_settings, ["DetectorBench"])

        default_settings = get_reduction_mode_strings_for_gui(SANSInstrument.NoInstrument)
        self._assert_same(default_settings, ["LAB", "HAB", "Merged", "All"])

    def test_that_gets_correct_reduction_selection(self):
        sans_settings = get_reduction_selection(SANSInstrument.SANS2D)
        self._assert_same_map(sans_settings, {ISISReductionMode.LAB: "rear", ISISReductionMode.HAB: "front",
                                              ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

        loq_settings = get_reduction_selection(SANSInstrument.LOQ)
        self._assert_same_map(loq_settings, {ISISReductionMode.LAB: "main-detector", ISISReductionMode.HAB: "Hab",
                                             ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

        larmor_settings = get_reduction_selection(SANSInstrument.LARMOR)
        self._assert_same_map(larmor_settings, {ISISReductionMode.LAB: "DetectorBench"})

        default_settings = get_reduction_selection(SANSInstrument.NoInstrument)
        self._assert_same_map(default_settings, {ISISReductionMode.LAB: "LAB", ISISReductionMode.HAB: "HAB",
                                                 ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

    def test_that_can_get_reduction_mode_string(self):
        self.do_test_reduction_mode_string(SANSInstrument.SANS2D, ISISReductionMode.LAB, "rear")
        self.do_test_reduction_mode_string(SANSInstrument.LOQ, ISISReductionMode.HAB, "Hab")
        self.do_test_reduction_mode_string(SANSInstrument.LARMOR, ISISReductionMode.LAB, "DetectorBench")
        self.do_test_reduction_mode_string(SANSInstrument.NoInstrument, ISISReductionMode.LAB, "LAB")
        self.do_test_reduction_mode_string(SANSInstrument.NoInstrument, ISISReductionMode.HAB, "HAB")


if __name__ == '__main__':
    unittest.main()


