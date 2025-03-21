# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.common.enums import SANSInstrument, SANSFacility
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateData import StateData
from sans.test_helper.file_information_mock import SANSFileInformationMock


def get_mock_data_info():
    # TODO I really really dislike having to do this in a test, but
    # TODO de-coupling StateData is required to avoid it
    file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024)
    data_builder = get_data_builder(SANSFacility.ISIS, file_information)
    data_builder.set_sample_scatter("SANS2D00022024")
    data_builder.set_sample_scatter_period(3)
    return data_builder.build()


def setup_parser_dict(dict_vals) -> tuple[dict, StateData]:
    def _add_missing_mandatory_key(nested_dict_to_check: dict, key_path: list[str], replacement_val):
        _dict = nested_dict_to_check
        for key in key_path[0:-1]:
            if key not in _dict:
                _dict[key] = {}
            _dict = _dict[key]

        if key_path[-1] not in _dict:
            _dict[key_path[-1]] = replacement_val  # Add in child value
        return nested_dict_to_check

    mocked_data_info = get_mock_data_info()
    # instrument key needs to generally be present
    dict_vals = _add_missing_mandatory_key(dict_vals, ["instrument", "name"], "LOQ")
    dict_vals = _add_missing_mandatory_key(dict_vals, ["detector", "configuration", "selected_detector"], "rear")

    return dict_vals, mocked_data_info
