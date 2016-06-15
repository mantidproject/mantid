import json
from SANSStateBase import (SANSStateBase, FloatParameter, DictParameter, ClassTypeParameter, StringParameter,
                           PositiveFloatParameter, convert_state_to_dict, set_state_from_property_manager,
                           sans_parameters)
from Common.SANSConstants import (Coordinates, CanonicalCoordinates, SANSConstants)


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateMoveWorkspace(object):
    pass


# --------------------------------------------------------
#  SANSStateMoveWorkspace Setup for the different machines
# --------------------------------------------------------
@sans_parameters
class SANSStateMoveWorkspaceDetectorISIS(SANSStateBase, SANSStateMoveWorkspace):
    x_translation_correction = FloatParameter()
    y_translation_correction = FloatParameter()
    z_translation_correction = FloatParameter()

    # They signify a rotation around the x, y and z direction
    x_rotation_correction = FloatParameter()
    y_rotation_correction = FloatParameter()
    z_rotation_correction = FloatParameter()

    x_tilt_correction = FloatParameter()
    y_tilt_correction = FloatParameter()
    z_tilt_correction = FloatParameter()

    # Name of the detector
    detector_name = StringParameter()
    detector_name_short = StringParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceDetectorISIS, self).__init__()
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

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        is_invalid = {}
        if not self.detector_name:
            is_invalid.update({"detector_name": "The detector name has not been specified."})
        if not self.detector_name_short:
            is_invalid.update({"detector_name_short": "The short detector name has not been specified."})
        if is_invalid:
            raise ValueError("SANSStateMoveWorkspaceDetectorISIS: The provided inputs are illegal. "
                             "Please see: {}".format(json.dumps(is_invalid)))


@sans_parameters
class SANSStateMoveWorkspaceISIS(SANSStateBase, SANSStateMoveWorkspace):
    sample_offset = FloatParameter()
    sample_offset_direction = ClassTypeParameter(Coordinates)
    detectors = DictParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceISIS, self).__init__()

        # Setup the sample offset
        self.sample_offset = 0.0

        # The sample offset direction is Z for the ISIS instruments
        self.sample_offset_direction = CanonicalCoordinates.Z

        # Setup the detectors
        self.detectors = {SANSConstants.low_angle_bank: SANSStateMoveWorkspaceDetectorISIS(),
                          SANSConstants.high_angle_bank: SANSStateMoveWorkspaceDetectorISIS()}

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        for key in self.detectors:
            self.detectors[key].validate()


@sans_parameters
class SANSStateMoveWorkspaceLOQ(SANSStateMoveWorkspaceISIS):
    monitor_names = DictParameter()
    center_position = FloatParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceLOQ, self).__init__()
        # Set the monitor names
        # TODO set this dynamically, based on IDF
        self.monitor_names = {1: 'monitor1',
                              2: 'monitor2',
                              3: 'monitor3',
                              4: 'monitor4'}

        # Set the center_position in meter
        self.center_position = 317.5 / 1000.

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        # No validation of the descriptors on this level, let potential exceptions from detectors "bubble" up
        super(SANSStateMoveWorkspaceLOQ, self).validate()


@sans_parameters
class SANSStateMoveWorkspaceSANS2D(SANSStateMoveWorkspaceISIS):
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
        super(SANSStateMoveWorkspaceSANS2D, self).__init__()
        # Set the descriptors which corresponds to information which we gain through the IPF
        self.hab_detector_radius = 306.0
        self.hab_detector_default_sd_m = 4.0
        self.hab_detector_default_x_m = 1.1
        self.lab_detector_default_sd_m = 4.0
        self.hab_detector_x = 0.0
        self.hab_detector_z = 0.0
        self.hab_detector_rotation = 0.0
        self.lab_detector_x = 0.0
        self.lab_detector_z = 0.0

        # Set the monitor names
        # TODO set this dynamically, based on IDF
        self.monitor_names = {1: 'monitor1',
                              2: 'monitor2',
                              3: 'monitor3',
                              4: 'monitor4'}

        self.monitor_4_offset = 0.0

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        super(SANSStateMoveWorkspaceSANS2D, self).validate()


@sans_parameters
class SANSStateMoveWorkspaceLARMOR(SANSStateMoveWorkspaceISIS):
    monitor_names = DictParameter()
    bench_rotation = FloatParameter()
    coord1_scale_factor = PositiveFloatParameter()

    def __init__(self):
        super(SANSStateMoveWorkspaceLARMOR, self).__init__()

        # Set the monitor names
        # TODO set this dynamically, based on IDF
        self.monitor_names = {1: 'monitor1',
                              2: 'monitor2',
                              3: 'monitor3',
                              4: 'monitor4'}

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        super(SANSStateMoveWorkspaceLARMOR, self).validate()
        is_invalid = {}
        if not self.coord1_scale_factor:
            is_invalid.update({"coord1_scale_factor": "The scale factor for coordinate 1 has not been specified."})
        if is_invalid:
            raise ValueError("SANSStateMoveWorkspaceLARMOR: The provided inputs are illegal. "
                             "Please see: {}".format(json.dumps(is_invalid)))

# -----------------------------------------------
# SANSStateData setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateData and SANSStateBase and fulfill its contract.
# -----------------------------------------------