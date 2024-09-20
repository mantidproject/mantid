# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
from enum import Enum


from sans.state.JsonSerializable import JsonSerializable


class Serializer(object):
    @staticmethod
    def to_json(obj):
        return json.dumps(obj, cls=SerializerImpl)

    @staticmethod
    def from_json(json_str):
        assert isinstance(json_str, str)
        return json.loads(json_str, object_hook=SerializerImpl.obj_hook)

    @staticmethod
    def load_file(file_path):
        with open(file_path, "r") as f:
            return json.load(f, object_hook=SerializerImpl.obj_hook)

    @staticmethod
    def save_file(obj, file_path):
        with open(file_path, "w") as f:
            json.dump(obj, f, cls=SerializerImpl, sort_keys=True, indent=4)


class SerializerImpl(json.JSONEncoder):
    def default(self, o):
        metaclass = type(type(o))  # Get class of o, then get metaclass of the class type
        if issubclass(metaclass, JsonSerializable):
            tag = JsonSerializable.tag_type(type(o))
            return {tag: o.__dict__}

        if isinstance(o, Enum):
            tag = JsonSerializable.tag_type(type(o))
            return {tag: o.value}

        if isinstance(o, tuple) and hasattr(o, "_fields"):
            raise ValueError("A NamedTuple was passed to the JSON encoder, this is not supported as" " it will be deserialized to a list")

        return json.JSONEncoder.default(self, o)

    @staticmethod
    def obj_hook(o):
        for type_tag, internal_dict in o.items():
            assert isinstance(type_tag, str)

            cls_type = JsonSerializable.class_type_from_tag(type_tag)
            if cls_type:
                return SerializerImpl._reconstruct_class(cls_type, internal_dict)

            enum_type = JsonSerializable._enum_type_from_tag(type_tag)
            if enum_type:
                return SerializerImpl._reconstruct_enum(enum_type, internal_dict)

        return o

    @staticmethod
    def _reconstruct_class(found_type, ordered_dict):
        assert isinstance(found_type, type)
        assert isinstance(ordered_dict, dict)
        obj = found_type()
        obj.__dict__ = ordered_dict
        return obj

    @staticmethod
    def _reconstruct_enum(found_type, val):
        return found_type(val)
