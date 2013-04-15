import unittest
from mantid.api import IFunction1D, IFunction, FunctionFactory

class PyLinear(IFunction1D):
    
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
        func.declareAttribute("IntAtt", 1)
        func.declareAttribute("DoubleAtt", 3.4)
        func.declareAttribute("StringAtt", "filename")
        func.declareAttribute("BoolAtt", True)

        self.assertRaises(ValueError, func.declareAttribute, "ListAtt", [1,2,3])


if __name__ == '__main__':
    unittest.main()
