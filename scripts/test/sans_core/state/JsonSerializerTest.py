# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import Algorithm
from enum import Enum


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# Test the typed parameters
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
from sans_core.state.JsonSerializable import JsonSerializable, json_serializable
from sans_core.state.Serializer import Serializer


class TestClass(metaclass=JsonSerializable):
    string_parameter = None  # : Str()
    bool_parameter = None  # : Bool
    float_parameter = None  # : Float
    positive_float_parameter = None  # : Float (Positive)
    positive_integer_parameter = None  # : Int (Positive)
    dict_parameter = None  # : Dict
    float_with_none_parameter = None  # : Float
    positive_float_with_none_parameter = None  # : Float (Optional)
    float_list_parameter = None  # : List[Float]
    string_list_parameter = None  # : List[Str]
    positive_integer_list_parameter = None  # : List[Int] (Positive)

    def __init__(self):
        super(TestClass, self).__init__()

    def validate(self):
        pass


@json_serializable
class FakeEnumClass(Enum):
    FOO = 1
    BAR = "2"


class ExampleWrapper(metaclass=JsonSerializable):
    # This has to be at the top module level, else the module name finding will fail
    def __init__(self):
        self._foo = FakeEnumClass.FOO
        self.bar = FakeEnumClass.BAR

    def validate(self):
        return True


class VerySimpleState(metaclass=JsonSerializable):
    string_parameter = None  # : Str()

    def __init__(self):
        super(VerySimpleState, self).__init__()
        self.string_parameter = "test_in_very_simple"

    def validate(self):
        pass


class SimpleState(metaclass=JsonSerializable):
    def __init__(self):
        super(SimpleState, self).__init__()
        self.string_parameter = "String_in_SimpleState"
        self.bool_parameter = False
        # We explicitly leave out the float_parameter
        self.positive_float_parameter = 1.0
        self.positive_integer_parameter = 6
        self.dict_parameter = {"1": 123, "2": "test"}
        self.float_with_none_parameter = 325.0
        # We expliclty leave out the positive_float_with_none_parameter
        self.float_list_parameter = [123.0, 234.0]
        self.string_list_parameter = ["test1", "test2"]
        self.positive_integer_list_parameter = [1, 2, 3]
        self.sub_state_very_simple = VerySimpleState()

    def validate(self):
        pass


class ComplexState(metaclass=JsonSerializable):
    def __init__(self):
        super(ComplexState, self).__init__()
        self.float_parameter = 23.0
        self.positive_float_with_none_parameter = 234.0
        self.sub_state_1 = SimpleState()
        self.dict_parameter = {"A": SimpleState(), "B": SimpleState()}

    def validate(self):
        pass


class JsonSerializerTest(unittest.TestCase):
    class FakeAlgorithm(Algorithm):
        def PyInit(self):
            self.declareProperty("Args", "")

        def PyExec(self):
            pass

    def test_that_enum_can_be_serialized(self):
        original_obj = ExampleWrapper()

        # Serializing test
        serialized = Serializer.to_json(original_obj)
        self.assertTrue("bar" in serialized)
        self.assertTrue("_foo" in serialized)
        self.assertTrue(isinstance(serialized, str), "The type was not converted to a string")

        # Deserializing Test
        fake = JsonSerializerTest.FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        new_obj = Serializer.from_json(property_manager)
        self.assertEqual(FakeEnumClass.BAR, new_obj.bar)
        self.assertEqual(FakeEnumClass.FOO, new_obj._foo)

    def test_that_enum_list_can_be_serialized(self):
        original_obj = ExampleWrapper()
        original_obj.bar = [FakeEnumClass.BAR, FakeEnumClass.BAR]

        # Serializing test
        serialized = Serializer.to_json(original_obj)
        self.assertTrue("bar" in serialized)
        self.assertTrue("_foo" in serialized)
        self.assertTrue(isinstance(serialized, str))

        # Deserializing Test
        fake = JsonSerializerTest.FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        new_obj = Serializer.from_json(property_manager)
        self.assertEqual(original_obj.bar, new_obj.bar)
        self.assertEqual(original_obj._foo, new_obj._foo)

    def test_that_sans_state_can_be_serialized_and_deserialized_when_going_through_an_algorithm(self):
        # Arrange
        state = ComplexState()

        # Act
        serialized = Serializer.to_json(state)
        fake = JsonSerializerTest.FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        self.assertEqual(type(serialized), str)
        state_2 = Serializer.from_json(property_manager)

        # The direct sub state
        self.assertEqual(state.sub_state_1.float_list_parameter, state_2.sub_state_1.float_list_parameter)

        # The regular parameters
        self.assertEqual(state_2.float_parameter, 23.0)
        self.assertEqual(state_2.positive_float_with_none_parameter, 234.0)


if __name__ == "__main__":
    unittest.main()
