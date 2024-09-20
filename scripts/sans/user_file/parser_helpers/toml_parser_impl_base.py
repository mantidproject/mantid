# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, List


class MissingMandatoryParam(KeyError):
    # Ensure the module path doesn't print which is very noisy:
    # https://stackoverflow.com/a/19419825
    __module__ = Exception.__module__


class TomlParserImplBase:
    def __init__(self, toml_dict: Dict):
        self._input = toml_dict

    def get_mandatory_val(self, keys: List):
        """
        Gets a mandatory key, the full path is required to ensure the error message is
        valid. Raises a MissingMandatoryParam if the key is not found with
        the full path.
        :param keys: A list of keys to iterate through
        """
        keys = [str(i) for i in keys]
        key_path = ".".join(keys)
        exception = MissingMandatoryParam(f"The following key is required but missing: {key_path}")
        # Don't use None in case user entered None themselves
        val = self.get_val(keys=keys, default=exception)

        if val == exception:
            raise exception  # Not found
        return val

    def get_val(self, keys, dict_to_parse=None, default=None):
        """
        Gets a nested value within the specified dictionary
        :param keys: A list of keys to iterate through the dictionary
        :param dict_to_parse: (Optional) The dict to parse, if None parses the input dict
        :param default: The default value to use if the given key was not found. If not provided None is used.
        :return: The corresponding value
        """
        if isinstance(keys, str):
            keys = [keys]

        try:
            return self._get_val_impl(keys=keys, dict_to_parse=dict_to_parse)
        except KeyError:
            return default

    def _get_val_impl(self, keys, dict_to_parse):
        if dict_to_parse is None:
            dict_to_parse = self._input

        assert isinstance(dict_to_parse, dict), "Expected a dict for get keys, got %r instead" % repr(dict_to_parse)

        val = dict_to_parse[keys[0]]
        if isinstance(val, dict) and len(keys) > 1:
            return self._get_val_impl(keys=keys[1:], dict_to_parse=val)
        return val
