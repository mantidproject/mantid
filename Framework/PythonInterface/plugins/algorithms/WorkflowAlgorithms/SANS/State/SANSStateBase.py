from abc import (ABCMeta, abstractmethod)
import inspect
from mantid.kernel import PropertyManager


# -------------------------------------------------------
# Parameters
# -------------------------------------------------------
class TypedParameter(object):
    def __init__(self, name, parameter_type, validator=lambda x: True, default=None):
        self.name = "_" + name
        self.parameter_type = parameter_type
        self.value = default if isinstance(default, parameter_type) else None
        self.validator = validator

    def __get__(self, instance, owner):
        _owner = owner
        if instance is None:
            return self
        else:
            return getattr(instance, self.name, self.value)

    def __set__(self, instance, value):
        if not isinstance(value, self.parameter_type):
            raise TypeError("Trying to set {} which expects a value of type {}."
                            " Got a value of {} which is of type {}".format(self.name, str(self.parameter_type),
                                                                            str(value), str(type(value))))
        if self.validator(value):
            setattr(instance, self.name, value)
        else:
            raise ValueError("Trying to set {} with an invalid value of {}".format(self.name, str(value)))

    def __delete__(self):
        raise AttributeError("Cannot delete the attribute {}".format(self.name))


# ---------------------------------------------------------------
# Validators
# ---------------------------------------------------------------
def is_not_none(value):
    return value is not None


def is_positive(value):
    return value >= 0


# ------------------------------------------------
# SANSStateBase
# ------------------------------------------------
class SANSStateBase:
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


class PropertyManagerConverter(object):
    def __init__(self):
        super(PropertyManagerConverter, self).__init__()

    @staticmethod
    def convert_state_to_property_manager(instance):
        # Get all descriptors of a class
        descriptor_names = []
        for descriptor_name, descriptor_object in inspect.getmembers(type(instance)):
            if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
                descriptor_names.append(descriptor_name)

        # Get the descriptor values from the instance
        descriptor_values = {}
        for key in descriptor_names:
            value = getattr(instance, key)
            descriptor_values.update({key: value})

        # Add the descriptors to a PropertyManager object
        property_manager = PropertyManager()
        for key in descriptor_values:
            value = descriptor_values[key]
            if value is not None:
                property_manager.declareProperty(key, value)
        return property_manager

