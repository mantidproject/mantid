import unittest
from mantid.api import AlgorithmManager
from testhelpers import run_algorithm

class AlgorithmTest(unittest.TestCase):
  
    _load = None

    def setUp(self):
        if self._load is None:
            self.__class__._load = AlgorithmManager.createUnmanaged('Load')
            self._load.initialize()
  
    def test_alg_attrs_are_correct(self):
        self.assertTrue(self._load.name(), 'Load')
        self.assertTrue(self._load.version(), 1)
        self.assertTrue(self._load.category(), 'DataHandling')

    def test_get_unknown_property_raises_error(self):
        self.assertRaises(RuntimeError, self._load.getProperty, "NotAProperty")
        
    def test_alg_set_valid_prop_succeeds(self):
        self._load.setProperty('Filename', 'LOQ48127.raw')
        
    def test_alg_set_invalid_prop_raises_error(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        alg.initialize()
        args = ('Filename', 'nonexistent.txt')
        self.assertRaises(ValueError, alg.setProperty, *args)
        
    def test_cannot_execute_with_invalid_properties(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        alg.initialize()
        self.assertRaises(RuntimeError, alg.execute)
        
    def test_execute_succeeds_with_valid_props(self):
        data = [1.0,2.0,3.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength',child=True)
        self.assertEquals(alg.isExecuted(), True)
        self.assertEquals(alg.getProperty('NSpec').value, 1)
        self.assertEquals(type(alg.getProperty('NSpec').value), int)
        self.assertEquals(alg.getProperty('NSpec').name, 'NSpec')
        ws = alg.getProperty('OutputWorkspace').value
        self.assertTrue(ws.getMemorySize() > 0.0 )
        
        as_str = str(alg)
        self.assertEquals(as_str, "CreateWorkspace.1(OutputWorkspace=UNUSED_NAME_FOR_CHILD,DataX=1,2,3,DataY=1,2,3,UnitX=Wavelength)")
        
    def test_createSubAlgorithm_creates_new_algorithm_that_is_set_as_child(self):
        parent_alg = AlgorithmManager.createUnmanaged('Load')
        child_alg = parent_alg.createSubAlgorithm('Rebin')
        
        self.assertTrue(child_alg.isChild())

    def test_createSubAlgorithm_respects_keyword_arguments(self):
        parent_alg = AlgorithmManager.createUnmanaged('Load')
        try:
            child_alg = parent_alg.createSubAlgorithm(name='Rebin',version=1,startProgress=0.5,endProgress=0.9,enableLogging=True)
        except Exception,exc:
            self.fail("Expected createSubAlgorithm not to throw but it did: %s" % (str(exc)))
            
        # Unknown keyword
        self.assertRaises(Exception, parent_alg.createSubAlgorithm, name='Rebin',version=1,startProgress=0.5,endProgress=0.9,enableLogging=True, unknownKW=1)
        
if __name__ == '__main__':
    unittest.main()
    
