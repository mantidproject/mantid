import unittest

from mantid.api import IFunction1D, FunctionFactory

class TestFunction(IFunction1D):

    def init(self):
        pass

class FunctionFactoryTest(unittest.TestCase):

    def test_get_function_factory_does_not_return_None(self):
        self.assertTrue(FunctionFactory is not None)

    def test_get_functions(self):
        all_funcs = FunctionFactory.getFunctionNames()
        self.assertTrue( len(all_funcs) > 0 )
        self.assertTrue("Gaussian" in all_funcs)

    def test_get_Gaussian(self):
        name = "Gaussian"
        func = FunctionFactory.createFunction(name)
        self.assertTrue(func.name() == name)
        self.assertTrue(len(func.__repr__()) > len(name))
        self.assertTrue("Peak" in func.categories())


    def test_function_subscription(self):
        nfuncs_orig = len(FunctionFactory.getFunctionNames())
        FunctionFactory.subscribe(TestFunction)
        new_funcs = FunctionFactory.getFunctionNames()
        self.assertEquals(nfuncs_orig+1, len(new_funcs))
        self.assertTrue("TestFunction" in new_funcs)

        FunctionFactory.unsubscribe("TestFunction")
        new_funcs = FunctionFactory.getFunctionNames()
        self.assertEquals(nfuncs_orig, len(new_funcs))
        self.assertTrue("TestFunction" not in new_funcs)


if __name__ == '__main__':
    unittest.main()
