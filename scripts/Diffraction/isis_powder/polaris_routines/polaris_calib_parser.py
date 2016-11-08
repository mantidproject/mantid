from __future__ import (absolute_import, division, print_function)

import os
import yaml


def get_calibration_dict(cycle):
    config_file = _open_yaml_file()
    try:
        output = config_file[str(cycle)]
    except KeyError:
        raise RuntimeError("Cycle " + str(cycle) + " not defined in calibration file")
    return output


def _open_yaml_file():
    config_file_name = "polaris_calibration.yaml"
    config_file_path = os.path.join(os.path.dirname(__file__), config_file_name)

    read_config = None

    with open(config_file_path, 'r') as input_stream:
        try:
            read_config = yaml.load(input_stream)
        except yaml.YAMLError as exception:
            print(exception)
            raise RuntimeError("Failed to parse POLARIS calibration YAML file")

    return read_config
