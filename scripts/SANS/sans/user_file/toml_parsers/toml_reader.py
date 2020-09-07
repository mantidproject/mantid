# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import toml


class TomlReader(object):
    @staticmethod
    def get_user_file_dict(toml_file_path):
        try:
            return toml.load(toml_file_path)
        except toml.TomlDecodeError as e:
            # Convert into a std exception type so we don't need
            # TOML lib in outer code
            raise ValueError(e)
