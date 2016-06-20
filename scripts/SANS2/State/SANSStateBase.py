from abc import (ABCMeta, abstractmethod)
import inspect
import copy
from mantid.kernel import PropertyManager


# ------------------------------------------------
# Free functions
# ------------------------------------------------
def get_descriptor_values(instance):
    # Get all descriptor names which are TypedParameter of instance's type
    descriptor_names = []
    for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
        if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
            descriptor_names.append(descriptor_name)

    # Get the descriptor values from the instance
    descriptor_values = {}
    for key in descriptor_names:
        if hasattr(instance, key):
            value = getattr(instance, key)
            if value is not None:
                descriptor_values.update({key: value})
    return descriptor_values


def convert_state_to_property_manager(instance):
    descriptor_values = get_descriptor_values(instance)

    # Add the descriptors to a PropertyManager object
    property_manager = PropertyManager()
    for key in descriptor_values:
        value = descriptor_values[key]
        if value is not None:
            property_manager.declareProperty(key, value)
    return property_manager


def convert_state_to_dict(instance):
    descriptor_values = get_descriptor_values(instance)
    # Add the descriptors to a dict
    state_dict = dict()
    for key, value in descriptor_values.iteritems():
        # If the value is a SANSBaseState then create a dict from it
        if isinstance(value, SANSStateBase):
            sub_state_dict = convert_state_to_dict(value)
            value = sub_state_dict
        state_dict.update({key: value})
    return state_dict


def set_state_from_property_manager(instance, property_manager):
    keys = property_manager.keys()
    for key in keys:
        value = property_manager.getProperty(key).value
        setattr(instance, key, value)


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
        index = cls.__counter
        # Name which is used to store value in the instance. This will be unique and not accessible via the standard
        # attribute access, since the developer/user cannot apply the hash symbol in their code (it is valid though
        # when writing into the __dict__). Note that the name which we generate here will be altered (via a
        # class decorator) in the classes which actually use the TypedParameter descriptor, to make it more readable.
        self.name = '_{}#{}'.format(prefix, index)
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
            # The descriptor should be holding onto its own copy
            copied_value = copy.deepcopy(value)
            setattr(instance, self.name, copied_value)
        else:
            raise ValueError("Trying to set {} with an invalid value of {}".format(self.name, str(value)))

    def __delete__(self):
        raise AttributeError("Cannot delete the attribute {}".format(self.name))

    def _type_check(self, value):
        if not isinstance(value, self.parameter_type):
            raise TypeError("Trying to set {} which expects a value of type {}."
                            " Got a value of {} which is of type: {}".format(self.name, str(self.parameter_type),
                                                                             str(value), str(type(value))))


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
    This TypedParameter variant allows for storing a class type. We make use of this quite a lot
    """
    def __init__(self, class_type):
        super(ClassTypeParameter, self).__init__(class_type, is_not_none)

    def _type_check(self, value):
        if not issubclass(value, self.parameter_type):
            raise TypeError("Trying to set {} which expects a value of type {}."
                            " Got a value of {} which is of type: {}".format(self.name, str(self.parameter_type),
                                                                             str(value), str(value)))


# ------------------------------------------------
# SANSStateBase
# ------------------------------------------------
class SANSStateBase(object):
    __metaclass__ = ABCMeta

    @property
    @abstractmethod
    def property_manager(self):
        pass

    @property_manager.setter
    @abstractmethod
    def property_manager(self, value):
        pass

    @abstractmethod
    def validate(self):
        pass


def sans_parameters(cls):
    """
    Class decorator which changes the names of TypedParameters in a class instance in order to make it more readable.
    :param cls: The class with the TypedParameters
    :return: The class with the TypedParameters
    """
    for attribute_name, attribute_value in cls.__dict__.iteritems():
        if isinstance(attribute_value, TypedParameter):
            attribute_value.name = '_{}#{}'.format(type(attribute_value).__name__, attribute_name)
    return cls
