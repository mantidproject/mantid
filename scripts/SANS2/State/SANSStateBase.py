""" Fundamental classes and Descriptors for the SANSState mechanism."""
from abc import (ABCMeta, abstractmethod)
import copy


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
            # The descriptor should be holding onto its own data and return a deepcopy of the data.
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
            raise TypeError("Trying to set {} which expects a value of type {}."
                            " Got a value of {} which is of type: {}".format(self.name, str(self.parameter_type),
                                                                             str(value), str(value)))


# ------------------------------------------------
# SANSStateBase
# ------------------------------------------------
class SANSStateBase(object):
    """ The fundamental base of the SANSState"""
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

    This is especially helpful for debugging. And also in order to find attributes in the dictionaries.
    :param cls: The class with the TypedParameters
    :return: The class with the TypedParameters
    """
    for attribute_name, attribute_value in cls.__dict__.iteritems():
        if isinstance(attribute_value, TypedParameter):
            attribute_value.name = '_{}#{}'.format(type(attribute_value).__name__, attribute_name)
    return cls
