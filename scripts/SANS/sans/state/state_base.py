# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=too-few-public-methods, invalid-name

""" Fundamental classes and Descriptors for the State mechanism."""
from __future__ import (absolute_import, division, print_function)
from abc import (ABCMeta, abstractmethod)
import copy
import inspect
from functools import (partial)
from six import string_types, with_metaclass

from mantid.kernel import (PropertyManager, std_vector_dbl, std_vector_str, std_vector_int, std_vector_long)
from mantid.py3compat import Enum


# ---------------------------------------------------------------
# Validator functions
# ---------------------------------------------------------------
def is_not_none(value):
    return value is not None


def is_positive(value):
    return value >= 0


def is_positive_or_none(value):
    return value is None or value >= 0


def all_list_elements_are_of_specific_type_and_not_empty(value, comparison_type,
                                                         additional_comparison=lambda x: True, type_check=isinstance):
    """
    Ensures that all elements of a list are of a specific type and that the list is not empty

    :param value: the list to check
    :param comparison_type: the expected type of the elements of the list.
    :param additional_comparison: additional comparison lambda.
    :param type_check: the method which performs type checking.
    :return: True if the list is not empty and all types are as expected, else False.
    """
    is_of_type = True
    for element in value:
        # Perform type check
        if not type_check(element, comparison_type):
            is_of_type = False
        # Perform additional check
        if not additional_comparison(element):
            is_of_type = False

    if not value:
        is_of_type = False
    return is_of_type


def all_list_elements_are_of_instance_type_and_not_empty(value, comparison_type, additional_comparison=lambda x: True):
    """
    Ensures that all elements of a list are of a certain INSTANCE type and that the list is not empty.
    """
    return all_list_elements_are_of_specific_type_and_not_empty(value=value, comparison_type=comparison_type,
                                                                additional_comparison=additional_comparison,
                                                                type_check=isinstance)


def all_list_elements_are_of_class_type_and_not_empty(value, comparison_type, additional_comparison=lambda x: True):
    """
    Ensures that all elements of a list are of a certain INSTANCE type and that the list is not empty.
    """
    return all_list_elements_are_of_specific_type_and_not_empty(value=value, comparison_type=comparison_type,
                                                                additional_comparison=additional_comparison,
                                                                type_check=issubclass)


def all_list_elements_are_float_and_not_empty(value):
    typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=float)
    return typed_comparison(value)


def all_list_elements_are_float_and_positive_and_not_empty(value):
    typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=float,
                               additional_comparison=lambda x: x >= 0)
    return typed_comparison(value)


def all_list_elements_are_string_and_not_empty(value):
    typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=str)
    return typed_comparison(value)


def all_list_elements_are_int_and_not_empty(value):
    typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=int)
    return typed_comparison(value)


def all_list_elements_are_int_and_positive_and_not_empty(value):
    typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=int,
                               additional_comparison=lambda x: x >= 0)
    return typed_comparison(value)


def validator_sub_state(sub_state):
    is_valid = True
    try:
        sub_state.validate()
    except ValueError:
        is_valid = False
    return is_valid


# -------------------------------------------------------
# Parameters
# -------------------------------------------------------
class TypedParameter(object):
    """
    The TypedParameter descriptor allows the user to store/handle a type-checked value with an additional
    validator option, e.g. one can restrict the held parameter to be only a positive value.
    """
    __counter = 0

    def __init__(self, parameter_type, validator=lambda x: True):
        cls = self.__class__
        prefix = cls.__name__
        # pylint: disable=protected-access
        index = cls.__counter
        cls.__counter += 1
        # Name which is used to store value in the instance. This will be unique and not accessible via the standard
        # attribute access, since the developer/user cannot apply the hash symbol in their code (it is valid though
        # when writing into the __dict__). Note that the name which we generate here will be altered (via a
        # class decorator) in the classes which actually use the TypedParameter descriptor, to make it more readable.
        self.name = '_{0}#{1}'.format(prefix, index)
        self.parameter_type = parameter_type
        self.value = None
        self.validator = validator

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            if hasattr(instance, self.name):
                return getattr(instance, self.name)
            else:
                return None

    def __set__(self, instance, value):
        # Perform a type check
        self._type_check(value)
        if self.validator(value):
            # The descriptor should be holding onto its own data and return a deepcopy of the data.
            copied_value = copy.deepcopy(value)
            setattr(instance, self.name, copied_value)
        else:
            raise ValueError("Trying to set {0} with an invalid value of {1}".format(self.name, str(value)))

    def __delete__(self):
        raise AttributeError("Cannot delete the attribute {0}".format(self.name))

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, str(self.parameter_type),
                                                                               str(value), str(type(value))))


# ---------------------------------------------------
# Various standard cases of the TypedParameter
# ---------------------------------------------------
class StringParameter(TypedParameter):
    def __init__(self):
        super(StringParameter, self).__init__(str, is_not_none)


class BoolParameter(TypedParameter):
    def __init__(self):
        super(BoolParameter, self).__init__(bool, is_not_none)


class FloatParameter(TypedParameter):
    def __init__(self):
        super(FloatParameter, self).__init__(float, is_not_none)


class PositiveFloatParameter(TypedParameter):
    def __init__(self):
        super(PositiveFloatParameter, self).__init__(float, is_positive)


class PositiveIntegerParameter(TypedParameter):
    def __init__(self):
        super(PositiveIntegerParameter, self).__init__(int, is_positive)


class DictParameter(TypedParameter):
    def __init__(self):
        super(DictParameter, self).__init__(dict, is_not_none)


class ClassTypeParameter(TypedParameter):
    """
    This TypedParameter variant allows for storing a class type.

    This could be for example something from the SANSType module, e.g. CanonicalCoordinates.X
    It is something that is used frequently with the main of moving away from using strings where types
    should be used instead.
    """
    def __init__(self, class_type):
        super(ClassTypeParameter, self).__init__(class_type, is_not_none)

    def _type_check(self, value):
        if not issubclass(value, self.parameter_type):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class EnumParameter(TypedParameter):
    def __init__(self, enum_type):
        super(EnumParameter, self).__init__(enum_type, is_not_none)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class FloatWithNoneParameter(TypedParameter):
    def __init__(self):
        super(FloatWithNoneParameter, self).__init__(float)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type) and value is not None:
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class StringWithNoneParameter(TypedParameter):
    def __init__(self):
        super(StringWithNoneParameter, self).__init__(str)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type) and value is not None:
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class PositiveFloatWithNoneParameter(TypedParameter):
    def __init__(self):
        super(PositiveFloatWithNoneParameter, self).__init__(float, is_positive_or_none)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type) and value is not None:
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class FloatListParameter(TypedParameter):
    def __init__(self):
        super(FloatListParameter, self).__init__(list)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type) or not all_list_elements_are_float_and_not_empty(value):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class PositiveFloatListParameter(TypedParameter):
    def __init__(self):
        super(PositiveFloatListParameter, self).__init__(list, all_list_elements_are_float_and_positive_and_not_empty)

    def _type_check(self, value):

        if not isinstance(value, self.parameter_type) or not all_list_elements_are_float_and_not_empty(value):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class StringListParameter(TypedParameter):
        def __init__(self):
            super(StringListParameter, self).__init__(list, all_list_elements_are_string_and_not_empty)

        def _type_check(self, value):
            if not isinstance(value, self.parameter_type) or not all_list_elements_are_string_and_not_empty(value):
                raise TypeError("Trying to set {0} which expects a value of type {1}."
                                " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                                   value, type(value)))


class PositiveIntegerListParameter(TypedParameter):
    def __init__(self):
        super(PositiveIntegerListParameter, self).__init__(list,
                                                           all_list_elements_are_int_and_positive_and_not_empty)

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type) or not all_list_elements_are_int_and_not_empty(value):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, self.parameter_type,
                                                                               value, type(value)))


class ClassTypeListParameter(TypedParameter):
    def __init__(self, class_type):
        typed_comparison = partial(all_list_elements_are_of_class_type_and_not_empty, comparison_type=class_type)
        super(ClassTypeListParameter, self).__init__(list, typed_comparison)


class EnumListParameter(TypedParameter):
    def __init__(self, enum_type):
        typed_comparison = partial(all_list_elements_are_of_instance_type_and_not_empty, comparison_type=enum_type,
                                   additional_comparison=self._additional_comparison)
        super(EnumListParameter, self).__init__(list, typed_comparison)

    @staticmethod
    def _additional_comparison(value):
        if isinstance(value, Enum):
            return True
        else:
            raise TypeError("Expected type Enum, but {0} is of type {1} instead.".format(value, type(value)))


# ------------------------------------------------
# StateBase
# ------------------------------------------------
class StateBase(with_metaclass(ABCMeta, object)):
    """ The fundamental base of the SANS State"""

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    @abstractmethod
    def validate(self):
        pass


def rename_descriptor_names(cls):
    """
    Class decorator which changes the names of TypedParameters in a class instance in order to make it more readable.

    This is especially helpful for debugging. And also in order to find attributes in the dictionaries.
    :param cls: The class with the TypedParameters
    :return: The class with the TypedParameters
    """
    for attribute_name, attribute_value in list(cls.__dict__.items()):
        if isinstance(attribute_value, TypedParameter):
            attribute_value.name = '_{0}#{1}'.format(type(attribute_value).__name__, attribute_name)
    return cls


# ------------------------------------------------
# Serialization of the State
# ------------------------------------------------
# Serialization of the state object is currently done via generating a dict object. Reversely, we can generate a
# State object from a property manager object, not a dict object. This quirk results from the way Mantid
# treats property manager inputs and outputs (it reads in dicts and converts them to property manager objects).
# We might have to live with that for now.
#
# During serialization we place identifier tags into the serialized object, e.g. we add a specifier if the item
# is a State type at all and if so which state it is.


STATE_NAME = "state_name"
STATE_MODULE = "state_module"
SEPARATOR_SERIAL = "#"
class_type_parameter_id = "ClassTypeParameterID#"
MODULE = "__module__"


def is_state(property_manager):
    return property_manager.existsProperty(STATE_NAME) and property_manager.existsProperty(STATE_MODULE)


def is_float_vector(value):
    return isinstance(value, std_vector_dbl)


def is_string_vector(value):
    return isinstance(value, std_vector_str)


def is_int_vector(value):
    return isinstance(value, std_vector_int) or isinstance(value, std_vector_long)


def get_module_and_class_name(instance):
    if inspect.isclass(instance):
        module_name, class_name = str(instance.__dict__[MODULE]), str(instance.__name__)
    else:
        module_name, class_name = str(type(instance).__dict__[MODULE]), str(type(instance).__name__)
    return module_name, class_name


def provide_class_from_module_and_class_name(module_name, class_name):
    # Importlib seems to be missing on RHEL6, hence we resort to __import__
    try:
        from importlib import import_module
        module = import_module(module_name)
    except ImportError:
        if "." in module_name:
            _, mod_name = module_name.rsplit(".", 1)
        else:
            mod_name = None
        if not mod_name:
            module = __import__(module_name)
        else:
            module = __import__(module_name, fromlist=[mod_name])
    return getattr(module, class_name)


def provide_class(instance):
    module_name = instance.getProperty(STATE_MODULE).value
    class_name = instance.getProperty(STATE_NAME).value
    return provide_class_from_module_and_class_name(module_name, class_name)


def is_class_type_parameter(value):
    return isinstance(value, string_types) and class_type_parameter_id in value


def is_vector_with_class_type_parameter(value):
    is_vector_with_class_type = True
    contains_str = is_string_vector(value)
    if contains_str:
        for element in value:
            if not is_class_type_parameter(element):
                is_vector_with_class_type = False
    else:
        is_vector_with_class_type = False
    return is_vector_with_class_type


def get_module_and_class_name_from_encoded_string(encoder, value):
    without_encoder = value.replace(encoder, "")
    return without_encoder.split(SEPARATOR_SERIAL)


def create_module_and_class_name_from_encoded_string(class_type_id, module_name, class_name):
    return class_type_id + module_name + SEPARATOR_SERIAL + class_name


def create_sub_state(value):
    # We are dealing with a sub state. We first have to create it and then populate it
    sub_state_class = provide_class(value)
    # Create the sub state, populate it and set it on the super state
    sub_state = sub_state_class()
    sub_state.property_manager = value
    return sub_state


def get_descriptor_values(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptor_names = []
    descriptor_types = {}
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptor_names.append(descriptor_name)
            descriptor_types.update({descriptor_name: descriptor_object})

    # Get the descriptor values from the instance
    descriptor_values = {}
    for key in descriptor_names:
        if hasattr(instance, key):
            value = getattr(instance, key)
            if value is not None:
                descriptor_values.update({key: value})
    return descriptor_values, descriptor_types


def get_class_descriptor_types(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptors = {}
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptors.update({descriptor_name: type(descriptor_object)})
    return descriptors


def convert_state_to_dict(instance):
    """
    Converts the state object to a dictionary.

    :param instance: the instance which is to be converted
    :return: a serialized state object in the form of a dict
    """
    descriptor_values, descriptor_types = get_descriptor_values(instance)
    # Add the descriptors to a dict
    state_dict = dict()
    for key, value in list(descriptor_values.items()):
        # If the value is a SANSBaseState then create a dict from it
        # If the value is a dict, then we need to check what the sub types are
        # If the value is a ClassTypeParameter, then we need to encode it
        # If the value is a list of ClassTypeParameters, then we need to encode each element in the list
        if isinstance(value, StateBase):
            sub_state_dict = value.property_manager
            value = sub_state_dict
        elif isinstance(value, dict):
            # If we have a dict, then we need to watch out since a value in the dict might be a State
            sub_dictionary = {}
            for key_sub, val_sub in list(value.items()):
                if isinstance(val_sub, StateBase):
                    sub_dictionary_value = val_sub.property_manager
                else:
                    sub_dictionary_value = val_sub
                sub_dictionary.update({key_sub: sub_dictionary_value})
            value = sub_dictionary
        elif isinstance(descriptor_types[key], ClassTypeParameter):
            value = get_serialized_class_type_parameter(value)
        elif isinstance(descriptor_types[key], ClassTypeListParameter):
            if value:
                # If there are entries in the list, then convert them individually and place them into a list.
                # The list will contain a sequence of serialized ClassTypeParameters
                serialized_value = []
                for element in value:
                    serialized_element = get_serialized_class_type_parameter(element)
                    serialized_value.append(serialized_element)
                value = serialized_value

        state_dict.update({key: value})
    # Add information about the current state object, such as in which module it lives and what its name is
    module_name, class_name = get_module_and_class_name(instance)
    state_dict.update({STATE_MODULE: module_name})
    state_dict.update({STATE_NAME: class_name})
    return state_dict


def set_state_from_property_manager(instance, property_manager):
    """
    Set the State object from the information stored on a property manager object. This is the deserialization step.

    :param instance: the instance which is to be set with a values of the property manager
    :param property_manager: the property manager with the stored setting
    """
    def _set_element(inst, k_element, v_element):
        if k_element != STATE_NAME and k_element != STATE_MODULE:
            setattr(inst, k_element, v_element)

    keys = list(property_manager.keys())
    for key in keys:
        value = property_manager.getProperty(key).value
        # There are four scenarios that need to be considered
        # 1. ParameterManager 1: This indicates (most often) that we are dealing with a new state -> create it and
        #                      apply recursion
        # 2. ParameterManager 2: In some cases the ParameterManager object is actually a map rather than a state ->
        #                         populate the state
        # 3. String with special meaning: Admittedly this is a hack, but we are limited by the input property types
        #                                 of Mantid algorithms, which can be string, int, float and containers of these
        #                                 types (and PropertyManagerProperties). We need a wider range of types, such
        #                                 as ClassTypeParameters. These are encoded (as good as possible) in a string
        # 4. Vector of strings with special meaning: See point 3)
        # 5. Vector for float: This needs to handle Mantid's float array
        # 6. Vector for string: This needs to handle Mantid's string array
        # 7. Vector for int: This needs to handle Mantid's integer array
        # 8. Normal values: all is fine, just populate them
        if type(value) is PropertyManager and is_state(value):
            sub_state = create_sub_state(value)
            setattr(instance, key, sub_state)
        elif type(value) is PropertyManager:
            # We must be dealing with an actual dict descriptor
            sub_dict_keys = list(value.keys())
            dict_element = {}
            # We need to watch out if a value of the dictionary is a sub state
            for sub_dict_key in sub_dict_keys:
                sub_dict_value = value.getProperty(sub_dict_key).value
                if type(sub_dict_value) == PropertyManager and is_state(sub_dict_value):
                    sub_state = create_sub_state(sub_dict_value)
                    sub_dict_value_to_insert = sub_state
                else:
                    sub_dict_value_to_insert = sub_dict_value
                dict_element.update({sub_dict_key: sub_dict_value_to_insert})
            setattr(instance, key, dict_element)
        elif is_class_type_parameter(value):
            class_type_parameter = get_deserialized_class_type_parameter(value)
            _set_element(instance, key, class_type_parameter)
        elif is_vector_with_class_type_parameter(value):
            class_type_list = []
            for element in value:
                class_type_parameter = get_deserialized_class_type_parameter(element)
                class_type_list.append(class_type_parameter)
            _set_element(instance, key, class_type_list)
        elif is_float_vector(value):
            float_list_value = list(value)
            _set_element(instance, key, float_list_value)
        elif is_string_vector(value):
            string_list_value = list(value)
            _set_element(instance, key, string_list_value)
        elif is_int_vector(value):
            int_list_value = list(value)
            _set_element(instance, key, int_list_value)
        else:
            _set_element(instance, key, value)


def get_serialized_class_type_parameter(value):
    # The module will only know about the outer class name, therefore we need
    # 1. The module name
    # 2. The name of the outer class
    # 3. The name of the actual class
    module_name, class_name = get_module_and_class_name(value)
    outer_class_name = value.outer_class_name
    class_name = outer_class_name + SEPARATOR_SERIAL + class_name
    return create_module_and_class_name_from_encoded_string(class_type_parameter_id, module_name, class_name)


def get_deserialized_class_type_parameter(value):
    # We need to first get the outer class from the module
    module_name, outer_class_name, class_name = \
        get_module_and_class_name_from_encoded_string(class_type_parameter_id, value)
    outer_class_type_parameter = provide_class_from_module_and_class_name(module_name, outer_class_name)
    # From the outer class we can then retrieve the inner class which normally defines the users selection
    return getattr(outer_class_type_parameter, class_name)


def create_deserialized_sans_state_from_property_manager(property_manager):
    return create_sub_state(property_manager)
