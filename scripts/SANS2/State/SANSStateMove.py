# pylint: disable=too-few-public-methods, too-many-instance-attributes

"""State for moving workspaces."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, FloatParameter, DictParameter, ClassTypeParameter,
                                       StringParameter, sans_parameters)
from SANS2.Common.SANSConstants import (SANSConstants)
from SANS2.Common.SANSType import (Coordinates, CanonicalCoordinates)
from SANS2.State.SANSStateFunctions import validation_message


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateMove(object):
    pass


# --------------------------------------------------------
#  SANSStateMoveWorkspace Setup for the different machines
# --------------------------------------------------------
@sans_parameters
class SANSStateMoveDetectorISIS(SANSStateBase, SANSStateMove):
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
        super(SANSStateMoveDetectorISIS, self).__init__()
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
            raise ValueError("SANSStateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@sans_parameters
class SANSStateMoveISIS(SANSStateBase, SANSStateMove):
    sample_offset = FloatParameter()
    sample_offset_direction = ClassTypeParameter(Coordinates)
    detectors = DictParameter()

    def __init__(self):
        super(SANSStateMoveISIS, self).__init__()

        # Setup the sample offset
        self.sample_offset = 0.0

        # The sample offset direction is Z for the ISIS instruments
        self.sample_offset_direction = CanonicalCoordinates.Z

        # Setup the detectors
        self.detectors = {SANSConstants.low_angle_bank: SANSStateMoveDetectorISIS(),
                          SANSConstants.high_angle_bank: SANSStateMoveDetectorISIS()}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        for key in self.detectors:
            self.detectors[key].validate()


@sans_parameters
class SANSStateMoveLOQ(SANSStateMoveISIS):
    monitor_names = DictParameter()
    center_position = FloatParameter()

    def __init__(self):
        super(SANSStateMoveLOQ, self).__init__()
        # Set the center_position in meter
        self.center_position = 317.5 / 1000.

        # Set the monitor names
        self.monitor_names = {}

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        super(SANSStateMoveLOQ, self).validate()


@sans_parameters
class SANSStateMoveSANS2D(SANSStateMoveISIS):
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
        super(SANSStateMoveSANS2D, self).__init__()
        # Set the descriptors which corresponds to information which we gain through the IPF
        self.hab_detector_radius = 306.0 /1000.
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
        super(SANSStateMoveSANS2D, self).validate()


@sans_parameters
class SANSStateMoveLARMOR(SANSStateMoveISIS):
    monitor_names = DictParameter()
    bench_rotation = FloatParameter()

    def __init__(self):
        super(SANSStateMoveLARMOR, self).__init__()

        # Set a default for the bench rotation
        self.bench_rotation = 0.0

        # Set the monitor names
        self.monitor_names = {}

    def validate(self):
        super(SANSStateMoveLARMOR, self).validate()


# -----------------------------------------------
# SANSStateMove setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateMove and SANSStateBase and fulfill its contract.
# -----------------------------------------------
