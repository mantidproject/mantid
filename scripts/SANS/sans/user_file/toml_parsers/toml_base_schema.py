# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import re
from abc import abstractmethod, ABCMeta


class TomlValidationError(Exception):
    # A key error would be more appropriate, but there is special
    # handling on KeyError which escapes any new lines making it un-readable
    pass


class TomlSchemaValidator(object, metaclass=ABCMeta):
    # As of the current TOML release there is no way to validate a schema so
    # we must provide an implementation

    def __init__(self, dict_to_validate):
        self._expected_list = self._build_nested_keys(self.reference_schema())
        self._to_validate_list = self._build_nested_keys(dict_to_validate)

    def validate(self):
        self._to_validate_list = filter(lambda s: not s.startswith("metadata"), self._to_validate_list)
        unrecognised = set(self._to_validate_list).difference(self._expected_list)

        if not unrecognised:
            return

        # Build any with wildcards
        wildcard_matchers = [re.compile(s) for s in self._expected_list if "*" in s]
        # Remove anything which matches any the regex wildcards
        unrecognised = [s for s in unrecognised if not any(wild_matcher.match(s) for wild_matcher in wildcard_matchers)]

        if len(unrecognised) > 0:
            err = "The following keys were not recognised:\n"
            err += "".join("{0} \n".format(k) for k in unrecognised)
            raise TomlValidationError(err)

    @staticmethod
    @abstractmethod
    def reference_schema():
        """
        Returns a dictionary layout of all supported keys
        :return: Dictionary containing all keys, and values set to None
        """
        pass

    @staticmethod
    def _build_nested_keys(d, path="", current_out=None):
        if not current_out:
            current_out = []

        def make_path(current_path, new_key):
            return current_path + "." + new_key if current_path else new_key

        for key, v in d.items():
            new_path = make_path(path, key)
            if isinstance(v, dict):
                # Recurse into dict
                current_out = TomlSchemaValidator._build_nested_keys(v, new_path, current_out)
            elif isinstance(v, set):
                # Pack all in from the set of names
                for name in v:
                    current_out.append(make_path(new_path, name))
            else:
                # This means its a value type with nothing special, so keep name
                current_out.append(new_path)

        return current_out
