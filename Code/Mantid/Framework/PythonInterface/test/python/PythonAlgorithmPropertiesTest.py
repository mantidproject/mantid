import unittest

from mantid import PythonAlgorithm, Direction

class BasicPropsAlg(PythonAlgorithm):
    
    def PyInit(self):
        self.declareProperty('SimpleInput', 1)
        self.declareProperty('SimpleOutput', 1.0, Direction.Output)
        
    def PyExec(self):
        pass
    
# ======================================================================

class PythonAlgorithmPropertiesTest(unittest.TestCase):
  
   def test_simple_property_declarations_have_correct_attrs(self):
        alg = BasicPropsAlg() #AlgorithmManager.Instance().createUnmanaged("TestPyAlgDeclaringProps")
        props = alg.getProperties()
        self.assertEquals(0, len(props))
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(2, len(props))
        
        input = alg.getProperty("SimpleInput")
        self.assertEquals(input.direction, Direction.Input)
        output = alg.getProperty("SimpleOutput")
        self.assertEquals(output.direction, Direction.Output)

