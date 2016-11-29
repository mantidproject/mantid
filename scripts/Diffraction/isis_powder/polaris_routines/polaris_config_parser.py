from __future__ import (absolute_import, division, print_function)

import yaml
import os


def get_basic_config(file_path):
    # At the moment we just return it without additional processing
    return _open_yaml_file(file_path=file_path)


def _open_yaml_file(file_path):

    if not os.path.isfile(file_path):
        raise ValueError("File not found at: " + str(file_path))

    read_config = None
    with open(file_path, 'r') as yaml_stream:
        try:
            read_config = yaml.load(yaml_stream)
        except yaml.YAMLError as exception:
            raise ValueError("Failed to parse YAML. Exception was:\n" + str(exception))

    return read_config
