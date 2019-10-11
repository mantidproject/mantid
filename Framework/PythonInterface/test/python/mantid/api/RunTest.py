# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import copy
from mantid.geometry import Goniometer
from mantid.kernel import DateAndTime
from mantid.api import Run

class RunTest(unittest.TestCase):

    _expt_ws = None
    _nspec = 1

    def setUp(self):
        if self.__class__._expt_ws is None:
            run = Run()
            run.addProperty("gd_prtn_chrg", 10.05, True)
            run.addProperty("nspectra", self._nspec, True)
            run.setStartAndEndTime(DateAndTime("2008-12-18T17:58:38"),
                                        DateAndTime("2008-12-18T17:59:40"))
            self.__class__._run = run

    def test_get_goniometer(self):
        run = Run()
        gm = run.getGoniometer()
        self.assertTrue(isinstance(gm, Goniometer))

    def test_proton_charge_returns_a_double(self):
        run = self._run
        charge = run.getProtonCharge()
        self.assertEqual(type(charge), float)
        self.assertAlmostEquals(charge, 10.05, 2)

    def test_run_hasProperty(self):
        self.assertTrue(self._run.hasProperty('start_time'))
        self.assertTrue('start_time' in self._run)
        self.assertFalse('not a log' in self._run)

    def test_run_getProperty(self):
        run_start = self._run.getProperty('start_time')
        self.assertEqual(type(run_start.value), str)
        self.assertEqual(run_start.value, "2008-12-18T17:58:38")

        def do_spectra_check(nspectra):
            self.assertEqual(type(nspectra.value), int)
            self.assertEqual(nspectra.value, self._nspec)
            self.assertRaises(RuntimeError,
                              self._run.getProperty, 'not_a_log')

        do_spectra_check(self._run.getProperty('nspectra'))
        do_spectra_check(self._run['nspectra'])
        do_spectra_check(self._run.get('nspectra'))

        # get returns the default if key does not exist, or None if no default
        self.assertEqual(self._run.get('not_a_log'), None)
        self.assertEqual(self._run.get('not_a_log', 5.), 5.)

    def test_run_getPropertyAsSingleValue_with_number(self):
        charge = self._run.getPropertyAsSingleValue('gd_prtn_chrg')
        self.assertAlmostEqual(10.05, charge)

    def test_add_property_with_known_type_succeeds(self):
        run = self._run
        nprops = len(run.getProperties())
        run.addProperty('int_t', 1, False)
        self.assertEqual(len(run.getProperties()), nprops + 1)
        run.addProperty('float_t', 2.4, False)
        self.assertEqual(len(run.getProperties()), nprops + 2)
        run.addProperty('str_t', 'from_python', False)
        self.assertEqual(len(run.getProperties()), nprops + 3)
        run['int_t'] = 6.5
        self.assertEqual(len(run.getProperties()), nprops + 3)
        self.assertEqual(run.getProperty('int_t').value, 6.5)

    def test_add_propgates_units_correctly(self):
        run = self._run
        run.addProperty('float_t', 2.4, 'metres', True)
        prop = run.getProperty('float_t')
        self.assertEqual(prop.units, 'metres')

    def test_add_property_with_unknown_type_raises_error(self):
        run = self._run
        # This used to test for dict, but we allow for dict now.
        self.assertRaises(ValueError, run.addProperty, 'set_t', set(), False)

    def test_keys_returns_a_list_of_the_property_names(self):
        run = self._run
        names = run.keys()
        self.assertEqual(type(names), list)

        # Test a few
        self.assertTrue('nspectra' in names)
        self.assertTrue('start_time' in names)
        self.assertFalse('not a log' in names)

    def test_startime(self):
        """ Test exported function startTime()
        """
        run = self._run

        runstart = run.startTime()
        runstartstr = str(runstart)
        self.assertEqual(
            runstartstr, "2008-12-18T17:58:38 "
        )  # The space at the end is to get around an IPython bug (#8351)
        self.assertTrue(isinstance(runstart, DateAndTime))

    def test_endtime(self):
        """ Test exported function endTime()
        """
        run = self._run

        runend = run.endTime()
        runendstr = str(runend)
        self.assertEqual(
            runendstr, "2008-12-18T17:59:40 "
        )  # The space at the end is to get around an IPython bug (#8351)
        self.assertTrue(isinstance(runend, DateAndTime))

    def do_test_copyable(self, copy_op):
        original = self._run
        # make copy
        cp = copy_op(original)
        # Check identity different
        self.assertNotEqual(id(original), id(cp))
        # Simple tests that cp is equal to original
        self.assertEqual(original.startTime(), cp.startTime())
        self.assertEqual(original.endTime(), cp.endTime())

    def test_shallow_copyable(self):
        self.do_test_copyable(copy.copy)

    def test_deep_copyable(self):
        self.do_test_copyable(copy.deepcopy)


if __name__ == '__main__':
    unittest.main()
