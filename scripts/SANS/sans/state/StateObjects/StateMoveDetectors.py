# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods, too-many-instance-attributes

"""State for moving workspaces."""

import copy
from typing import Dict

from SANS.sans.common.enums import CanonicalCoordinates, SANSInstrument, DetectorType
from SANS.sans.state.JsonSerializable import JsonSerializable
from SANS.sans.state.automatic_setters import automatic_setters


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StateMoveDetectors(metaclass=JsonSerializable):
    def __init__(self):
        super(StateMoveDetectors, self).__init__()
        # Translation correction
        self.x_translation_correction = 0.0  # : Float
        self.y_translation_correction = 0.0  # : Float
        self.z_translation_correction = 0.0  # : Float

        self.rotation_correction = 0.0  # : Float
        self.side_correction = 0.0  # : Float
        self.radius_correction = 0.0  # : Float

        self.x_tilt_correction = 0.0  # : Float
        self.y_tilt_correction = 0.0  # : Float
        self.z_tilt_correction = 0.0  # : Float

        # Sample centre Pos 1 + Pos 2
        self.sample_centre_pos1 = 0.0  # : Float
        self.sample_centre_pos2 = 0.0  # : Float


class StateMove(metaclass=JsonSerializable):
    def __init__(self):
        super(StateMove, self).__init__()

        self.sample_offset = 0.0  # : Float
        self.detectors: Dict[StateMoveDetectors] = {}

        # The sample offset direction is Z for the ISIS instruments
        self.sample_offset_direction = CanonicalCoordinates.Z
        self.monitor_4_offset: float = 0.0

    def validate(self):
        # If the detectors are empty, then we raise
        if not self.detectors:
            raise ValueError("No detectors have been set.")


class StateMoveLOQ(StateMove):
    def __init__(self):
        super(StateMoveLOQ, self).__init__()
        # Set the center_position in meter
        self.center_position = 317.5 / 1000.0  # : Float

        # Setup the detectors
        self.detectors = {DetectorType.LAB.value: StateMoveDetectors(), DetectorType.HAB.value: StateMoveDetectors()}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        super(StateMoveLOQ, self).validate()


class StateMoveSANS2D(StateMove):
    def __init__(self):
        super(StateMoveSANS2D, self).__init__()
        # Set the descriptors which corresponds to information which we gain through the IPF
        self.hab_detector_radius = 306.0 / 1000.0  # : Float
        self.hab_detector_default_sd_m = 4.0  # : Float
        self.hab_detector_default_x_m = 1.1  # : Float
        self.lab_detector_default_sd_m = 4.0  # : Float

        # The actual values are found on the workspace and should be used from there. This is only a fall back.
        self.hab_detector_x = 0.0  # : Float
        self.hab_detector_z = 0.0  # : Float
        self.hab_detector_rotation = 0.0  # : Float
        self.lab_detector_x = 0.0  # : Float
        self.lab_detector_z = 0.0  # : Float

        # Setup the detectors
        self.detectors = {DetectorType.LAB.value: StateMoveDetectors(), DetectorType.HAB.value: StateMoveDetectors()}

    def validate(self):
        super(StateMoveSANS2D, self).validate()


class StateMoveLARMOR(StateMove):
    def __init__(self):
        super(StateMoveLARMOR, self).__init__()

        # Set a default for the bench rotation
        self.bench_rotation = 0.0  # : Float

        # Setup the detectors
        self.detectors = {DetectorType.LAB.value: StateMoveDetectors()}

    def validate(self):
        super(StateMoveLARMOR, self).validate()


class StateMoveZOOM(StateMove):
    def __init__(self):
        super(StateMoveZOOM, self).__init__()
        self.lab_detector_default_sd_m = 0.0  # : Float
        self.monitor_5_offset: float = 0.0

        # Setup the detectors
        self.detectors = {DetectorType.LAB.value: StateMoveDetectors()}

    def validate(self):
        super(StateMoveZOOM, self).validate()


class StateMoveNoInst(StateMove):
    def __init__(self):
        super(StateMoveNoInst, self).__init__()
        self.lab_detector_default_sd_m = 0.0  # : Float

        # Setup the detectors
        self.detectors = {DetectorType.LAB.value: StateMoveDetectors(), DetectorType.HAB.value: StateMoveDetectors()}

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------


class StateMoveLOQBuilder(object):
    @automatic_setters(StateMoveLOQ, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self):
        super(StateMoveLOQBuilder, self).__init__()
        self.state = StateMoveLOQ()

    def build(self):
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.0

    def convert_pos2(self, value):
        return value / 1000.0


class StateMoveSANS2DBuilder(object):
    @automatic_setters(StateMoveSANS2D, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self):
        super(StateMoveSANS2DBuilder, self).__init__()
        self.state = StateMoveSANS2D()

    def build(self):
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.0

    def convert_pos2(self, value):
        return value / 1000.0


class StateMoveZOOMBuilder(object):
    @automatic_setters(StateMoveZOOM, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self):
        super(StateMoveZOOMBuilder, self).__init__()
        self.state = StateMoveZOOM()

    def build(self):
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.0

    def convert_pos2(self, value):
        return value / 1000.0


class StateMoveLARMORBuilder(object):
    @automatic_setters(StateMoveLARMOR, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(StateMoveLARMORBuilder, self).__init__()
        self.state = StateMoveLARMOR()

        self.conversion_value = 1000.0
        self._set_conversion_value(data_info)

    def _set_conversion_value(self, data_info):
        run_number = data_info.sample_scatter_run_number

        # If we are going through init before file was selected, lets assume were using
        # a newer file so in the probable-case the division is the same as Pos 2
        # When a run number is entered this re-runs anyway overwriting our assumptions
        # User files after 2217 use Si. units for x but not y
        if run_number is None or run_number > 2217:
            self.conversion_value = 1.0

    def build(self):
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / self.conversion_value

    def convert_pos2(self, value):
        return value / 1000.0


class StateMoveNoInstBuilder(object):
    @automatic_setters(StateMoveNoInst, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self):
        self.state = StateMoveNoInst()

    def build(self):
        return self.state

    def convert_pos1(self, value):
        return value / 1000.0

    def convert_pos2(self, value):
        return value / 1000.0


def get_move_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument

    if instrument is SANSInstrument.LOQ:
        return StateMoveLOQBuilder()
    elif instrument is SANSInstrument.SANS2D:
        return StateMoveSANS2DBuilder()
    elif instrument is SANSInstrument.LARMOR:
        return StateMoveLARMORBuilder(data_info)
    elif instrument is SANSInstrument.ZOOM:
        return StateMoveZOOMBuilder()
    elif instrument is SANSInstrument.NO_INSTRUMENT:
        return StateMoveNoInstBuilder()
    else:
        raise NotImplementedError(
            "StateMoveBuilder: Could not find any valid move builder for the " "specified StateData object {0}".format(str(data_info))
        )
