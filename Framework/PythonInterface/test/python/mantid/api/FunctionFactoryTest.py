# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.api import FrameworkManagerImpl, IFunction1D, FunctionFactory


class TestFunctionNoAttrs(IFunction1D):
    pass


class TestFunctionOnlyInit(IFunction1D):

    def init(self):
       pass


class TestFunctionOnlyFunction1D(IFunction1D):

    def function1D(self, xvals):
        pass


class TestFunctionCorrectForm(IFunction1D):

    def init(self):
        pass

    def function1D(self, xvals):
        pass


class FunctionFactoryTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManagerImpl.Instance()

    def test_get_function_factory_does_not_return_None(self):
        self.assertNotEqual(FunctionFactory, None)

    def test_get_functions(self):
        all_funcs = FunctionFactory.getFunctionNames()
        self.assertTrue( len(all_funcs) > 0 )
        self.assertTrue("Gaussian" in all_funcs)

    def test_get_Gaussian(self):
        name = "Gaussian"
        func = FunctionFactory.createFunction(name)
        self.assertEqual(func.name(),  name)
        self.assertTrue(len(func.__repr__()) > len(name))
        self.assertTrue("Peak" in func.categories())

    def test_function_subscription_of_non_class_type_raises_error(self):
        def not_a_fit_function(*args, **kwargs):
            pass
        self.assertRaises(ValueError, FunctionFactory.subscribe, not_a_fit_function)

    def test_function_subscription_of_class_without_IFunction_base_raises_error(self):
        class NotAFitFunction(object):
            pass
        self.assertRaises(ValueError, FunctionFactory.subscribe, NotAFitFunction)

    def test_function_subscription_without_required_attrs_fails(self):
        self.assertRaises(RuntimeError, FunctionFactory.Instance().subscribe, TestFunctionNoAttrs)
        self.assertTrue("TestFunctionNoAttrs" not in FunctionFactory.getFunctionNames())
        self.assertRaises(RuntimeError, FunctionFactory.Instance().subscribe, TestFunctionOnlyInit)
        self.assertTrue("TestFunctionOnlyInit" not in FunctionFactory.getFunctionNames())

    def test_function_with_expected_attrs_subscribes_successfully(self):
        nfuncs_orig = len(FunctionFactory.getFunctionNames())
        FunctionFactory.subscribe(TestFunctionCorrectForm)
        new_funcs = FunctionFactory.getFunctionNames()
        self.assertEquals(nfuncs_orig+1, len(new_funcs))
        self.assertTrue("TestFunctionCorrectForm" in new_funcs)

    def test_function_existing_function_can_be_unsubscribed(self):
        FunctionFactory.subscribe(TestFunctionCorrectForm)
        nfuncs_before = len(FunctionFactory.getFunctionNames())
        FunctionFactory.unsubscribe("TestFunctionCorrectForm")
        available_functions = FunctionFactory.getFunctionNames()
        self.assertEquals(nfuncs_before - 1, len(available_functions))
        self.assertTrue("TestFunctionCorrectForm" not in available_functions)


if __name__ == '__main__':
    unittest.main()
