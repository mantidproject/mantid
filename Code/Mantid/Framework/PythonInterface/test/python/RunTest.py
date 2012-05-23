import unittest
from testhelpers import run_algorithm

class RunTest(unittest.TestCase):

    _expt_ws = None
  
    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm('Load', Filename='LOQ48127.raw', SpectrumMax=1, child=True)
            self.__class__._expt_ws = alg.getProperty("OutputWorkspace").value

    def test_proton_charge_returns_a_double(self):
        run = self._expt_ws.run()
        charge = run.getProtonCharge()
        self.assertEquals(type(charge), float)
        self.assertAlmostEquals(charge, 10.040912628173828, 15)
        
    def test_run_hasProperty(self):
        self.assertTrue(self._expt_ws.run().hasProperty('run_start'))
        self.assertTrue('run_start' in self._expt_ws.run())
        self.assertFalse('not a log' in self._expt_ws.run())

    def test_run_getProperty(self):
        run_start = self._expt_ws.run().getProperty('run_start')
        self.assertEquals(type(run_start.value), str)
        self.assertEquals(run_start.value, "2008-12-18T17:58:38")
        
        def do_spectra_check(nspectra):
            self.assertEquals(type(nspectra.value), int)
            self.assertEquals(nspectra.value, 8)
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
        self.assertEquals(len(names), 31)
        # Test a few
        self.assertTrue('nspectra' in names)
        self.assertTrue('run_start' in names)
        self.assertFalse('not a log' in names)
    
if __name__ == '__main__':
    unittest.main()