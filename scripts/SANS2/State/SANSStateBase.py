﻿# pylint: disable=too-few-public-methods, invalid-name

""" Fundamental classes and Descriptors for the SANSState mechanism."""
from abc import (ABCMeta, abstractmethod)
import copy
import inspect

from mantid.kernel import PropertyManager


# ---------------------------------------------------------------
# Validator functions
# ---------------------------------------------------------------
def is_not_none(value):
    return value is not None


def is_positive(value):
    return value >= 0


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

    This could be for example something from the SANSEnumerations module, e.g. CanonicalCoordinates.X
    It is something that is used frequently with the main of moving away from using strings where types
    should be used instead.
    """
    def __init__(self, class_type):
        super(ClassTypeParameter, self).__init__(class_type, is_not_none)

    def _type_check(self, value):
        if not issubclass(value, self.parameter_type):
            raise TypeError("Trying to set {0} which expects a value of type {1}."
                            " Got a value of {2} which is of type: {3}".format(self.name, str(self.parameter_type),
                                                                               str(value), str(value)))


# ------------------------------------------------
# SANSStateBase
# ------------------------------------------------
class SANSStateBase(object):
    """ The fundamental base of the SANSState"""
    __metaclass__ = ABCMeta

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    @abstractmethod
    def validate(self):
        pass


def sans_parameters(cls):
    """
    Class decorator which changes the names of TypedParameters in a class instance in order to make it more readable.

    This is especially helpful for debugging. And also in order to find attributes in the dictionaries.
    :param cls: The class with the TypedParameters
    :return: The class with the TypedParameters
    """
    for attribute_name, attribute_value in cls.__dict__.iteritems():
        if isinstance(attribute_value, TypedParameter):
            attribute_value.name = '_{0}#{1}'.format(type(attribute_value).__name__, attribute_name)
    return cls


STATE_NAME = "state_name"
STATE_MODULE = "state_module"
SEPARATOR_SERIAL = "#"
class_type_parameter_id = "ClassTypeParameterID#"
MODULE = "__module__"


# ------------------------------------------------
# Serialization of the State
# ------------------------------------------------
def is_state(property_manager):
    return property_manager.existsProperty(STATE_NAME) and property_manager.existsProperty(STATE_MODULE)


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
        _, mod_name = module_name.rsplit(".", 1)
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
    return isinstance(value, basestring) and class_type_parameter_id in value


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
    descriptor_values, descriptor_types = get_descriptor_values(instance)
    # Add the descriptors to a dict
    state_dict = dict()
    for key, value in descriptor_values.iteritems():
        # If the value is a SANSBaseState then create a dict from it
        # If the value is a dict, then we need to check what the sub types are
        # IF the value is a ClassTypeParameter, then we need to encode it
        if isinstance(value, SANSStateBase):
            sub_state_dict = value.property_manager
            value = sub_state_dict
        elif isinstance(value, dict):
            # If we have a dict, then we need to watch out since a value in the dict might be a SANSState
            sub_dictionary = {}
            for key_sub, val_sub in value.iteritems():
                if isinstance(val_sub, SANSStateBase):
                    sub_dictionary_value = val_sub.property_manager
                else:
                    sub_dictionary_value = val_sub
                sub_dictionary.update({key_sub: sub_dictionary_value})
            value = sub_dictionary
        elif isinstance(descriptor_types[key], ClassTypeParameter):
            # The module will only know about the outer class name, therefore we need
            # 1. The module name
            # 2. The name of the outer class
            # 3. The name of the actual class
            module_name, class_name = get_module_and_class_name(value)
            outer_class_name = value.outer_class_name
            class_name = outer_class_name + SEPARATOR_SERIAL + class_name
            value = create_module_and_class_name_from_encoded_string(class_type_parameter_id, module_name, class_name)

        state_dict.update({key: value})
    # Add information about the current state object, such as in which module it lives and what its name is
    module_name, class_name = get_module_and_class_name(instance)
    state_dict.update({STATE_MODULE: module_name})
    state_dict.update({STATE_NAME: class_name})
    return state_dict


def set_state_from_property_manager(instance, property_manager):
    def _set_element(inst, k_element, v_element):
        if k_element != STATE_NAME and k_element != STATE_MODULE:
            setattr(inst, k_element, v_element)

    keys = property_manager.keys()
    for key in keys:
        value = property_manager.getProperty(key).value
        # There are four scenarios that need to be considered
        # 1. ParameterManager 1: This indicates (most often) that we are dealing with a new state -> create it and
        #                      apply recursion
        # 2. ParameterManager 2: In some cases the ParameterManager object is actually a map rather than a state ->
        #                         populate the state
        # 3. String with special meaning: Admittedly this is a hack, but we limited by the input property types
        #                                 of Mantid algorithms, which can be string, int, float and containers of these
        #                                 types (and PropertyManagerProperties). We need a wider range of types, such
        #                                 as ClassTypeParameters. These are encoded (as good as possible) in a string
        # 4. Normal values: all is fine, just populate them
        if type(value) == PropertyManager and is_state(value):
            sub_state = create_sub_state(value)
            setattr(instance, key, sub_state)
        elif type(value) == PropertyManager:
            # We must be dealing with an actual dict descriptor
            sub_dict_keys = value.keys()
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
            # We need to first get the outer class from the module
            module_name, outer_class_name, class_name = \
                get_module_and_class_name_from_encoded_string(class_type_parameter_id, value)
            outer_class_type_parameter = provide_class_from_module_and_class_name(module_name, outer_class_name)
            # From the outer class we can then retrieve the inner class which normally defines the users selection
            class_type_parameter = getattr(outer_class_type_parameter, class_name)
            _set_element(instance, key, class_type_parameter)
        else:
            _set_element(instance, key, value)


def create_deserialized_sans_state_from_property_manager(property_manager):
    return create_sub_state(property_manager)
