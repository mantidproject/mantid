import unittest
from mantid.kernel import *
from mantid.api import *
import numpy as np

class MyPeak(IPeakFunction):

    def init(self):
        self.declareAttribute("Centre", 1)
        self.declareAttribute("Height", 3.4)
        self.declareAttribute("Width", 1.5)

    def functionLocal(self, xvals):
        return 5*xvals

class IPeakFunctionTest(unittest.TestCase):

    def test_instance_can_be_created_standalone(self):
        func = MyPeak()
        self.assertTrue(isinstance(func, IPeakFunction))
        self.assertTrue(isinstance(func, IFunction1D))

    def test_instance_can_be_created_from_factory(self):
        FunctionFactory.subscribe(MyPeak)
        func_name = MyPeak.__name__
        func = FunctionFactory.createFunction(func_name)
        self.assertTrue(isinstance(func, IPeakFunction))
        self.assertTrue(isinstance(func, IFunction1D))
        FunctionFactory.unsubscribe(func_name)

    def test_functionLocal_can_be_called_directly(self):
        func = MyPeak()
        func.initialize()
        xvals=np.array([1.,2.,3.])
        out = func.functionLocal(xvals)
        self.assertEquals(3, out.shape[0])
        self.assertEquals(5., out[0])
        self.assertEquals(10., out[1])
        self.assertEquals(15., out[2])

if __name__ == '__main__':
    unittest.main()
