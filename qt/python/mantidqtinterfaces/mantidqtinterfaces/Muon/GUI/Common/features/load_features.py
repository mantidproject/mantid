# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import config


def load_features():
    feature_list = {}
    # this allows for a mangled config
    try:
        feature_list=attempt_load()
    except ValueError:
        return {}
    return feature_list


def get_muon_GUI_config():
    return config["muon.GUI"]


def attempt_load():
    feature_list = {}
    string = get_muon_GUI_config()
    if string:
        item_list = string.split(",")
        for item in item_list:
            key, value = item.split(":")
            feature_list[key.replace(" ","")] = int(value)
    return feature_list
