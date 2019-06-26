# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Set of general purpose functions which are related to the SANSState approach."""

from __future__ import (absolute_import, division, print_function)

from sans.common.enums import (DetectorType)
from sans.common.xml_parsing import (get_monitor_names_from_idf_file, get_named_elements_from_ipf_file)


# ----------------------------------------------------------------------------------------------------------------------
# General functions
# ----------------------------------------------------------------------------------------------------------------------
def is_pure_none_or_not_none(elements_to_check):
    """
    Checks a list of elements contains None entries and non-None entries

    :param elements_to_check: a list with entries to check
    :return: True if the list contains either only None or only non-None elements, else False
    """
    are_all_none_or_all_not_none = True

    if len(elements_to_check) == 0:
        return are_all_none_or_all_not_none
    return all(element is not None for element in elements_to_check) or  \
           all(element is None for element in elements_to_check)  # noqa


def is_not_none_and_first_larger_than_second(elements_to_check):
    """
    This function checks if both are not none and then checks if the first element is smaller than the second element.

    :param elements_to_check: a list with two entries. The first is the lower bound and the second entry is the upper
                              bound
    :return: False if at least one input is None or if both are not None and the first element is smaller than the
             second else True
    """
    is_invalid = True
    if len(elements_to_check) != 2:
        return is_invalid
    if any(element is None for element in elements_to_check):
        is_invalid = False
        return is_invalid
    if elements_to_check[0] < elements_to_check[1]:
        is_invalid = False
    return is_invalid


def one_is_none(elements_to_check):
    return any(element is None for element in elements_to_check)


def validation_message(error_message, instruction, variables):
    """
    Generates a validation message for the SANSState.

    :param error_message: A message describing the error.
    :param instruction: A message describing what to do to fix the error
    :param variables: A dictionary which contains the variable names and values which are involved in the error.
    :return: a formatted validation message string.
    """
    message = ""
    for key, value in sorted(variables.items()):
        message += "{0}: {1}\n".format(key, value)
    message += instruction
    return {error_message: message}


def set_detector_names(state, ipf_path, invalid_detector_types=None):
    """
    Sets the detectors names on a State object which has a `detector` map entry, e.g. StateMask

    :param state: the state object
    :param ipf_path: the path to the Instrument Parameter File
    :param invalid_detector_types: a list of invalid detector types which don't exist for the instrument
    """
    if invalid_detector_types is None:
        invalid_detector_types = []

    lab_keyword = DetectorType.LAB.name
    hab_keyword = DetectorType.HAB.name
    detector_names = {lab_keyword: "low-angle-detector-name",
                      hab_keyword: "high-angle-detector-name"}
    detector_names_short = {lab_keyword: "low-angle-detector-short-name",
                            hab_keyword: "high-angle-detector-short-name"}

    names_to_search = []
    names_to_search.extend(list(detector_names.values()))
    names_to_search.extend(list(detector_names_short.values()))

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search, str)

    for detector_type in state.detectors:
        try:
            if DetectorType[detector_type] in invalid_detector_types:
                continue
            detector_name_tag = detector_names[detector_type]
            detector_name_short_tag = detector_names_short[detector_type]
            detector_name = found_detector_names[detector_name_tag]
            detector_name_short = found_detector_names[detector_name_short_tag]
        except KeyError:
            continue
        state.detectors[detector_type].detector_name = detector_name
        state.detectors[detector_type].detector_name_short = detector_name_short


def set_monitor_names(state, idf_path, invalid_monitor_names=None):
    if invalid_monitor_names is None:
        invalid_monitor_names = []
    monitor_names = get_monitor_names_from_idf_file(idf_path, invalid_monitor_names)
    state.monitor_names = monitor_names
