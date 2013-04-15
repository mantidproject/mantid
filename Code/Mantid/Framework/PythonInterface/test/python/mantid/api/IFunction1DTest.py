import unittest
from mantid.api import IFunction1D, IFunction, FunctionFactory

class PyLinear(IFunction1D):
    
    def init(self):
        self.declareAttribute("IntAtt", 1)
        self.declareAttribute("DoubleAtt", 3.4)
        self.declareAttribute("StringAtt", "filename")
        self.declareAttribute("BoolAtt", True)
        
        self.declareParameter("ParamZeroInitNoDescr")
        self.declareParameter("ParamNoDescr", 1.5)
        self.declareParameter("OtherParam",4,"Some fitting parameter")
        
    def function1D(self, xvals, out):
        pass

class IFunction1DTest(unittest.TestCase):

    def test_instance_can_be_created_standalone(self):
        func = PyLinear()
        self.assertTrue(isinstance(func, IFunction1D))

    def test_instance_can_be_created_from_factory(self):
        FunctionFactory.subscribe(PyLinear)
        func_name = PyLinear.__name__
        func = FunctionFactory.createFunction(func_name)
        self.assertTrue(isinstance(func, IFunction1D))
        FunctionFactory.unsubscribe(func_name)

    def test_declareAttribute_only_accepts_known_types(self):
        func = PyLinear()
        func.initialize() # Contains known types
        self.assertEquals(4, func.nAttributes()) # Make sure initialize ran 
        self.assertRaises(ValueError, func.declareAttribute, "ListAtt", [1,2,3])

    def test_correct_attribute_values_are_returned_when_asked(self):
        func = PyLinear()
        func.initialize() # Contains known types
        
        self.assertEquals(1, func.getAttributeValue("IntAtt"))
        self.assertEquals(3.4, func.getAttributeValue("DoubleAtt"))
        self.assertEquals("filename", func.getAttributeValue("StringAtt"))
        self.assertEquals(True, func.getAttributeValue("BoolAtt"))

    def test_correct_parameters_are_attached_during_init(self):
        func = PyLinear()
        func.initialize()
        
        self.assertEquals(3, func.nParams())

        self.assertEquals("ParamZeroInitNoDescr",func.parameterName(0))
        self.assertEquals("",func.paramDescription(0))
        self.assertEquals(0.0,func.getParameterValue(0))

        self.assertEquals("ParamNoDescr",func.parameterName(1))
        self.assertEquals("",func.paramDescription(1))
        self.assertEquals(1.5,func.getParameterValue(1))

        self.assertEquals("OtherParam",func.parameterName(2))
        self.assertEquals("Some fitting parameter",func.paramDescription(2))
        self.assertEquals(4.0,func.getParameterValue(2))

if __name__ == '__main__':
    unittest.main()
