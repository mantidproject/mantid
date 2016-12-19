# pylint: disable=too-few-public-methods, too-many-instance-attributes

"""State for moving workspaces."""

import json
import copy

from sans.state.state_base import (StateBase, FloatParameter, DictParameter, ClassTypeParameter,
                                   StringParameter, rename_descriptor_names)
from sans.common.enums import (Coordinates, CanonicalCoordinates, SANSInstrument, DetectorType)
from sans.common.file_information import (get_instrument_paths_for_sans_file)
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
    detector_name = StringParameter()
    detector_name_short = StringParameter()

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
        if not self.detector_name:
            entry = validation_message("Missing detector name",
                                       "Make sure that a detector name was specified.",
                                       {"detector_name": self.detector_name})
            is_invalid.update(entry)
        if not self.detector_name_short:
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
    sample_offset_direction = ClassTypeParameter(Coordinates)
    detectors = DictParameter()

    def __init__(self):
        super(StateMove, self).__init__()

        # Setup the sample offset
        self.sample_offset = 0.0

        # The sample offset direction is Z for the ISIS instruments
        self.sample_offset_direction = CanonicalCoordinates.Z

        # Setup the detectors
        self.detectors = {DetectorType.to_string(DetectorType.LAB): StateMoveDetector(),
                          DetectorType.to_string(DetectorType.HAB): StateMoveDetector()}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        for key in self.detectors:
            self.detectors[key].validate()


@rename_descriptor_names
class StateMoveLOQ(StateMove):
    monitor_names = DictParameter()
    center_position = FloatParameter()

    def __init__(self):
        super(StateMoveLOQ, self).__init__()
        # Set the center_position in meter
        self.center_position = 317.5 / 1000.

        # Set the monitor names
        self.monitor_names = {}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        super(StateMoveLOQ, self).validate()


@rename_descriptor_names
class StateMoveSANS2D(StateMove):
    monitor_names = DictParameter()

    hab_detector_radius = FloatParameter()
    hab_detector_default_sd_m = FloatParameter()
    hab_detector_default_x_m = FloatParameter()

    lab_detector_default_sd_m = FloatParameter()

    hab_detector_x = FloatParameter()
    hab_detector_z = FloatParameter()

    hab_detector_rotation = FloatParameter()

    lab_detector_x = FloatParameter()
    lab_detector_z = FloatParameter()

    monitor_4_offset = FloatParameter()

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

        self.monitor_4_offset = 0.0

    def validate(self):
        super(StateMoveSANS2D, self).validate()


@rename_descriptor_names
class StateMoveLARMOR(StateMove):
    monitor_names = DictParameter()
    bench_rotation = FloatParameter()

    def __init__(self):
        super(StateMoveLARMOR, self).__init__()

        # Set a default for the bench rotation
        self.bench_rotation = 0.0

        # Set the monitor names
        self.monitor_names = {}

    def validate(self):
        super(StateMoveLARMOR, self).validate()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def setup_idf_and_ipf_content(move_info, data_info):
    # Get the IDF and IPF path since they contain most of the import information
    file_name = data_info.sample_scatter
    idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)
    # Set the detector names
    set_detector_names(move_info, ipf_path)
    # Set the monitor names
    set_monitor_names(move_info, idf_path)


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
        setup_idf_and_ipf_content(self.state, data_info)

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
        setup_idf_and_ipf_content(self.state, data_info)
        self.conversion_value = 1000.
        self._set_conversion_value(data_info)

    def _set_conversion_value(self, data_info):
        run_number = data_info.sample_scatter_run_number
        self.conversion_value = 1000. if run_number >= 2217 else 1.

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
    else:
        raise NotImplementedError("StateMoveBuilder: Could not find any valid move builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
