# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict

from sans.common.enums import SANSInstrument, DetectorType
from sans.common.xml_parsing import get_monitor_names_from_idf_file, get_named_elements_from_ipf_file
from sans.state.JsonSerializable import JsonSerializable
from mantid.py36compat import dataclass
from sans.state.StateObjects.StateData import StateData


@dataclass()
class DetectorNames(metaclass=JsonSerializable):
    detector_name: str = None
    detector_name_short: str = None


class StateInstrumentInfo(metaclass=JsonSerializable):
    """
    Holds instrument state that is found in the IDF / IPF files. This was separated
    out from the other state objects as we need to know our run number to look up
    the correct IPF/IDF. However to load in a run number we need the other instrument
    fields of state info filled. This helps break that circular dependency.
    """

    @staticmethod
    def build_from_data_info(data_info):
        inst_info = StateInstrumentInfo()
        _init_detector_names(inst_info=inst_info, data_info=data_info)
        _init_monitor_names(inst_info=inst_info, data_info=data_info)

        inst_info.idf_path = data_info.idf_file_path
        return inst_info

    def __init__(self):
        self.detector_names = {DetectorType.LAB.value: DetectorNames(), DetectorType.HAB.value: DetectorNames()}
        self.monitor_names: Dict[str, str] = dict()
        self.idf_path: str = ""


def _get_invalid_monitor_names(instrument: SANSInstrument):
    if instrument is SANSInstrument.LARMOR:
        return ["monitor6", "monitor7", "monitor8", "monitor9", "monitor10"]
    elif instrument is SANSInstrument.SANS2D:
        return ["monitor5", "monitor6", "monitor7", "monitor8"]
    elif instrument is SANSInstrument.ZOOM:
        return ["monitor6", "monitor7", "monitor8", "monitor9", "monitor10"]
    return None


def _get_invalid_monitor_types(instrument: SANSInstrument):
    if instrument in (SANSInstrument.LARMOR, SANSInstrument.ZOOM):
        return [DetectorType.HAB]


def _set_detector_names(state: StateInstrumentInfo, ipf_path, invalid_detector_types=None):
    """
    Sets the detectors names on a State object which has a `detector` map entry, e.g. StateMask

    :param state: the state object
    :param ipf_path: the path to the Instrument Parameter File
    :param invalid_detector_types: a list of invalid detector types which don't exist for the instrument
    """
    if invalid_detector_types is None:
        invalid_detector_types = []

    lab_keyword = DetectorType.LAB.value
    hab_keyword = DetectorType.HAB.value
    detector_names = {lab_keyword: "low-angle-detector-name", hab_keyword: "high-angle-detector-name"}
    detector_names_short = {lab_keyword: "low-angle-detector-short-name", hab_keyword: "high-angle-detector-short-name"}

    names_to_search = []
    names_to_search.extend(list(detector_names.values()))
    names_to_search.extend(list(detector_names_short.values()))

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search, str)

    for detector_type in state.detector_names:
        try:
            if DetectorType(detector_type) in invalid_detector_types:
                continue
            detector_name_tag = detector_names[detector_type]
            detector_name_short_tag = detector_names_short[detector_type]
            detector_name = found_detector_names[detector_name_tag]
            detector_name_short = found_detector_names[detector_name_short_tag]
        except KeyError:
            continue
        state.detector_names[detector_type].detector_name = detector_name
        state.detector_names[detector_type].detector_name_short = detector_name_short


def _set_monitor_names(inst_info_state, idf_path, invalid_monitor_names=None):
    if invalid_monitor_names is None:
        invalid_monitor_names = []
    monitor_names = get_monitor_names_from_idf_file(idf_path, invalid_monitor_names)
    inst_info_state.monitor_names = monitor_names


def _init_detector_names(inst_info: StateInstrumentInfo, data_info: StateData):
    ipf_file_path = data_info.ipf_file_path
    if ipf_file_path:
        invalid_detector_types = _get_invalid_monitor_types(data_info.instrument)
        _set_detector_names(inst_info, ipf_file_path, invalid_detector_types=invalid_detector_types)


def _init_monitor_names(inst_info: StateInstrumentInfo, data_info: StateData):
    # Get the IDF and IPF path since they contain most of the import information
    idf_file_path = data_info.idf_file_path

    # Set the monitor names
    if idf_file_path:
        invalid_monitors = _get_invalid_monitor_names(data_info.instrument)
        _set_monitor_names(inst_info, idf_file_path, invalid_monitor_names=invalid_monitors)
