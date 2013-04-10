import unittest
from mantid.api import IFunction1D, IFunction, FunctionFactory

class PyLinear(IFunction1D):
    
    def function1D(self, out, xvals):
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


if __name__ == '__main__':
    unittest.main()
