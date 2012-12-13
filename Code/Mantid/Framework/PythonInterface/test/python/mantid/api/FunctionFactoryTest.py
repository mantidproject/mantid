import unittest

from mantid.api import FunctionFactory

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

if __name__ == '__main__':
    unittest.main()
