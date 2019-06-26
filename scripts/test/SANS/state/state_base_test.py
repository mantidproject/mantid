# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import mantid

from mantid.api import Algorithm
from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.py3compat import Enum

from sans.state.state_base import (StringParameter, BoolParameter, FloatParameter, PositiveFloatParameter,
                                   PositiveIntegerParameter, DictParameter, EnumParameter,
                                   FloatWithNoneParameter, PositiveFloatWithNoneParameter, FloatListParameter,
                                   StringListParameter, PositiveIntegerListParameter, EnumListParameter,
                                   StateBase, rename_descriptor_names, TypedParameter, validator_sub_state,
                                   create_deserialized_sans_state_from_property_manager)

TestType = Enum("TestType", "TypeA TypeB")


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# Test the typed parameters
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateBaseTestClass(StateBase):
    string_parameter = StringParameter()
    bool_parameter = BoolParameter()
    float_parameter = FloatParameter()
    positive_float_parameter = PositiveFloatParameter()
    positive_integer_parameter = PositiveIntegerParameter()
    dict_parameter = DictParameter()
    float_with_none_parameter = FloatWithNoneParameter()
    positive_float_with_none_parameter = PositiveFloatWithNoneParameter()
    float_list_parameter = FloatListParameter()
    string_list_parameter = StringListParameter()
    positive_integer_list_parameter = PositiveIntegerListParameter()
    enum_parameter = EnumParameter(TestType)
    enum_list_parameter = EnumListParameter(TestType)

    def __init__(self):
        super(StateBaseTestClass, self).__init__()

    def validate(self):
        pass


class TypedParameterTest(unittest.TestCase):
    def _check_that_raises(self, error_type, obj, descriptor_name, value):
        try:
            setattr(obj, descriptor_name, value)
            self.fail()
        except error_type:
            pass
        except Exception as e:  # noqa
            self.fail()

    def test_that_can_set_to_valid_value_of_correct_type(self):
        test_class = StateBaseTestClass()
        try:
            test_class.string_parameter = "Test"
            test_class.bool_parameter = True
            test_class.float_parameter = -23.5768
            test_class.positive_float_parameter = 234.5
            test_class.positive_integer_parameter = 12
            test_class.dict_parameter = {}
            test_class.dict_parameter = {"test": 12, "test2": 13}
            test_class.float_with_none_parameter = None
            test_class.float_with_none_parameter = -123.67
            test_class.positive_float_with_none_parameter = None
            test_class.positive_float_with_none_parameter = 123.67
            test_class.float_list_parameter = [12., -123., 2355.]
            test_class.string_list_parameter = ["test", "test"]
            test_class.positive_integer_list_parameter = [1, 2, 4]
            test_class.enum_parameter = TestType.TypeA
            test_class.enum_list_parameter = [TestType.TypeA, TestType.TypeB]

        except ValueError:
            self.fail()

    def test_that_will_raise_type_error_if_set_with_wrong_type(self):
        test_class = StateBaseTestClass()
        self._check_that_raises(TypeError, test_class, "string_parameter", 1.)
        self._check_that_raises(TypeError, test_class, "bool_parameter", 1.)
        self._check_that_raises(TypeError, test_class, "float_parameter", "test")
        self._check_that_raises(TypeError, test_class, "positive_float_parameter", "test")
        self._check_that_raises(TypeError, test_class, "positive_integer_parameter", "test")
        self._check_that_raises(TypeError, test_class, "dict_parameter", "test")
        self._check_that_raises(TypeError, test_class, "float_with_none_parameter", "test")
        self._check_that_raises(TypeError, test_class, "positive_float_with_none_parameter", "test")
        self._check_that_raises(TypeError, test_class, "float_list_parameter", [1.23, "test"])
        self._check_that_raises(TypeError, test_class, "string_list_parameter", ["test", "test", 123.])
        self._check_that_raises(TypeError, test_class, "positive_integer_list_parameter", [1, "test"])
        self._check_that_raises(TypeError, test_class, "enum_parameter", "test")
        self._check_that_raises(TypeError, test_class, "enum_list_parameter", ["test", TestType.TypeA])

    def test_that_will_raise_if_set_with_wrong_value(self):
        # Note that this check does not apply to all parameter, it checks the validator
        test_class = StateBaseTestClass()
        self._check_that_raises(ValueError, test_class, "positive_float_parameter", -1.2)
        self._check_that_raises(ValueError, test_class, "positive_integer_parameter", -1)
        self._check_that_raises(ValueError, test_class, "positive_float_with_none_parameter", -234.)
        self._check_that_raises(ValueError, test_class, "positive_integer_list_parameter", [1, -2, 4])

# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# Test the sans_parameters decorator
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------

class SANSParameterTest(unittest.TestCase):
    @rename_descriptor_names
    class SANSParameterTestClass(object):
        my_string_parameter = StringParameter()
        my_bool_parameter = BoolParameter()

    class SANSParameterTestClass2(object):
        my_string_parameter = StringParameter()
        my_bool_parameter = BoolParameter()

    def test_that_name_is_in_readable_format_in_instance_dictionary(self):
        test_class = SANSParameterTest.SANSParameterTestClass()
        test_class.my_string_parameter = "test"
        test_class.my_bool_parameter = True
        keys = list(test_class.__dict__.keys())
        # We don't have a sensible name in the instance dictionary
        self.assertTrue("_BoolParameter#my_bool_parameter" in keys)
        self.assertTrue("_StringParameter#my_string_parameter" in keys)

    def test_that_name_cannot_be_found_in_instance_dictionary_when_sans_parameters_decorator_is_not_applied(self):
        test_class = SANSParameterTest.SANSParameterTestClass2()
        test_class.my_string_parameter = "test"
        test_class.my_bool_parameter = True
        keys = list(test_class.__dict__.keys())
        # We don't have a sensible name in the instance dictionary.
        # It will be rather stored as something like: _BoolParameter#2 etc.
        self.assertTrue("_BoolParameter#my_bool_parameter" not in keys)
        self.assertTrue("_StringParameter#my_string_parameter" not in keys)


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# StateBase
# This will mainly test serialization
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------

@rename_descriptor_names
class VerySimpleState(StateBase):
    string_parameter = StringParameter()

    def __init__(self):
        super(VerySimpleState, self).__init__()
        self.string_parameter = "test_in_very_simple"

    def validate(self):
        pass


@rename_descriptor_names
class SimpleState(StateBase):
    string_parameter = StringParameter()
    bool_parameter = BoolParameter()
    float_parameter = FloatParameter()
    positive_float_parameter = PositiveFloatParameter()
    positive_integer_parameter = PositiveIntegerParameter()
    dict_parameter = DictParameter()
    float_with_none_parameter = FloatWithNoneParameter()
    positive_float_with_none_parameter = PositiveFloatWithNoneParameter()
    float_list_parameter = FloatListParameter()
    string_list_parameter = StringListParameter()
    positive_integer_list_parameter = PositiveIntegerListParameter()
    enum_parameter = EnumParameter(TestType)
    enum_list_parameter = EnumListParameter(TestType)

    sub_state_very_simple = TypedParameter(VerySimpleState, validator_sub_state)

    def __init__(self):
        super(SimpleState, self).__init__()
        self.string_parameter = "String_in_SimpleState"
        self.bool_parameter = False
        # We explicitly leave out the float_parameter
        self.positive_float_parameter = 1.
        self.positive_integer_parameter = 6
        self.dict_parameter = {"1": 123, "2": "test"}
        self.float_with_none_parameter = 325.
        # We explicitly leave out the positive_float_with_none_parameter
        self.float_list_parameter = [123., 234.]
        self.string_list_parameter = ["test1", "test2"]
        self.positive_integer_list_parameter = [1, 2, 3]
        self.enum_parameter = TestType.TypeA
        self.enum_list_parameter = [TestType.TypeA, TestType.TypeB]
        self.sub_state_very_simple = VerySimpleState()

    def validate(self):
        pass


@rename_descriptor_names
class ComplexState(StateBase):
    float_parameter = FloatParameter()
    positive_float_with_none_parameter = PositiveFloatWithNoneParameter()
    sub_state_1 = TypedParameter(SimpleState, validator_sub_state)
    dict_parameter = DictParameter()

    def __init__(self):
        super(ComplexState, self).__init__()
        self.float_parameter = 23.
        self.positive_float_with_none_parameter = 234.
        self.sub_state_1 = SimpleState()
        self.dict_parameter = {"A": SimpleState(), "B": SimpleState()}

    def validate(self):
        pass


class TestStateBase(unittest.TestCase):
    def _assert_simple_state(self, state):
        self.assertEqual(state.string_parameter,  "String_in_SimpleState")
        self.assertFalse(state.bool_parameter)
        self.assertEqual(state.float_parameter, None)  # We did not set it on the instance
        self.assertEqual(state.positive_float_parameter,  1.)
        self.assertEqual(state.positive_integer_parameter,  6)
        self.assertEqual(state.dict_parameter["1"],  123)
        self.assertEqual(state.dict_parameter["2"],  "test")
        self.assertEqual(state.float_with_none_parameter,  325.)
        self.assertEqual(state.positive_float_with_none_parameter, None)

        self.assertEqual(len(state.float_list_parameter),  2)
        self.assertEqual(state.float_list_parameter[0],  123.)
        self.assertEqual(state.float_list_parameter[1],  234.)

        self.assertEqual(len(state.string_list_parameter),  2)
        self.assertEqual(state.string_list_parameter[0],  "test1")
        self.assertEqual(state.string_list_parameter[1],  "test2")

        self.assertEqual(len(state.positive_integer_list_parameter),  3)
        self.assertEqual(state.positive_integer_list_parameter[0],  1)
        self.assertEqual(state.positive_integer_list_parameter[1],  2)
        self.assertEqual(state.positive_integer_list_parameter[2],  3)

        self.assertEqual(state.enum_parameter, TestType.TypeA)
        self.assertEqual(len(state.enum_list_parameter),  2)
        self.assertEqual(state.enum_list_parameter[0],  TestType.TypeA)
        self.assertEqual(state.enum_list_parameter[1],  TestType.TypeB)

        self.assertEqual(state.sub_state_very_simple.string_parameter,  "test_in_very_simple")
        
    def test_that_sans_state_can_be_serialized_and_deserialized_when_going_through_an_algorithm(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass

        # Arrange
        state = ComplexState()

        # Act
        serialized = state.property_manager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        self.assertEqual(type(serialized),  dict)
        self.assertEqual(type(property_manager),  PropertyManager)
        state_2 = create_deserialized_sans_state_from_property_manager(property_manager)
        state_2.property_manager = property_manager

        # The direct sub state
        self._assert_simple_state(state_2.sub_state_1)

        # The two states in the dictionary
        self._assert_simple_state(state_2.dict_parameter["A"])
        self._assert_simple_state(state_2.dict_parameter["B"])

        # The regular parameters
        self.assertEqual(state_2.float_parameter,  23.)
        self.assertEqual(state_2.positive_float_with_none_parameter,  234.)


if __name__ == '__main__':
    unittest.main()
