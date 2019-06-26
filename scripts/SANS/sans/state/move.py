# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods, too-many-instance-attributes

"""State for moving workspaces."""

from __future__ import (absolute_import, division, print_function)
import json
import copy

from sans.state.state_base import (StateBase, FloatParameter, DictParameter, EnumParameter,
                                   StringWithNoneParameter, rename_descriptor_names)
from sans.common.enums import (CanonicalCoordinates, SANSInstrument, DetectorType)
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import (validation_message, set_detector_names, set_monitor_names)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateMoveDetector(StateBase):
    x_translation_correction = FloatParameter()
    y_translation_correction = FloatParameter()
    z_translation_correction = FloatParameter()

    rotation_correction = FloatParameter()
    side_correction = FloatParameter()
    radius_correction = FloatParameter()

    x_tilt_correction = FloatParameter()
    y_tilt_correction = FloatParameter()
    z_tilt_correction = FloatParameter()

    sample_centre_pos1 = FloatParameter()
    sample_centre_pos2 = FloatParameter()

    # Name of the detector
    detector_name = StringWithNoneParameter()
    detector_name_short = StringWithNoneParameter()

    def __init__(self):
        super(StateMoveDetector, self).__init__()
        # Translation correction
        self.x_translation_correction = 0.0
        self.y_translation_correction = 0.0
        self.z_translation_correction = 0.0

        self.rotation_correction = 0.0
        self.side_correction = 0.0
        self.radius_correction = 0.0

        self.x_tilt_correction = 0.0
        self.y_tilt_correction = 0.0
        self.z_tilt_correction = 0.0

        # Sample centre Pos 1 + Pos 2
        self.sample_centre_pos1 = 0.0
        self.sample_centre_pos2 = 0.0

    def validate(self):
        is_invalid = {}
        if self.detector_name == "" or self.detector_name is None:
            entry = validation_message("Missing detector name",
                                       "Make sure that a detector name was specified.",
                                       {"detector_name": self.detector_name})
            is_invalid.update(entry)
        if self.detector_name_short == "" or self.detector_name_short is None:
            entry = validation_message("Missing short detector name",
                                       "Make sure that a short detector name was specified.",
                                       {"detector_name_short": self.detector_name_short})
            is_invalid.update(entry)
        if is_invalid:
            raise ValueError("StateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@rename_descriptor_names
class StateMove(StateBase):
    sample_offset = FloatParameter()
    sample_offset_direction = EnumParameter(CanonicalCoordinates)
    detectors = DictParameter()
    monitor_names = DictParameter()

    def __init__(self):
        super(StateMove, self).__init__()

        # Setup the sample offset
        self.sample_offset = 0.0

        # The sample offset direction is Z for the ISIS instruments
        self.sample_offset_direction = CanonicalCoordinates.Z

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        for key in self.detectors:
            self.detectors[key].validate()

        # If the detectors are empty, then we raise
        if not self.detectors:
            raise ValueError("No detectors have been set.")


@rename_descriptor_names
class StateMoveLOQ(StateMove):
    center_position = FloatParameter()

    def __init__(self):
        super(StateMoveLOQ, self).__init__()
        # Set the center_position in meter
        self.center_position = 317.5 / 1000.

        # Set the monitor names
        self.monitor_names = {}

        # Setup the detectors
        self.detectors = {DetectorType.LAB.name: StateMoveDetector(),
                          DetectorType.HAB.name: StateMoveDetector()}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        super(StateMoveLOQ, self).validate()


@rename_descriptor_names
class StateMoveSANS2D(StateMove):
    hab_detector_radius = FloatParameter()
    hab_detector_default_sd_m = FloatParameter()
    hab_detector_default_x_m = FloatParameter()

    lab_detector_default_sd_m = FloatParameter()

    hab_detector_x = FloatParameter()
    hab_detector_z = FloatParameter()

    hab_detector_rotation = FloatParameter()

    lab_detector_x = FloatParameter()
    lab_detector_z = FloatParameter()

    monitor_n_offset = FloatParameter()

    def __init__(self):
        super(StateMoveSANS2D, self).__init__()
        # Set the descriptors which corresponds to information which we gain through the IPF
        self.hab_detector_radius = 306.0 / 1000.
        self.hab_detector_default_sd_m = 4.0
        self.hab_detector_default_x_m = 1.1
        self.lab_detector_default_sd_m = 4.0

        # The actual values are found on the workspace and should be used from there. This is only a fall back.
        self.hab_detector_x = 0.0
        self.hab_detector_z = 0.0
        self.hab_detector_rotation = 0.0
        self.lab_detector_x = 0.0
        self.lab_detector_z = 0.0

        # Set the monitor names
        self.monitor_names = {}

        self.monitor_n_offset = 0.0

        # Setup the detectors
        self.detectors = {DetectorType.LAB.name: StateMoveDetector(),
                          DetectorType.HAB.name: StateMoveDetector()}

    def validate(self):
        super(StateMoveSANS2D, self).validate()


@rename_descriptor_names
class StateMoveLARMOR(StateMove):
    bench_rotation = FloatParameter()

    def __init__(self):
        super(StateMoveLARMOR, self).__init__()

        # Set a default for the bench rotation
        self.bench_rotation = 0.0

        # Set the monitor names
        self.monitor_names = {}

        # Setup the detectors
        self.detectors = {DetectorType.LAB.name: StateMoveDetector()}

    def validate(self):
        super(StateMoveLARMOR, self).validate()


@rename_descriptor_names
class StateMoveZOOM(StateMove):
    lab_detector_default_sd_m = FloatParameter()
    monitor_n_offset = FloatParameter()

    def __init__(self):
        super(StateMoveZOOM, self).__init__()
        self.lab_detector_default_sd_m = 0.0

        # Set the monitor names
        self.monitor_names = {}
        self.monitor_n_offset = 0.0

        # Setup the detectors
        self.detectors = {DetectorType.LAB.name: StateMoveDetector()}

    def validate(self):
        super(StateMoveZOOM, self).validate()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def setup_idf_and_ipf_content(move_info, data_info, invalid_detector_types=None, invalid_monitor_names=None):
    # Get the IDF and IPF path since they contain most of the import information
    idf_file_path = data_info.idf_file_path
    ipf_file_path = data_info.ipf_file_path

    # Set the detector names
    set_detector_names(move_info, ipf_file_path, invalid_detector_types)
    # Set the monitor names
    set_monitor_names(move_info, idf_file_path, invalid_monitor_names)


class StateMoveLOQBuilder(object):
    @automatic_setters(StateMoveLOQ, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(StateMoveLOQBuilder, self).__init__()
        self.state = StateMoveLOQ()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.

    def convert_pos2(self, value):
        return value / 1000.


class StateMoveSANS2DBuilder(object):
    @automatic_setters(StateMoveSANS2D, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(StateMoveSANS2DBuilder, self).__init__()
        self.state = StateMoveSANS2D()
        # TODO: At the moment we set the monitor names up manually here. In principle we have all necessary information
        #       in the IDF we should be able to parse it and get.
        invalid_monitor_names = ["monitor5", "monitor6", "monitor7", "monitor8"]
        setup_idf_and_ipf_content(self.state, data_info, invalid_monitor_names=invalid_monitor_names)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.

    def convert_pos2(self, value):
        return value / 1000.


class StateMoveZOOMBuilder(object):
    @automatic_setters(StateMoveZOOM, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(StateMoveZOOMBuilder, self).__init__()
        self.state = StateMoveZOOM()
        # TODO: At the moment we set the monitor names up manually here. In principle we have all necessary information
        #       in the IDF we should be able to parse it and get.
        invalid_monitor_names = ["monitor6", "monitor7", "monitor8", "monitor9", "monitor10"]
        invalid_detector_types = [DetectorType.HAB]
        setup_idf_and_ipf_content(self.state, data_info,
                                  invalid_detector_types=invalid_detector_types,
                                  invalid_monitor_names=invalid_monitor_names)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.

    def convert_pos2(self, value):
        return value / 1000.


class StateMoveLARMORBuilder(object):
    @automatic_setters(StateMoveLARMOR, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(StateMoveLARMORBuilder, self).__init__()
        self.state = StateMoveLARMOR()
        # There are several invalid monitor names which are not setup for LARMOR, also the IPF has a high-angle-bank
        # but this is not setup for LARMOR
        # TODO: At the moment we set the monitor names up manually here. In principle we have all necessary information
        #       in the IDF we should be able to parse it and get.
        invalid_monitor_names = ["monitor6", "monitor7", "monitor8", "monitor9", "monitor10"]
        invalid_detector_types = [DetectorType.HAB]
        setup_idf_and_ipf_content(self.state, data_info,
                                  invalid_detector_types=invalid_detector_types,
                                  invalid_monitor_names=invalid_monitor_names)
        self.conversion_value = 1000.
        self._set_conversion_value(data_info)

    def _set_conversion_value(self, data_info):
        run_number = data_info.sample_scatter_run_number
        self.conversion_value = 1000. if run_number <= 2217 else 1.

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / self.conversion_value

    def convert_pos2(self, value):
        return value / 1000.


def get_move_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LOQ:
        return StateMoveLOQBuilder(data_info)
    elif instrument is SANSInstrument.SANS2D:
        return StateMoveSANS2DBuilder(data_info)
    elif instrument is SANSInstrument.LARMOR:
        return StateMoveLARMORBuilder(data_info)
    elif instrument is SANSInstrument.ZOOM:
        return StateMoveZOOMBuilder(data_info)
    else:
        raise NotImplementedError("StateMoveBuilder: Could not find any valid move builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
