# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods, invalid-name

from __future__ import (absolute_import, division, print_function)
import math
from mantid.api import MatrixWorkspace
from six import with_metaclass
from abc import (ABCMeta, abstractmethod)
from sans.state.move import StateMove
from sans.common.enums import (SANSInstrument, CanonicalCoordinates, DetectorType)
from sans.common.general_functions import (create_unmanaged_algorithm, get_single_valued_logs_from_workspace,
                                           quaternion_to_angle_and_axis, sanitise_instrument_name)


# -------------------------------------------------
# Free functions
# -------------------------------------------------
def move_component(workspace, offsets, component_to_move, is_relative=True):
    """
    Move an individual component on a workspace

    :param workspace: the workspace which the component which is to be moved.
    :param offsets: a Coordinate vs. Value map of offsets.
    :param component_to_move: the name of a component on the instrument. This component must be name which exist.
                              on the instrument.
    :param is_relative: if the move is relative of not.
    :return:
    """
    move_name = "MoveInstrumentComponent"
    move_options = {"Workspace": workspace,
                    "ComponentName": component_to_move,
                    "RelativePosition": is_relative}
    for key, value in list(offsets.items()):
        if key is CanonicalCoordinates.X:
            move_options.update({"X": value})
        elif key is CanonicalCoordinates.Y:
            move_options.update({"Y": value})
        elif key is CanonicalCoordinates.Z:
            move_options.update({"Z": value})
        else:
            raise RuntimeError("SANSMove: Trying to move the components along an unknown direction. "
                               "See here: {0}".format(str(component_to_move)))
    alg = create_unmanaged_algorithm(move_name, **move_options)
    alg.execute()


def rotate_component(workspace, angle, direction, component_to_rotate):
    """
    Rotate a component on a workspace.

    :param workspace: the workspace which contains the component which is to be rotated.
    :param angle: the angle by which it is to be rotated in degrees.
    :param direction: the rotation direction. This is a unit vector encoded as a Coordinate vs Value map.
    :param component_to_rotate: name of the component which is to be rotated
    :return:
    """
    rotate_name = "RotateInstrumentComponent"
    rotate_options = {"Workspace": workspace,
                      "ComponentName": component_to_rotate,
                      "RelativeRotation": "1"}
    for key, value in list(direction.items()):
        if key is CanonicalCoordinates.X:
            rotate_options.update({"X": value})
        elif key is CanonicalCoordinates.Y:
            rotate_options.update({"Y": value})
        elif key is CanonicalCoordinates.Z:
            rotate_options.update({"Z": value})
        else:
            raise RuntimeError("SANSMove: Trying to rotate the components along an unknown direction. "
                               "See here: {0}".format(str(component_to_rotate)))
    rotate_options.update({"Angle": angle})
    alg = create_unmanaged_algorithm(rotate_name, **rotate_options)
    alg.execute()


def move_sample_holder(workspace, sample_offset, sample_offset_direction):
    """
    Moves the sample holder by specified amount.

    :param workspace: the workspace which will have its sample holder moved.
    :param sample_offset: the offset value
    :param sample_offset_direction: the offset direction (can only be currently along a canonical direction)
    """
    offset = {sample_offset_direction: sample_offset}
    move_component(workspace, offset, 'some-sample-holder')


def apply_standard_displacement(move_info, workspace, coordinates, component):
    """
    Applies a standard displacement to a workspace.

    A standard displacement means here that it is a displacement along X and Y since Z is normally the direction
    of the beam.
    :param move_info: a StateMove object.
    :param workspace: the workspace which is to moved.
    :param coordinates: a list of coordinates by how much to move the component on the workspace. Note that currently
                        only the first two entries are used.
    :param component: the component which is to be moved.
    """
    # Get the detector name
    component_name = move_info.detectors[component].detector_name
    # Offset
    offset = {CanonicalCoordinates.X: coordinates[0],
              CanonicalCoordinates.Y: coordinates[1]}
    move_component(workspace, offset, component_name)


def is_zero_axis(axis):
    """
    Checks if the axis is potentially a null vector and hence not a useful axis at all.

    :param axis: the axis to check.
    :return: true if the axis is a null vector (or close to it) else false.
    """
    total_value = 0.0
    for entry in axis:
        total_value += abs(entry)
    return total_value < 1e-4


def set_selected_components_to_original_position(workspace, component_names):
    """
    Sets a component to its original (non-moved) position, i.e. the position one obtains when using standard loading.

    The way we know the original position/rotation is the base instrument. We look at the difference between the
    instrument and the base instrument and perform a reverse operation.

    :param workspace: the workspace which will have the move applied to it.
    :param component_names: the name of the component which is to be moved.
    """
    # First get the original rotation and position of the unaltered instrument components. This information
    # is stored in the base instrument
    instrument = workspace.getInstrument()
    base_instrument = instrument.getBaseInstrument()

    # Get the original position and rotation
    for component_name in component_names:
        base_component = base_instrument.getComponentByName(component_name)
        moved_component = instrument.getComponentByName(component_name)

        # It can be that monitors are already defined in the IDF but they cannot be found on the workspace. They
        # are buffer monitor names which the experiments might use in the future. Hence we need to check if a component
        # is zero at this point
        if base_component is None or moved_component is None:
            continue

        base_position = base_component.getPos()
        base_rotation = base_component.getRotation()

        moved_position = moved_component.getPos()
        moved_rotation = moved_component.getRotation()

        move_alg = None
        if base_position != moved_position:
            if move_alg is None:
                move_alg_name = "MoveInstrumentComponent"
                move_alg_options = {"Workspace": workspace,
                                    "RelativePosition": False,
                                    "ComponentName": component_name,
                                    "X": base_position[0],
                                    "Y": base_position[1],
                                    "Z": base_position[2]}
                move_alg = create_unmanaged_algorithm(move_alg_name, **move_alg_options)
            else:
                move_alg.setProperty("ComponentName", component_name)
                move_alg.setProperty("X", base_position[0])
                move_alg.setProperty("Y", base_position[1])
                move_alg.setProperty("Z", base_position[2])
            move_alg.execute()

        rot_alg = None
        if base_rotation != moved_rotation:
            angle, axis = quaternion_to_angle_and_axis(moved_rotation)
            # If the axis is a zero vector then, we continue, there is nothing to rotate
            if not is_zero_axis(axis):
                if rot_alg is None:
                    rot_alg_name = "RotateInstrumentComponent"
                    rot_alg_options = {"Workspace": workspace,
                                       "RelativeRotation": True,
                                       "ComponentName": component_name,
                                       "X": axis[0],
                                       "Y": axis[1],
                                       "Z": axis[2],
                                       "Angle": -1*angle}
                    rot_alg = create_unmanaged_algorithm(rot_alg_name, **rot_alg_options)
                else:
                    rot_alg.setProperty("ComponentName", component_name)
                    rot_alg.setProperty("X", axis[0])
                    rot_alg.setProperty("Y", axis[1])
                    rot_alg.setProperty("Z", axis[2])
                    rot_alg.setProperty("Angle", -1*angle)
                rot_alg.execute()


def set_components_to_original_for_isis(move_info, workspace, component):
    """
    This function resets the components for ISIS instruments. These are normally HAB, LAB, the monitors and
    the sample holder

    :param move_info: a StateMove object.
    :param workspace: the workspace which is being reset.
    :param component: the component which is being reset on the workspace. If this is not specified, then
                      everything is being reset.
    """
    def _reset_detector(_key, _move_info, _component_names):
        if _key in _move_info.detectors:
            _detector_name = _move_info.detectors[_key].detector_name
            if _detector_name:
                _component_names.append(_detector_name)

    # We reset the HAB, the LAB, the sample holder and monitor 4
    if not component:
        component_names = list(move_info.monitor_names.values())

        hab_key = DetectorType.to_string(DetectorType.HAB)
        _reset_detector(hab_key, move_info, component_names)

        lab_key = DetectorType.to_string(DetectorType.LAB)
        _reset_detector(lab_key, move_info, component_names)

        component_names.append("some-sample-holder")
    else:
        component_names = [component]

    # We also want to check the sample holder
    set_selected_components_to_original_position(workspace, component_names)


def get_detector_component(move_info, component):
    """
    Gets the detector component on the workspace

    :param move_info: a StateMove object.
    :param component: A component name, ie a detector name or a short detector name as specified in the IPF.
    :return: the key entry for detectors on the StateMove object which corresponds to the input component.
    """
    component_selection = component
    if component:
        for detector_key in list(move_info.detectors.keys()):
            is_name = component == move_info.detectors[detector_key].detector_name
            is_name_short = component == move_info.detectors[detector_key].detector_name_short
            if is_name or is_name_short:
                component_selection = detector_key
    return component_selection


def move_low_angle_bank_for_SANS2D_and_ZOOM(move_info, workspace, coordinates, use_rear_det_z=True):
    # REAR_DET_Z
    if use_rear_det_z:
        lab_detector_z_tag = "Rear_Det_Z"

        log_names = [lab_detector_z_tag]
        log_types = [float]
        log_values = get_single_valued_logs_from_workspace(workspace, log_names, log_types,
                                                           convert_from_millimeter_to_meter=True)

        lab_detector_z = move_info.lab_detector_z \
            if log_values[lab_detector_z_tag] is None else log_values[lab_detector_z_tag]
    else:
        lab_detector_z = 0.

    # Perform x and y tilt
    lab_detector = move_info.detectors[DetectorType.to_string(DetectorType.LAB)]
    SANSMoveSANS2D.perform_x_and_y_tilts(workspace, lab_detector)

    lab_detector_default_sd_m = move_info.lab_detector_default_sd_m
    x_shift = -coordinates[0]
    y_shift = -coordinates[1]

    z_shift = (lab_detector_z + lab_detector.z_translation_correction) - lab_detector_default_sd_m
    detector_name = lab_detector.detector_name
    offset = {CanonicalCoordinates.X: x_shift,
              CanonicalCoordinates.Y: y_shift,
              CanonicalCoordinates.Z: z_shift}
    move_component(workspace, offset, detector_name)


# -------------------------------------------------
# Move classes
# -------------------------------------------------
class SANSMove(with_metaclass(ABCMeta, object)):
    def __init__(self):
        super(SANSMove, self).__init__()

    @abstractmethod
    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        pass

    @abstractmethod
    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        pass

    @abstractmethod
    def do_set_to_zero(self, move_info, workspace, component):
        pass

    @staticmethod
    @abstractmethod
    def is_correct(instrument_type, run_number, **kwargs):
        pass

    def move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        SANSMove._validate(move_info, workspace, coordinates, component)
        component_selection = get_detector_component(move_info, component)
        return self.do_move_initial(move_info, workspace, coordinates, component_selection, is_transmission_workspace)

    def move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        SANSMove._validate(move_info, workspace, coordinates, component)
        component_selection = get_detector_component(move_info, component)
        return self.do_move_with_elementary_displacement(move_info, workspace, coordinates, component_selection)

    def set_to_zero(self, move_info, workspace, component):
        SANSMove._validate_set_to_zero(move_info, workspace, component)
        return self.do_set_to_zero(move_info, workspace, component)

    @staticmethod
    def _validate_component(move_info, component):
        if component is not None and len(component) != 0:
            found_name = False
            for detector_keys in list(move_info.detectors.keys()):
                is_name = component == move_info.detectors[detector_keys].detector_name
                is_name_short = component == move_info.detectors[detector_keys].detector_name_short
                if is_name or is_name_short:
                    found_name = True
                    break
            if not found_name:
                raise ValueError("SANSMove: The component to be moved {0} cannot be found in the"
                                 " state information of type {1}".format(str(component), str(type(move_info))))

    @staticmethod
    def _validate_workspace(workspace):
        if not isinstance(workspace, MatrixWorkspace):
            raise ValueError("SANSMove: The input workspace has to be a MatrixWorkspace")

    @staticmethod
    def _validate_state(move_info):
        if not isinstance(move_info, StateMove):
            raise ValueError("SANSMove: The provided state information is of the wrong type. It must be"
                             " of type StateMove, but was {0}".format(str(type(move_info))))

    @staticmethod
    def _validate(move_info, workspace, coordinates, component):
        SANSMove._validate_state(move_info)
        if coordinates is None or len(coordinates) == 0:
            raise ValueError("SANSMove: The provided coordinates cannot be empty.")
        SANSMove._validate_workspace(workspace)
        SANSMove._validate_component(move_info, component)
        move_info.validate()

    @staticmethod
    def _validate_set_to_zero(move_info, workspace, component):
        SANSMove._validate_state(move_info)
        SANSMove._validate_workspace(workspace)
        SANSMove._validate_component(move_info, component)
        move_info.validate()


class SANSMoveSANS2D(SANSMove):
    def __init__(self):
        super(SANSMoveSANS2D, self).__init__()

    @staticmethod
    def perform_x_and_y_tilts(workspace, detector):
        detector_name = detector.detector_name
        # Perform rotation a y tilt correction. This tilt rotates around the instrument axis / around the X-AXIS!
        y_tilt_correction = detector.y_tilt_correction
        if y_tilt_correction != 0.0:
            y_tilt_correction_direction = {CanonicalCoordinates.X: 1.0,
                                           CanonicalCoordinates.Y: 0.0,
                                           CanonicalCoordinates.Z: 0.0}
            rotate_component(workspace, y_tilt_correction, y_tilt_correction_direction, detector_name)

        # Perform rotation a x tilt correction. This tilt rotates around the instrument axis / around the Z-AXIS!
        x_tilt_correction = detector.x_tilt_correction
        if x_tilt_correction != 0.0:
            x_tilt_correction_direction = {CanonicalCoordinates.X: 0.0,
                                           CanonicalCoordinates.Y: 0.0,
                                           CanonicalCoordinates.Z: 1.0}
            rotate_component(workspace, x_tilt_correction, x_tilt_correction_direction, detector_name)

# pylint: disable=too-many-locals
    @staticmethod
    def _move_high_angle_bank(move_info, workspace, coordinates):
        # Get FRONT_DET_X, FRONT_DET_Z, FRONT_DET_ROT, REAR_DET_X

        hab_detector_x_tag = "Front_Det_X"
        hab_detector_z_tag = "Front_Det_Z"
        hab_detector_rotation_tag = "Front_Det_ROT"
        lab_detector_x_tag = "Rear_Det_X"

        log_names = [hab_detector_x_tag, hab_detector_z_tag, hab_detector_rotation_tag, lab_detector_x_tag]
        log_types = [float, float, float, float]
        log_values = get_single_valued_logs_from_workspace(workspace, log_names, log_types,
                                                           convert_from_millimeter_to_meter=True)

        hab_detector_x = move_info.hab_detector_x \
            if log_values[hab_detector_x_tag] is None else log_values[hab_detector_x_tag]

        hab_detector_z = move_info.hab_detector_z \
            if log_values[hab_detector_z_tag] is None else log_values[hab_detector_z_tag]

        hab_detector_rotation = move_info.hab_detector_rotation \
            if log_values[hab_detector_rotation_tag] is None else log_values[hab_detector_rotation_tag]
        # When we read in the FRONT_Det_ROT tag, we divided by 1000. (since we converted the others to meter)
        if log_values[hab_detector_rotation_tag] is not None:
            hab_detector_rotation *= 1000.

        lab_detector_x = move_info.lab_detector_x \
            if log_values[lab_detector_x_tag] is None else log_values[lab_detector_x_tag]

        # Fixed values
        hab_detector_radius = move_info.hab_detector_radius
        hab_detector_default_x_m = move_info.hab_detector_default_x_m
        hab_detector_default_sd_m = move_info.hab_detector_default_sd_m

        # Detector and name
        hab_detector = move_info.detectors[DetectorType.to_string(DetectorType.HAB)]
        detector_name = hab_detector.detector_name

        # Perform x and y tilt
        SANSMoveSANS2D.perform_x_and_y_tilts(workspace, hab_detector)

        # Perform rotation of around the Y-AXIS. This is more complicated as the high angle bank detector is
        # offset.
        rotation_angle = (-hab_detector_rotation - hab_detector.rotation_correction)
        rotation_direction = {CanonicalCoordinates.X: 0.0,
                              CanonicalCoordinates.Y: 1.0,
                              CanonicalCoordinates.Z: 0.0}
        rotate_component(workspace, rotation_angle, rotation_direction, detector_name)

        # Add translational corrections
        x = coordinates[0]
        y = coordinates[1]
        lab_detector = move_info.detectors[DetectorType.to_string(DetectorType.LAB)]
        rotation_in_radians = math.pi * (hab_detector_rotation + hab_detector.rotation_correction)/180.

        x_shift = ((lab_detector_x + lab_detector.x_translation_correction -
                    hab_detector_x - hab_detector.x_translation_correction -
                    hab_detector.side_correction*(1.0 - math.cos(rotation_in_radians)) +
                    (hab_detector_radius + hab_detector.radius_correction)*(math.sin(rotation_in_radians))) -
                   hab_detector_default_x_m - x)
        y_shift = hab_detector.y_translation_correction - y
        z_shift = (hab_detector_z + hab_detector.z_translation_correction +
                   (hab_detector_radius + hab_detector.radius_correction) * (1.0 - math.cos(rotation_in_radians)) -
                   hab_detector.side_correction * math.sin(rotation_in_radians)) - hab_detector_default_sd_m

        offset = {CanonicalCoordinates.X: x_shift,
                  CanonicalCoordinates.Y: y_shift,
                  CanonicalCoordinates.Z: z_shift}

        move_component(workspace, offset, detector_name)

    @staticmethod
    def _move_low_angle_bank(move_info, workspace, coordinates):
        move_low_angle_bank_for_SANS2D_and_ZOOM(move_info, workspace, coordinates)

    @staticmethod
    def _move_monitor_n(workspace, move_info, monitor_spectrum_number):
        if move_info.monitor_n_offset != 0.0:
            monitor_spectrum_number_as_string = str(monitor_spectrum_number)
            monitor_n_name = move_info.monitor_names[monitor_spectrum_number_as_string]
            instrument = workspace.getInstrument()
            monitor_n = instrument.getComponentByName(monitor_n_name)

            # Get position of monitor n
            monitor_position = monitor_n.getPos()
            z_position_monitor = monitor_position.getZ()

            # The location is relative to the rear-detector, get this position
            lab_detector = move_info.detectors[DetectorType.to_string(DetectorType.LAB)]
            detector_name = lab_detector.detector_name
            lab_detector_component = instrument.getComponentByName(detector_name)
            detector_position = lab_detector_component.getPos()
            z_position_detector = detector_position.getZ()

            monitor_n_offset = move_info.monitor_n_offset
            z_new = z_position_detector + monitor_n_offset
            z_move = z_new - z_position_monitor
            offset = {CanonicalCoordinates.Z: z_move}

            move_component(workspace, offset, monitor_n_name)

    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        # For LOQ we only have to coordinates
        assert len(coordinates) == 2
        _component = component  # noqa
        _is_transmission_workspace = is_transmission_workspace  # noqa

        # Move the high angle bank
        self._move_high_angle_bank(move_info, workspace, coordinates)

        # Move the low angle bank
        self._move_low_angle_bank(move_info, workspace, coordinates)

        # Move the sample holder
        move_sample_holder(workspace, move_info.sample_offset, move_info.sample_offset_direction)

        # Move monitor
        monitor_spectrum_number = 4
        self._move_monitor_n(workspace, move_info, monitor_spectrum_number=monitor_spectrum_number)

    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        # For LOQ we only have to coordinates
        assert len(coordinates) == 2
        coordinates_to_move = [-coordinates[0], -coordinates[1]]
        apply_standard_displacement(move_info, workspace, coordinates_to_move, component)

    def do_set_to_zero(self, move_info, workspace, component):
        set_components_to_original_for_isis(move_info, workspace, component)

    @staticmethod
    def is_correct(instrument_type, run_number, **kwargs):
        return instrument_type is SANSInstrument.SANS2D


class SANSMoveLOQ(SANSMove):
    def __init__(self):
        super(SANSMoveLOQ, self).__init__()

    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        # For LOQ we only have two coordinates
        assert len(coordinates) == 2
        if not is_transmission_workspace:
            # First move the sample holder
            move_sample_holder(workspace, move_info.sample_offset, move_info.sample_offset_direction)

            x = coordinates[0]
            y = coordinates[1]
            center_position = move_info.center_position

            x_shift = center_position - x
            y_shift = center_position - y

            detectors = [DetectorType.to_string(DetectorType.LAB), DetectorType.to_string(DetectorType.HAB)]
            for detector in detectors:
                # Get the detector name
                component_name = move_info.detectors[detector].detector_name

                # Shift the detector by the the input amount
                offset = {CanonicalCoordinates.X: x_shift,
                          CanonicalCoordinates.Y: y_shift}
                move_component(workspace, offset, component_name)

                # Shift the detector according to the corrections of the detector under investigation
                offset_from_corrections = {CanonicalCoordinates.X: move_info.detectors[detector].x_translation_correction,
                                           CanonicalCoordinates.Y: move_info.detectors[detector].y_translation_correction,
                                           CanonicalCoordinates.Z: move_info.detectors[detector].z_translation_correction}
                move_component(workspace, offset_from_corrections, component_name)

    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        # For LOQ we only have to coordinates
        assert len(coordinates) == 2
        coordinates_to_move = [-coordinates[0], -coordinates[1]]
        apply_standard_displacement(move_info, workspace, coordinates_to_move, component)

    def do_set_to_zero(self, move_info, workspace, component):
        set_components_to_original_for_isis(move_info, workspace, component)

    @staticmethod
    def is_correct(instrument_type, run_number, **kwargs):
        return instrument_type is SANSInstrument.LOQ


class SANSMoveLARMOROldStyle(SANSMove):
    def __init__(self):
        super(SANSMoveLARMOROldStyle, self).__init__()

    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        _is_transmission_workspace = is_transmission_workspace  # noqa

        # For LARMOR we only have to coordinates
        assert len(coordinates) == 2

        # Move the sample holder
        move_sample_holder(workspace, move_info.sample_offset, move_info.sample_offset_direction)

        # Shift the low-angle bank detector in the y direction
        y_shift = -coordinates[1]
        coordinates_for_only_y = [0.0, y_shift]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_y,
                                    DetectorType.to_string(DetectorType.LAB))

        # Shift the low-angle bank detector in the x direction
        x_shift = -coordinates[0]
        coordinates_for_only_x = [x_shift, 0.0]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_x,
                                    DetectorType.to_string(DetectorType.LAB))

    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        # For LOQ we only have to coordinates
        assert len(coordinates) == 2

        # Shift component along the y direction
        # Shift the low-angle bank detector in the y direction
        y_shift = -coordinates[1]
        coordinates_for_only_y = [0.0, y_shift]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_y, component)

        # Shift component along the x direction
        x_shift = -coordinates[0]
        coordinates_for_only_x = [x_shift, 0.0]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_x, component)

    def do_set_to_zero(self, move_info, workspace, component):
        set_components_to_original_for_isis(move_info, workspace, component)

    @staticmethod
    def is_correct(instrument_type, run_number, **kwargs):
        is_correct_instrument = instrument_type is SANSInstrument.LARMOR
        is_correct_run_number = run_number < 2217
        return is_correct_instrument and is_correct_run_number


class SANSMoveLARMORNewStyle(SANSMove):
    def __init__(self):
        super(SANSMoveLARMORNewStyle, self).__init__()

    @staticmethod
    def _rotate_around_y_axis(move_info, workspace, angle, component, bench_rotation):
        detector = move_info.detectors[component]
        detector_name = detector.detector_name
        # Note that the angle definition for the bench in LARMOR and in Mantid seem to have a different handedness
        total_angle = bench_rotation - angle
        direction = {CanonicalCoordinates.X: 0.0,
                     CanonicalCoordinates.Y: 1.0,
                     CanonicalCoordinates.Z: 0.0}
        rotate_component(workspace, total_angle, direction, detector_name)

    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        _is_transmission_workspace = is_transmission_workspace  # noqa

        # For LARMOR we only have to coordinates
        assert len(coordinates) == 2

        # Move the sample holder
        move_sample_holder(workspace, move_info.sample_offset, move_info.sample_offset_direction)

        # Shift the low-angle bank detector in the y direction
        y_shift = -coordinates[1]
        coordinates_for_only_y = [0.0, y_shift]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_y,
                                    DetectorType.to_string(DetectorType.LAB))

        # Shift the low-angle bank detector in the x direction
        angle = coordinates[0]

        bench_rot_tag = "Bench_Rot"
        log_names = [bench_rot_tag]
        log_types = [float]
        log_values = get_single_valued_logs_from_workspace(workspace, log_names, log_types)
        bench_rotation = move_info.bench_rotation \
            if log_values[bench_rot_tag] is None else log_values[bench_rot_tag]

        self._rotate_around_y_axis(move_info, workspace, angle,
                                   DetectorType.to_string(DetectorType.LAB), bench_rotation)

    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        # For LOQ we only have to coordinates
        assert len(coordinates) == 2

        # Shift component along the y direction
        # Shift the low-angle bank detector in the y direction
        y_shift = -coordinates[1]
        coordinates_for_only_y = [0.0, y_shift]
        apply_standard_displacement(move_info, workspace, coordinates_for_only_y, component)

        # Shift component along the x direction; not that we don't want to perform a bench rotation again
        angle = coordinates[0]
        self._rotate_around_y_axis(move_info, workspace, angle, component, 0.0)

    def do_set_to_zero(self, move_info, workspace, component):
        set_components_to_original_for_isis(move_info, workspace, component)

    @staticmethod
    def is_correct(instrument_type, run_number, **kwargs):
        is_correct_instrument = instrument_type is SANSInstrument.LARMOR
        is_correct_run_number = run_number >= 2217
        return is_correct_instrument and is_correct_run_number


class SANSMoveZOOM(SANSMove):
    @staticmethod
    def _move_low_angle_bank(move_info, workspace, coordinates):
        move_low_angle_bank_for_SANS2D_and_ZOOM(move_info, workspace, coordinates, use_rear_det_z=False)

    @staticmethod
    def _move_monitor_n(workspace, move_info, monitor_spectrum_number):
        monitor_offset = move_info.monitor_n_offset
        if monitor_offset != 0.0:
            monitor_spectrum_number_as_string = str(monitor_spectrum_number)
            monitor_n_name = move_info.monitor_names[monitor_spectrum_number_as_string]
            instrument = workspace.getInstrument()
            monitor_n = instrument.getComponentByName(monitor_n_name)

            # Get position of monitor n
            monitor_position = monitor_n.getPos()
            z_position_monitor = monitor_position.getZ()

            # The location is relative to the rear-detector, get this position
            lab_detector = move_info.detectors[DetectorType.to_string(DetectorType.LAB)]
            detector_name = lab_detector.detector_name
            lab_detector_component = instrument.getComponentByName(detector_name)
            detector_position = lab_detector_component.getPos()
            z_position_detector = detector_position.getZ()

            monitor_n_offset = monitor_offset
            z_new = z_position_detector + monitor_n_offset
            z_move = z_new - z_position_monitor
            offset = {CanonicalCoordinates.Z: z_move}

            move_component(workspace, offset, monitor_n_name)

    def do_move_initial(self, move_info, workspace, coordinates, component, is_transmission_workspace):
        # For ZOOM we only have to coordinates
        assert len(coordinates) == 2

        _component = component  # noqa
        _is_transmission_workspace = is_transmission_workspace  # noqa

        # Move the low angle bank
        self._move_low_angle_bank(move_info, workspace, coordinates)

        # Move the sample holder
        move_sample_holder(workspace, move_info.sample_offset, move_info.sample_offset_direction)

        # Move the monitor
        monitor_spectrum = 5  # Only M5 can be moved for ZOOM
        self._move_monitor_n(workspace, move_info, monitor_spectrum_number=monitor_spectrum)

    def do_move_with_elementary_displacement(self, move_info, workspace, coordinates, component):
        # For ZOOM we only have to coordinates
        assert len(coordinates) == 2
        coordinates_to_move = [-coordinates[0], -coordinates[1]]
        apply_standard_displacement(move_info, workspace, coordinates_to_move, component)

    def do_set_to_zero(self, move_info, workspace, component):
        set_components_to_original_for_isis(move_info, workspace, component)

    @staticmethod
    def is_correct(instrument_type, run_number, **kwargs):
        return instrument_type is SANSInstrument.ZOOM


def create_mover(workspace):
    # Get selection
    run_number = workspace.getRunNumber()
    instrument = workspace.getInstrument()
    instrument_name = instrument.getName()
    instrument_name = sanitise_instrument_name(instrument_name)
    instrument_type = SANSInstrument.from_string(instrument_name)
    if SANSMoveLOQ.is_correct(instrument_type, run_number):
        mover = SANSMoveLOQ()
    elif SANSMoveSANS2D.is_correct(instrument_type, run_number):
        mover = SANSMoveSANS2D()
    elif SANSMoveLARMOROldStyle.is_correct(instrument_type, run_number):
        mover = SANSMoveLARMOROldStyle()
    elif SANSMoveLARMORNewStyle.is_correct(instrument_type, run_number):
        mover = SANSMoveLARMORNewStyle()
    elif SANSMoveZOOM.is_correct(instrument_type, run_number):
        mover = SANSMoveZOOM()
    else:
        mover = None
        NotImplementedError("SANSLoaderFactory: Other instruments are not implemented yet.")
    return mover
