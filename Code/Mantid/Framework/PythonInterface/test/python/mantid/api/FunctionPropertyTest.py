from mantid.api import FunctionProperty,PythonAlgorithm, IFunction
from testhelpers import assertRaisesNothing
import unittest
import math

class FunctionPropertyTest(unittest.TestCase):

    #------------------------------------------------------------
    class TestFunctionPropAlg(PythonAlgorithm):
        def PyInit(self):
            self.declareProperty(FunctionProperty("fun"))
        def PyExec(self):
            fp = self.getProperty("fun")
            if not isinstance(fp, FunctionProperty):
                raise RuntimeError("Expected a FunctionProperty but found %s " % str(type(fp)))
            func = fp.value
            if not isinstance(func, IFunction):
                raise RuntimeError("Expected an IFunction but found %s " % str(type(func)))

            height=func.getParamValue(0)
            if math.fabs(height - 1.0) > 1e-12:
                raise RuntimeError("Height does not have the expected value")
    #------------------------------------------------------------

#---- Success cases ----
    def test_constructor_succeeds_with_non_empty_string_name(self):
        assertRaisesNothing(self, FunctionProperty, "Function")

    def test_type_string_returns_Function(self):
        func = FunctionProperty("fun")
        self.assertEqual("Function", func.type)

    def test_value_is_empty_string_for_default_property(self):
        func = FunctionProperty("name")
        self.assertEquals(None, func.value)
        self.assertEquals("", func.valueAsStr)

    def test_valid_string_value_gives_function_object_as_value(self):
        alg=self.TestFunctionPropAlg()
        alg.initialize()
        alg.setProperty("fun", "name=Gaussian,PeakCentre=5.0,Height=1.0")
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)

#---- Error cases ----
    def test_invalid_string_value_gives_function_object_as_value(self):
        alg=self.TestFunctionPropAlg()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "fun", "blah")

if __name__ == '__main__':
    unittest.main()
