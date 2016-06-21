import unittest
from testhelpers import run_algorithm
from mantid.geometry import Goniometer
from mantid.kernel import DateAndTime

class RunTest(unittest.TestCase):

    _expt_ws = None
    _nspec=1

    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm('CreateWorkspace', DataX=[1,2,3,4,5], DataY=[1,2,3,4,5],NSpec=self._nspec, child=True)
            ws = alg.getProperty("OutputWorkspace").value
            ws.run().addProperty("gd_prtn_chrg", 10.05, True)
            ws.run().addProperty("nspectra", self._nspec, True)
            ws.run().setStartAndEndTime(DateAndTime("2008-12-18T17:58:38"), DateAndTime("2008-12-18T17:59:40"))
            self.__class__._expt_ws = ws

    def test_get_goniometer(self):
        run = self._expt_ws.run()
        gm = run.getGoniometer()
        self.assertTrue(isinstance(gm, Goniometer))

    def test_proton_charge_returns_a_double(self):
        run = self._expt_ws.run()
        charge = run.getProtonCharge()
        self.assertEquals(type(charge), float)
        self.assertAlmostEquals(charge, 10.05, 2)

    def test_run_hasProperty(self):
        self.assertTrue(self._expt_ws.run().hasProperty('start_time'))
        self.assertTrue('start_time' in self._expt_ws.run())
        self.assertFalse('not a log' in self._expt_ws.run())

    def test_run_getProperty(self):
        run_start = self._expt_ws.run().getProperty('start_time')
        self.assertEquals(type(run_start.value), str)
        self.assertEquals(run_start.value, "2008-12-18T17:58:38")

        def do_spectra_check(nspectra):
            self.assertEquals(type(nspectra.value), int)
            self.assertEquals(nspectra.value, self._nspec)
            self.assertRaises(RuntimeError, self._expt_ws.run().getProperty, 'not_a_log')

        do_spectra_check(self._expt_ws.run().getProperty('nspectra'))
        do_spectra_check(self._expt_ws.run()['nspectra'])
        do_spectra_check(self._expt_ws.run().get('nspectra'))

        # get returns the default if key does not exist, or None if no default
        self.assertEquals(self._expt_ws.run().get('not_a_log'), None)
        self.assertEquals(self._expt_ws.run().get('not_a_log', 5.), 5.)

    def test_add_property_with_known_type_succeeds(self):
        run = self._expt_ws.run()
        nprops = len(run.getProperties())
        run.addProperty('int_t', 1, False)
        self.assertEquals(len(run.getProperties()), nprops + 1)
        run.addProperty('float_t', 2.4, False)
        self.assertEquals(len(run.getProperties()), nprops + 2)
        run.addProperty('str_t', 'from_python', False)
        self.assertEquals(len(run.getProperties()), nprops + 3)
        run['int_t'] = 6.5
        self.assertEquals(len(run.getProperties()), nprops + 3)
        self.assertEquals(run.getProperty('int_t').value, 6.5)

    def test_add_propgates_units_correctly(self):
        run = self._expt_ws.run()
        run.addProperty('float_t', 2.4, 'metres', True)
        prop = run.getProperty('float_t')
        self.assertEquals(prop.units, 'metres')

    def test_add_property_with_unknown_type_raises_error(self):
        run = self._expt_ws.run()
        self.assertRaises(ValueError, run.addProperty, 'dict_t', {}, False)

    def test_keys_returns_a_list_of_the_property_names(self):
        run = self._expt_ws.run()
        names = run.keys()
        self.assertEqual(type(names), list)

        # Test a few
        self.assertTrue('nspectra' in names)
        self.assertTrue('start_time' in names)
        self.assertFalse('not a log' in names)

    def test_startime(self):
        """ Test exported function startTime()
        """
        run = self._expt_ws.run()

        runstart = run.startTime()
        runstartstr = str(runstart)
        self.assertEquals(runstartstr, "2008-12-18T17:58:38 ") # The space at the end is to get around an IPython bug (#8351)
        self.assertTrue(isinstance(runstart, DateAndTime))

    def test_endtime(self):
        """ Test exported function endTime()
        """
        run = self._expt_ws.run()

        runend = run.endTime()
        runendstr = str(runend)
        self.assertEquals(runendstr, "2008-12-18T17:59:40 ") # The space at the end is to get around an IPython bug (#8351)
        self.assertTrue(isinstance(runend, DateAndTime))

if __name__ == '__main__':
    unittest.main()
