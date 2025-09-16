# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Tests for PythonObjectProperty"""

import unittest
from mantid.simpleapi import CreateWorkspace, mtd
from mantid.kernel import Direction, PythonObjectProperty, PythonObjectTypeValidator
from mantid.api import Algorithm, WorkspaceProperty


class FakeAlgorithm(Algorithm):
    def PyInit(self):
        self.declareProperty(PythonObjectProperty("PyObject"))

    def PyExec(self):
        pass


class NestedClass:
    def __init__(self, a):
        self.a = a


class FakeClass:
    def __init__(self, x, y):
        self.x = x
        self.y = NestedClass(y)


class FakeValidatedAlgorithm(Algorithm):
    def PyInit(self):
        validator = PythonObjectTypeValidator(FakeClass)
        self.declareProperty(PythonObjectProperty("PyObject", None, validator))

    def PyExec(self):
        pass


class PythonObjectPropertyTest(unittest.TestCase):
    def test_construction_with_no_value_defaults_to_input(self):
        prop = PythonObjectProperty("MyInputProperty")

        self.assertEqual(prop.direction, Direction.Input)

    def test_construction_with_direction_is_respected(self):
        prop = PythonObjectProperty("MyOutputProperty", direction=Direction.Output)

        self.assertEqual(prop.direction, Direction.Output)

    def test_construction_with_no_value_defaults_to_None(self):
        prop = PythonObjectProperty("MyProperty")

        self.assertEqual(
            "MyProperty",
            prop.name,
        )
        self.assertEqual(Direction.Input, prop.direction)
        self.assertIs(prop.value, None)

    def test_construction_with_default_value_returns_same_value(self):
        default_value = 10
        prop = PythonObjectProperty("MyProperty", default_value)

        self.assertEqual(prop.name, "MyProperty")
        self.assertEqual(prop.direction, Direction.Input)
        self.assertEqual(prop.value, default_value)

    def test_python_validator(self):
        # make sure a validator cannot be created from a non-class-type object
        x = 7
        with self.assertRaises(Exception):
            PythonObjectTypeValidator(x)

        x = {"key": 12, "other": 3.141592}
        with self.assertRaises(Exception):
            PythonObjectTypeValidator(x)

        # make sure it can be created from a standard type
        intValidator = PythonObjectTypeValidator(int)
        self.assertEqual(intValidator.isValid(12), "")
        self.assertEqual(intValidator.isValid(3.141592), "The passed object is of type float and not of type int")
        self.assertEqual(intValidator.isValid(None), "The passed object is of type NoneType and not of type int")

        # make sure it can be created from a custom type
        class A:
            pass

        a = A()

        classValidator = PythonObjectTypeValidator(A)
        self.assertEqual(classValidator.isValid(a), "")
        self.assertEqual(classValidator.isValid(12), "The passed object is of type int and not of type A")

    def test_construction_with_validator_int(self):
        # create a type validator and init a property with it
        validator = PythonObjectTypeValidator(int)
        prop = PythonObjectProperty("PyObjectProperty", None, validator)

        # check for int
        a_int = 100001000000001
        res = prop.setValue(a_int)
        # successful setting will register as an empty string being returned
        self.assertEqual(res, "")
        self.assertEqual(a_int, prop.value)

        # try setting with a float
        a_float = 3.141592
        res = prop.setValue(a_float)
        # an error condition will register as a string message being returned
        self.assertNotEqual(res, "")
        # make sure the value has not changed and is still valid
        self.assertEqual(a_int, prop.value)

    def test_construction_with_validator_classes(self):
        # two stub classes for the check
        class A:
            pass

        class B:
            pass

        # not create a type validator for A and init a property with it
        validator = PythonObjectTypeValidator(A)
        prop = PythonObjectProperty("PyObjectProperty", None, validator)

        # check success with A objects
        a = A()
        res = prop.setValue(a)
        # successful setting will register as an empty string being returned
        self.assertEqual(res, "")
        self.assertEqual(a, prop.value)

        # check failure with B objects
        b = B()
        res = prop.setValue(b)
        # an error condition will register as a string message being returned
        self.assertNotEqual(res, "")
        # make sure the value has not changed and is still valid
        self.assertEqual(a, prop.value)

    def test_set_property_on_algorithm_PyObject(self):
        fake = FakeAlgorithm()
        fake.initialize()

        value = object()
        fake.setProperty("PyObject", value)

        # It is important that both the value and address match to prove it
        # is just the same object that we got back
        self.assertIs(value, fake.getProperty("PyObject").value)
        self.assertEqual(value, fake.getProperty("PyObject").value)
        self.assertEqual(id(value), id(fake.getProperty("PyObject").value))

    def test_set_property_on_algorithm_class(self):
        fake = FakeAlgorithm()
        fake.initialize()

        value = FakeClass(16, "NXshorts")
        fake.setProperty("PyObject", value)

        # It is important that both the value and address match to prove it
        # is just the same object that we got back
        self.assertIs(value, fake.getProperty("PyObject").value)
        self.assertEqual(value, fake.getProperty("PyObject").value)
        self.assertEqual(id(value), id(fake.getProperty("PyObject").value))
        # check individual properties
        ret = fake.getProperty("PyObject").value
        self.assertEqual(ret.x, value.x)
        self.assertEqual(ret.y, value.y)

    def test_set_property_on_algorithm_class_with_validator(self):
        fake = FakeValidatedAlgorithm()
        fake.initialize()

        value = {"not": "correct"}
        with self.assertRaises(Exception):
            fake.setProperty("PyObject", value)

        value = FakeClass(16, "NXshorts")
        fake.setProperty("PyObject", value)

        # It is important that both the value and address match to prove it
        # is just the same object that we got back
        self.assertIs(value, fake.getProperty("PyObject").value)
        self.assertEqual(value, fake.getProperty("PyObject").value)
        self.assertEqual(id(value), id(fake.getProperty("PyObject").value))

    def test_value_int(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for int
        a_int = 100001000000001
        fake.setProperty("PyObject", a_int)
        self.assertEqual(str(a_int), fake.getPropertyValue("PyObject"))
        fake.setProperty("PyObject", str(a_int))
        self.assertEqual(a_int, fake.getProperty("PyObject").value)

    def test_value_float(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for float
        a_float = 3.141592 / 2.718281828
        fake.setProperty("PyObject", a_float)
        self.assertEqual(str(a_float), fake.getPropertyValue("PyObject"))
        self.assertEqual("1.1557271095438455", fake.getPropertyValue("PyObject"))  # NOTE 16 digits
        fake.setProperty("PyObject", str(a_float))
        self.assertEqual(a_float, fake.getProperty("PyObject").value)

    def test_value_list(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for list
        a_list = [3.4, 5]
        fake.setProperty("PyObject", a_list)
        self.assertEqual(str(a_list), fake.getPropertyValue("PyObject"))
        fake.setProperty("PyObject", str(a_list))
        self.assertEqual(a_list, fake.getProperty("PyObject").value)

    def test_value_dict(self):
        import json

        fake = FakeAlgorithm()
        fake.initialize()

        # check for dict
        a_dict = {"key1": 12, "key2": -14}
        fake.setProperty("PyObject", a_dict)
        self.assertEqual(json.dumps(a_dict), fake.getPropertyValue("PyObject"))
        fake.setProperty("PyObject", json.dumps(a_dict))
        self.assertEqual(a_dict, fake.getProperty("PyObject").value)

    def test_value_failure_tuple_keys(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for list
        a_dict = {(1, 2): 1, (3, 4): 4}
        fake.setProperty("PyObject", a_dict)
        self.assertEqual("<unrepresentable object>", fake.getPropertyValue("PyObject"))

    def test_value_string(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for string
        a_string = "hello world"
        fake.setProperty("PyObject", a_string)
        self.assertEqual(a_string, fake.getPropertyValue("PyObject"))
        self.assertEqual(a_string, fake.getProperty("PyObject").value)

    def test_value_class(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for class
        obj = FakeClass(7, "frnakling")
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, set the property from the string, ensure same
        propVal = fake.getPropertyValue("PyObject")
        fake.setPropertyValue("PyObject", propVal)
        objAsJson = {"x": 7, "y": {"a": "frnakling"}}
        self.assertEqual(objAsJson, fake.getProperty("PyObject").value)

    def test_value_list_of_class(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for class
        obj = [FakeClass(1, 2), FakeClass(3, 4)]
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, set the property from the string, ensure same
        propVal = fake.getPropertyValue("PyObject")
        fake.setPropertyValue("PyObject", propVal)
        objAsJson = [{"x": 1, "y": {"a": 2}}, {"x": 3, "y": {"a": 4}}]
        self.assertEqual(objAsJson, fake.getProperty("PyObject").value)

    def test_value_dict_of_class(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # check for class
        obj = {"key1": FakeClass(1, 2), "key2": FakeClass(3, 4)}
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, set the property from the string, ensure same
        propVal = fake.getPropertyValue("PyObject")
        fake.setPropertyValue("PyObject", propVal)
        objAsJson = {"key1": {"x": 1, "y": {"a": 2}}, "key2": {"x": 3, "y": {"a": 4}}}
        self.assertEqual(objAsJson, fake.getProperty("PyObject").value)

    def test_value_class_recursion_stops_list(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # create a self-referential object
        obj = [1]
        obj[0] = obj
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, ensure it terminates
        propVal = fake.getPropertyValue("PyObject")
        self.assertIn("...", propVal)

    def test_value_class_recursion_stops_dict(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # create a self-referential object
        obj = {"key": "value"}
        obj["key"] = obj
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, ensure it terminates
        propVal = fake.getPropertyValue("PyObject")
        self.assertIn("...", propVal)

    def test_value_class_recursion_stops_class(self):
        fake = FakeAlgorithm()
        fake.initialize()

        # create a self-referential object
        obj = FakeClass(1, 2)
        obj.y = obj
        fake.setProperty("PyObject", obj)
        self.assertIs(obj, fake.getProperty("PyObject").value)

        # take the string, ensure it terminates
        propVal = fake.getPropertyValue("PyObject")
        self.assertIn("...", propVal)

    def test_value_in_ws_history(self):
        class FakeWsAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PythonObjectProperty("PyObject"))
                self.declareProperty(WorkspaceProperty("InputWorkspace", "", Direction.Input))
                self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output))

            def PyExec(self):
                self.setProperty("OutputWorkspace", self.getProperty("InputWorkspace").value)

        # run the fake algorithm with a FakeClass and a Workspace property
        ws = CreateWorkspace([1], [1])
        value = FakeClass(16, "NXShorts")

        fake = FakeWsAlgorithm()
        fake.initialize()
        fake.setProperty("PyObject", value)
        fake.setProperty("InputWorkspace", ws)
        fake.setProperty("OutputWorkspace", "outws")
        fake.execute()

        historyValue = mtd["outws"].getHistory().getAlgorithmHistory(1).getPropertyValue("PyObject")
        self.assertEqual(historyValue, '{"x": 16, "y": {"a": "NXShorts"}}')


if __name__ == "__main__":
    unittest.main()
