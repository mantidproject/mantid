# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum


def json_serializable(cls):  # Decorator for enums
    assert issubclass(cls, Enum)
    JsonSerializable._register_enum_type(cls)
    return cls


class JsonSerializable(type):
    """The fundamental base of the SANS State"""

    _derived_types = {}

    __ENUM_TAG = "E#"
    __TYPE_TAG = "T#"

    def __init__(cls, name, bases, dct):
        cls._derived_types[cls._tag_type(cls)] = cls
        super(JsonSerializable, cls).__init__(name, bases, dct)

    @staticmethod
    def tag_type(incoming_type):
        def check_in_dict(tag):
            if tag not in JsonSerializable._derived_types:
                raise RuntimeError(
                    "Trying to serialize enum {0} which is not registered with JsonSerializer"
                    "\nUse the add_json_support decorator on the enum".format(tag)
                )
            return tag

        metaclass = type(incoming_type)
        if issubclass(incoming_type, Enum):
            return check_in_dict(JsonSerializable._tag_enum(incoming_type))

        if issubclass(metaclass, JsonSerializable):
            return check_in_dict(JsonSerializable._tag_type(incoming_type))

        raise RuntimeError("Unknown type {0} passed to tag_type".format(incoming_type))

    @staticmethod
    def class_type_from_tag(tag):
        return JsonSerializable._find_type(tag, JsonSerializable.__TYPE_TAG)

    @staticmethod
    def _enum_type_from_tag(tag):
        return JsonSerializable._find_type(tag, JsonSerializable.__ENUM_TAG)

    @staticmethod
    def _find_type(tag, type_to_search_for):
        if not tag.startswith(type_to_search_for):
            return

        try:
            return JsonSerializable._derived_types[tag]
        except KeyError:
            if tag.startswith(type_to_search_for):
                raise RuntimeError("Trying to deserialize {0} which is not registered with JsonSerializer".format(tag))

    @staticmethod
    def _register_enum_type(e_type):
        JsonSerializable._derived_types[JsonSerializable._tag_enum(e_type)] = e_type

    @staticmethod
    def _tag_enum(t):
        return JsonSerializable.__ENUM_TAG + t.__name__

    @staticmethod
    def _tag_type(t):
        return JsonSerializable.__TYPE_TAG + t.__name__
