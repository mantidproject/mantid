"""Defines tests for the simple property declarations types within 
Python algorithms
"""

import unittest
import testhelpers

from mantid import PythonAlgorithm, Direction
from mantid import BoundedValidator, FileProperty, FileAction

    
class BasicPropsAlg(PythonAlgorithm):
    
    def PyInit(self):
        self.declareProperty('SimpleInput', 1)
        self.declareProperty('SimpleOutput', 1.0, Direction.Output)
        self.declareProperty('InputString', "", Direction.Input)
        
    def PyExec(self):
        pass
    
# ======================================================================

class PythonAlgorithmPropertiesTest(unittest.TestCase):
  
    def test_simple_property_declarations_have_correct_attrs(self):
        """
        Test the basic property declarations without validators
        """
        class BasicPropsAlg(PythonAlgorithm):
            
            _testdocstring = "This is a doc string"
            def PyInit(self):
                self.declareProperty('SimpleInput', 1)
                self.declareProperty('SimpleOutput', 1.0, Direction.Output)
                self.declareProperty('InputString', "", Direction.Input)
                self.declareProperty('PropWithDocDefaultDir', 1, self._testdocstring)
                self.declareProperty('PropWithDocOutputDir', 1.0, self._testdocstring, Direction.Output)

                
            def PyExec(self):
                pass
        ##########################################################################
        alg = BasicPropsAlg()
        props = alg.getProperties()
        self.assertEquals(0, len(props))
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(5, len(props))
        
        input = alg.getProperty("SimpleInput")
        self.assertEquals(input.direction, Direction.Input)
        self.assertEquals(input.value, 1)
        output = alg.getProperty("SimpleOutput")
        self.assertEquals(output.direction, Direction.Output)
        self.assertEquals(output.value, 1.0)
        str_prop = alg.getProperty("InputString")
        self.assertEquals(str_prop.direction, Direction.Input)
        self.assertEquals(str_prop.value, "")
        
        doc_prop_def_dir = alg.getProperty("PropWithDocDefaultDir")
        self.assertEquals(doc_prop_def_dir.direction, Direction.Input)
        self.assertEquals(doc_prop_def_dir.documentation, alg._testdocstring)
        doc_prop_out_dir = alg.getProperty("PropWithDocOutputDir")
        self.assertEquals(doc_prop_out_dir.direction, Direction.Output)
        self.assertEquals(doc_prop_out_dir.documentation, alg._testdocstring)
        
    def test_properties_obey_attached_validators(self):
        """
            Test property declarations with validator.
            The validators each have their own test.
        """
        class PropertiesWithValidation(PythonAlgorithm):
            
            def PyInit(self):
                self.declareProperty('NumPropWithDefaultDir', -1, BoundedValidator(lower=0))
                self.declareProperty('NumPropWithInOutDir', -1, BoundedValidator(lower=0),"doc string", Direction.InOut)
            
            def PyExec(self):
                pass
        ###################################################
        alg = PropertiesWithValidation()
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(2, len(props))
        
        def_dir = alg.getProperty("NumPropWithDefaultDir")
        self.assertEquals(def_dir.direction, Direction.Input)
        self.assertNotEquals("", def_dir.isValid())
        self.assertRaises(ValueError, alg.setProperty, "NumPropWithDefaultDir", -10)
        testhelpers.assertRaisesNothing(self, alg.setProperty, "NumPropWithDefaultDir", 11)
        
    def test_specialized_property_declaration(self):
        """
            Test property declaration using a specialised property.
            The property types should have their own tests too.
        """
        class SpecializedProperties(PythonAlgorithm):
            
            _testdocstring = 'This is a FileProperty'
            def PyInit(self):
                self.declareProperty(FileProperty("NoDocString", "", FileAction.Load))
                self.declareProperty(FileProperty("WithDocString", "", FileAction.Load), self._testdocstring)
            
            def PyExec(self):
                pass
        ####################################################
        alg = SpecializedProperties()
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(2, len(props))
        
        nodoc = alg.getProperty("NoDocString")
        self.assertTrue(isinstance(nodoc, FileProperty))
        self.assertEquals("", nodoc.documentation)
        withdoc = alg.getProperty("WithDocString")
        self.assertTrue(isinstance(withdoc, FileProperty))
        self.assertEquals(alg._testdocstring, withdoc.documentation)
        
if __name__ == '__main__':
    unittest.main()
