# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import FrameworkManagerImpl, FunctionFactory, CompositeFunction


class CompositeFunctionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManagerImpl.Instance()

    def test_instance_can_be_created_standalone(self):
        func = CompositeFunction()
        self.assertTrue(isinstance(func, CompositeFunction))

    def test_instance_can_be_created_from_factory(self):
        func = FunctionFactory.createInitialized("name=FlatBackground;name=FlatBackground")
        self.assertTrue(isinstance(func, CompositeFunction))
        self.assertEqual(len(func), 2)
        self.assertEqual(func.nParams(), 2)

    def test_members_can_be_added(self):
        func = CompositeFunction()
        f1 = FunctionFactory.createInitialized("name=FlatBackground,A0=1")
        f2 = FunctionFactory.createInitialized("name=FlatBackground,A0=2")
        f3 = FunctionFactory.createInitialized("name=FlatBackground,A0=3")
        func.add(f1)
        func.add(f2)
        func.add(f3)
        self.assertEqual(len(func), 3)
        self.assertEqual(str(func[0]), "name=FlatBackground,A0=1")
        self.assertEqual(str(func[1]), "name=FlatBackground,A0=2")
        self.assertEqual(str(func[2]), "name=FlatBackground,A0=3")

    def test_parameters_can_be_get_and_set(self):
        func = FunctionFactory.createInitialized("name=FlatBackground,A0=1;name=FlatBackground,A0=2")
        self.assertEqual(func.getParameterValue("f0.A0"), 1.0)
        self.assertEqual(func.getParameterValue("f1.A0"), 2.0)
        func.setParameter("f0.A0", 10.0)
        self.assertEqual(func.getParameterValue("f0.A0"), 10.0)
        func.setParameter("f1.A0", 20.0)
        self.assertEqual(func.getParameterValue("f1.A0"), 20.0)

    def test_parameters_can_be_set_via_members(self):
        func = FunctionFactory.createInitialized("name=FlatBackground,A0=1;name=FlatBackground,A0=2")
        func[0].setParameter("A0", 10.0)
        func[1].setParameter("A0", 20.0)
        self.assertEqual(func.getParameterValue("f0.A0"), 10.0)
        self.assertEqual(func.getParameterValue("f1.A0"), 20.0)

    def test_nested_functions(self):
        s = "name=FlatBackground,A0=1;(name=FlatBackground,A0=2;name=FlatBackground,A0=3)"
        func = FunctionFactory.createInitialized(s)
        self.assertEqual(len(func), 2)
        self.assertFalse(isinstance(func[0], CompositeFunction))
        self.assertTrue(isinstance(func[1], CompositeFunction))
        self.assertEqual(len(func[1]), 2)
        self.assertEqual(func.getParameterValue("f0.A0"), 1.0)
        self.assertEqual(func.getParameterValue("f1.f0.A0"), 2.0)
        self.assertEqual(func.getParameterValue("f1.f1.A0"), 3.0)
        self.assertEqual(func.nParams(), 3)


if __name__ == "__main__":
    unittest.main()
