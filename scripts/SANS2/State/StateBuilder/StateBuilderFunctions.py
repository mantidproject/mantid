# pylint: disable=invalid-name, too-many-arguments
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.XMLParsing import (get_monitor_names_from_idf_file, get_named_elements_from_ipf_file)


# ---------------------------------------------------
#  Other free functions
# ---------------------------------------------------
def set_detector_names(info, ipf_path):
    detector_names = {SANSConstants.low_angle_bank: "low-angle-detector-name",
                      SANSConstants.high_angle_bank: "high-angle-detector-name"}
    detector_names_short = {SANSConstants.low_angle_bank: "low-angle-detector-short-name",
                            SANSConstants.high_angle_bank: "high-angle-detector-short-name"}

    names_to_search = []
    names_to_search.extend(detector_names.values())
    names_to_search.extend(detector_names_short.values())

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search, str)

    for detector_type in info.detectors:
        try:
            detector_name_tag = detector_names[detector_type]
            detector_name_short_tag = detector_names_short[detector_type]
            detector_name = found_detector_names[detector_name_tag]
            detector_name_short = found_detector_names[detector_name_short_tag]
        except KeyError:
            continue

        info.detectors[detector_type].detector_name = detector_name
        info.detectors[detector_type].detector_name_short = detector_name_short


def set_monitor_names(move_info, idf_path):
    monitor_names = get_monitor_names_from_idf_file(idf_path)
    move_info.monitor_names = monitor_names
