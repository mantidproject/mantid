import unittest
from mantid.api import IFunction1D, IFunction, FunctionFactory
import numpy as np

class NoCatgeoryFunction(IFunction1D):

    def init(self):
        pass

class Times2(IFunction1D):

    def category(self):
        return "SimpleFunction"

    def init(self):
        self.declareAttribute("IntAtt", 1)
        self.declareAttribute("DoubleAtt", 3.4)
        self.declareAttribute("StringAtt", "filename")
        self.declareAttribute("BoolAtt", True)

        self.declareParameter("ParamZeroInitNoDescr")
        self.declareParameter("ParamNoDescr", 1.5)
        self.declareParameter("OtherParam",4,"Some fitting parameter")

    def function1D(self, xvals):
        return 2*xvals

class IFunction1DTest(unittest.TestCase):

    def test_instance_can_be_created_standalone(self):
        func = Times2()
        self.assertTrue(isinstance(func, IFunction1D))

    def test_instance_can_be_created_from_factory(self):
        FunctionFactory.subscribe(Times2)
        func_name = Times2.__name__
        func = FunctionFactory.createFunction(func_name)
        self.assertTrue(isinstance(func, IFunction1D))
        FunctionFactory.unsubscribe(func_name)

    def test_category_with_no_override_returns_default_category(self):
        FunctionFactory.subscribe(NoCatgeoryFunction)
        func = FunctionFactory.createFunction("NoCatgeoryFunction")
        self.assertEquals("General", func.category())
        FunctionFactory.unsubscribe("NoCatgeoryFunction")

    def test_category_override_returns_overridden_result(self):
        FunctionFactory.subscribe(Times2)
        func = FunctionFactory.createFunction("Times2")
        self.assertEquals("SimpleFunction", func.category())
        FunctionFactory.unsubscribe("Times2")

    def test_declareAttribute_only_accepts_known_types(self):
        func = Times2()
        func.initialize() # Contains known types
        self.assertEquals(4, func.nAttributes()) # Make sure initialize ran
        self.assertRaises(ValueError, func.declareAttribute, "ListAtt", [1,2,3])

    def test_correct_attribute_values_are_returned_when_asked(self):
        func = Times2()
        func.initialize() # Contains known types

        self.assertEquals(1, func.getAttributeValue("IntAtt"))
        self.assertEquals(3.4, func.getAttributeValue("DoubleAtt"))
        self.assertEquals("filename", func.getAttributeValue("StringAtt"))
        self.assertEquals(True, func.getAttributeValue("BoolAtt"))

    def test_correct_parameters_are_attached_during_init(self):
        func = Times2()
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

    def test_function1D_can_be_called_directly(self):
        func = Times2()
        func.initialize()
        xvals=np.array([1,2,3])
        out = func.function1D(xvals)
        self.assertEquals(3, out.shape[0])
        self.assertEquals(2, out[0])
        self.assertEquals(4, out[1])
        self.assertEquals(6, out[2])

if __name__ == '__main__':
    unittest.main()
