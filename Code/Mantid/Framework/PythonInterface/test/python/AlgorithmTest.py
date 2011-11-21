import unittest

from mantid.api import algorithm_mgr

class AlgorithmTest(unittest.TestCase):
  
    _load = None

    def setUp(self):
        if self._load is None:
            self.__class__._load = algorithm_mgr.create_unmanaged('Load')
            self._load.initialize()
  
    def test_alg_attrs_are_correct(self):
        self.assertTrue(self._load.name(), 'Load')
        self.assertTrue(self._load.version(), 1)
        self.assertTrue(self._load.category(), 'DataHandling')
    
    # Move this to the simple api test    
    def xtest_alg_has_doc_string(self):
        # Test auto generated string, Load is manually written
        rebin = algorithm_mgr.create_unmanaged('Rebin')
        doc = rebin.__doc__
        self.assertTrue(len(doc) > 0 )
        self.assertTrue('InputWorkspace' in doc)
        self.assertTrue('OutputWorkspace' in doc)
        self.assertTrue('Params' in doc)
        self.assertTrue('PreserveEvents' in doc)
        
    def test_alg_set_valid_prop_succeeds(self):
        self._load.set_property('Filename', 'LOQ48127.raw')
        
    def test_alg_set_invalid_prop_raises_error(self):
        alg = algorithm_mgr.create_unmanaged('Load')
        alg.initialize()
        args = ('Filename', 'nonexistent.txt')
        self.assertRaises(ValueError, alg.set_property, *args)
        
    def test_cannot_execute_with_invalid_properties(self):
        alg = algorithm_mgr.create_unmanaged('Load')
        alg.initialize()
        self.assertRaises(RuntimeError, alg.execute)
        
    def test_execute_succeeds_with_valid_props(self):
        alg = algorithm_mgr.create_unmanaged('Load')
        alg.initialize()
        alg.set_property('Filename', 'LOQ48127.raw')
        nspec = 1
        alg.set_property('SpectrumMax', nspec)
        wsname = 'LOQ48127' 
        alg.set_property('OutputWorkspace', wsname)
        alg.set_child(True) # Just to keep the output from the data service
        alg.execute()
        self.assertEquals(alg.get_property('SpectrumMax').value, nspec)
        self.assertEquals(type(alg.get_property('SpectrumMax').value), int)
        self.assertEquals(alg.get_property('SpectrumMax').name, 'SpectrumMax')
                
        ws = alg.get_property('OutputWorkspace').value
        self.assertTrue(ws.get_memory_size() > 0.0 )
