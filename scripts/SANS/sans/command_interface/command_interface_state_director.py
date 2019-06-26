# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.py3compat import Enum
from sans.common.enums import DataType
from sans.user_file.state_director import StateDirectorISIS
from sans.state.data import get_data_builder
from sans.user_file.user_file_parser import (UserFileParser)
from sans.user_file.user_file_reader import (UserFileReader)
from sans.user_file.settings_tags import (MonId, monitor_spectrum, OtherId, SampleId, GravityId, SetId, position_entry,
                                          fit_general, FitId, monitor_file, mask_angle_entry, LimitsId, range_entry,
                                          simple_range, DetectorId, event_binning_string_values, det_fit_range,
                                          single_entry_with_detector)
from sans.common.file_information import SANSFileInformationFactory

# ----------------------------------------------------------------------------------------------------------------------
# Commands
# ----------------------------------------------------------------------------------------------------------------------


# ------------------
# IDs for commands. We use here serializable_enum since enum is not available in the current Python configuration.
# ------------------
class DataCommandId(Enum):
    sample_scatter = 1
    sample_transmission = 2
    sample_direct = 3
    can_scatter = 4
    can_transmission = 5
    can_direct = 6


class NParameterCommandId(Enum):
    clean = 1
    reduction_dimensionality = 2
    compatibility_mode = 3
    user_file = 4
    mask = 5
    sample_offset = 6
    detector = 7
    event_slices = 8
    flood_file = 9
    wavelength_correction_file = 10
    user_specified_output_name = 11
    user_specified_output_name_suffix = 12
    use_reduction_mode_as_suffix = 13
    incident_spectrum = 14
    gravity = 15
    centre = 16
    save = 17
    trans_fit = 18
    phi_limit = 19
    mask_radius = 20
    wavelength_limit = 21
    qxy_limit = 22
    wavrange_settings = 23
    front_detector_rescale = 24
    detector_offsets = 25


class Command(object):
    def __init__(self, command_id):
        super(Command, self).__init__()
        self.command_id = command_id


class DataCommand(Command):
    """
    A command which is associated with setting data information.
    """
    def __init__(self, command_id, file_name, period=None):
        super(DataCommand, self).__init__(command_id)
        self.file_name = file_name
        self.period = period


class NParameterCommand(Command):
    """
    A command which has n parameters in a list.
    """
    def __init__(self, command_id, values):
        super(NParameterCommand, self).__init__(command_id)
        self.values = values


class FitData(object):
    """
    Describes the fit mode. This is not part of the SANSType module since we only need it here. It is slightly
    inconsistent but it is very localized.
    """
    class Sample(object):
        pass

    class Can(object):
        pass

    class Both(object):
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Command Interface State Director

# Explanation of the implementation
#
# Previously the ISISCommandInterface just executed commands one after another. Settings were stored in the reduction
# singleton. Once in a while the reduction singleton was reset.
#
# Here we need to have state director which builds the SANS state out of these legacy commands. Note that before we
# can process any of the commands we need to find the data entries, since they drive the reduction.
# All other commands should be setting the SANSState in order.
# ----------------------------------------------------------------------------------------------------------------------
class CommandInterfaceStateDirector(object):
    def __init__(self, facility):
        super(CommandInterfaceStateDirector, self).__init__()
        self._commands = []
        self._state_director = None

        self._processed_state_settings = {}

        self._facility = facility
        self._method_map = None
        self._set_up_method_map()

    def add_command(self, command):
        self._commands.append(command)

    def clear_commands(self):
        self._commands = []
        self._processed_state_settings = {}

    def process_commands(self):
        """
        Here we process the commands that have been set. This would be triggered by a command which requests a reduction

        The execution strategy is:
        1. Find the data entries and great a SANSStateData object out of them
        2. Go sequentially through the commands in a FIFO manner (except for the data entries)
        3. Delete the processed state settings. We only need to retain the commands. If we also retain the
           processed state settings then we will populate some entries twice.
        4. Returns the constructed state
        @returns a list of valid SANSState object which can be used for data reductions or raises an exception.
        """
        # 1. Get a SANSStateData object.
        data_state = self._get_data_state()

        # 2. Go through
        state = self._process_command_queue(data_state)

        # 3. Leave commands in place put clear the list of processed commands, else they will be reused.
        self._processed_state_settings = {}

        # 4. Provide the state
        return state

    def get_commands(self):
        return self._commands

    def _get_data_state(self):
        # Get the data commands
        data_commands = self._get_data_commands()
        data_elements = self._get_elements_with_key(DataCommandId.sample_scatter, data_commands)
        data_element = data_elements[-1]
        file_name = data_element.file_name
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)

        # Build the state data
        data_builder = get_data_builder(self._facility, file_information)
        self._set_data_element(data_builder.set_sample_scatter, data_builder.set_sample_scatter_period,
                               DataCommandId.sample_scatter, data_commands)
        self._set_data_element(data_builder.set_sample_transmission, data_builder.set_sample_transmission_period,
                               DataCommandId.sample_transmission, data_commands)
        self._set_data_element(data_builder.set_sample_direct, data_builder.set_sample_direct_period,
                               DataCommandId.sample_direct, data_commands)
        self._set_data_element(data_builder.set_can_scatter, data_builder.set_can_scatter_period,
                               DataCommandId.can_scatter, data_commands)
        self._set_data_element(data_builder.set_can_transmission, data_builder.set_can_transmission_period,
                               DataCommandId.can_transmission, data_commands)
        self._set_data_element(data_builder.set_can_direct, data_builder.set_can_direct_period,
                               DataCommandId.can_direct, data_commands)

        return data_builder.build()

    def _get_data_commands(self):
        """
        Grabs and removes the data commands from the command queue.

        @return: a list of data commands
        """
        # Grab the data commands
        data_commands = [element for element in self._commands if isinstance(element, DataCommand)]
        return data_commands

    def _set_data_element(self, data_builder_file_setter, data_builder_period_setter, command_id, commands):
        """
        Sets a data element (e.g. sample scatter file and sample scatter period) on the data builder.

        @param data_builder_file_setter: a handle to the correct setter for the file on the data builder.
        @param data_builder_period_setter: a handle to the correct setter for the period on the data builder.
        @param command_id: the command id
        @param commands: a list of commands.
        """
        data_elements = self._get_elements_with_key(command_id, commands)

        # If there is no element, then there is nothing to do
        if len(data_elements) == 0:
            return

        # If there is more than one element, then we are only interested in the last element. The user could
        # have overridden his wishes, e.g.
        # ...
        # AssignSample('SANS2D1234')
        # ...
        # AssignSample('SANS2D4321')
        # ...
        # We select therefore the last element
        data_element = data_elements[-1]
        file_name = data_element.file_name
        period = data_element.period
        data_builder_file_setter(file_name)
        data_builder_period_setter(period)

    @staticmethod
    def _get_elements_with_key(command_id, command_list):
        """
        Get all elements in the command list with a certain id

        @param command_id: the id of the command.
        @param command_list: a list of commands.
        @return: a list of commands which match the id.
        """
        return [element for element in command_list if element.command_id is command_id]

    def _process_command_queue(self, data_state):
        """
        Process the command queue sequentially as FIFO structure

        @param data_state: the data state.
        @return: a SANSState object.
        """
        file_name = data_state.sample_scatter
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)

        self._state_director = StateDirectorISIS(data_state, file_information)

        # If we have a clean instruction in there, then we should apply it to all commands
        self._apply_clean_if_required()

        # Evaluate all commands which adds them to the _processed_state_settings dictionary,
        # except for DataCommands which we deal with separately
        for command in self._commands:
            if isinstance(command, DataCommand):
                continue
            command_id = command.command_id
            process_function = self._method_map[command_id]
            process_function(command)

        # The user file state director
        self._state_director.add_state_settings(self._processed_state_settings)
        return self._state_director.construct()

    def _set_up_method_map(self):
        """
        Sets up a mapping between command ids and the adequate processing methods which can handle the command.
        """
        self._method_map = {NParameterCommandId.user_file: self._process_user_file,
                            NParameterCommandId.mask: self._process_mask,
                            NParameterCommandId.incident_spectrum: self._process_incident_spectrum,
                            NParameterCommandId.clean: self._process_clean,
                            NParameterCommandId.reduction_dimensionality: self._process_reduction_dimensionality,
                            NParameterCommandId.sample_offset: self._process_sample_offset,
                            NParameterCommandId.detector: self._process_detector,
                            NParameterCommandId.gravity: self._process_gravity,
                            NParameterCommandId.centre: self._process_centre,
                            NParameterCommandId.trans_fit: self._process_trans_fit,
                            NParameterCommandId.front_detector_rescale: self._process_front_detector_rescale,
                            NParameterCommandId.event_slices: self._process_event_slices,
                            NParameterCommandId.flood_file: self._process_flood_file,
                            NParameterCommandId.phi_limit: self._process_phi_limit,
                            NParameterCommandId.wavelength_correction_file: self._process_wavelength_correction_file,
                            NParameterCommandId.mask_radius: self._process_mask_radius,
                            NParameterCommandId.wavelength_limit: self._process_wavelength_limit,
                            NParameterCommandId.qxy_limit: self._process_qxy_limit,
                            NParameterCommandId.wavrange_settings: self._process_wavrange,
                            NParameterCommandId.compatibility_mode: self._process_compatibility_mode,
                            NParameterCommandId.detector_offsets: self._process_detector_offsets,
                            NParameterCommandId.save: self._process_save,
                            NParameterCommandId.user_specified_output_name: self._process_user_specified_output_name,
                            NParameterCommandId.user_specified_output_name_suffix:
                                self._process_user_specified_output_name_suffix,
                            NParameterCommandId.use_reduction_mode_as_suffix:
                                self._process_use_reduction_mode_as_suffix
                            }

    def add_to_processed_state_settings(self, new_state_settings, treat_list_as_element=False):
        """
        Adds the new entries to the already processed state settings

        @param new_state_settings: a dictionary with new entries for the processed state settings
        @param treat_list_as_element: if we have a list and add it for the fist time, then we should treat it as an
                                      element if true. For example, if the state is [1, 2] the a new settint would, be
                                      [[1, 2,]] and not [1, 2]. With a further entry it could be [[1,2], [3,4]].
        """
        for key, value in list(new_state_settings.items()):
            # Add the new entry
            # 1. A similar entry can already exist, then append it (or extend it)
            # 2. The entry does not exist, but it is in form of a list (you would get that for example when
            #    dealing with input from the UserFileReader
            # 3. The entry does not exist and is not in a list. In this case we need to add it to a list.
            if key in self._processed_state_settings:
                # If the key already exists then we have to be careful. We have the current value V = [A, B, ...]
                # and our new element N
                # i. If the existing entries (ie A, ...) are not lists and N is not a list, then append to V.
                # ii. If the existing entries (ie A, ...) are not lists and N is a list then extend V.
                # iii. If the existing entries (ie A, ...) are lists and N is a list then append to V.
                # iv. If the existing entries (ie A, ...) are lists and N is not a list, then raise
                # The reason we have to be careful is that we might get an N from a user file which comes always already
                # in the form of a list.
                old_values = self._processed_state_settings[key]
                is_old_first_entry_a_list = isinstance(old_values[0], list)
                is_new_entry_a_list = isinstance(value, list)

                if not is_old_first_entry_a_list and not is_new_entry_a_list:
                    old_values.append(value)
                elif not is_old_first_entry_a_list and is_new_entry_a_list:
                    old_values.extend(value)
                elif is_old_first_entry_a_list and is_new_entry_a_list:
                    old_values.append(value)
                else:
                    raise RuntimeError("CommandInterfaceStateDirector: Trying to insert {0} which is a list into {0} "
                                       "which is collection of non-list elements".format(value, old_values))
            elif isinstance(value, list) and treat_list_as_element:
                self._processed_state_settings.update({key: [value]})
            elif isinstance(value, list):
                self._processed_state_settings.update({key: value})
            else:
                self._processed_state_settings.update({key: [value]})

    def _process_user_file(self, command):
        """
        Processes a user file and retain the parased tags

        @param command: the command with the user file path
        """
        file_name = command.values[0]
        user_file_reader = UserFileReader(file_name)
        new_state_entries = user_file_reader.read_user_file()
        self.add_to_processed_state_settings(new_state_entries)

    def _process_mask(self, command):
        """
        We need to process a mask line as specified in the user file.
        """
        mask_command = command.values[0]
        # Use the user file parser to extract the values from the user file setting.
        user_file_parser = UserFileParser()
        parsed_output = user_file_parser.parse_line(mask_command)
        self.add_to_processed_state_settings(parsed_output)

    def _process_incident_spectrum(self, command):
        incident_monitor = command.values[0]
        interpolate = command.values[1]
        is_trans = command.values[2]
        new_state_entries = {MonId.spectrum: monitor_spectrum(spectrum=incident_monitor,
                                                              is_trans=is_trans,
                                                              interpolate=interpolate)}
        self.add_to_processed_state_settings(new_state_entries)

    def _apply_clean_if_required(self):
        """
        The cleans all commands up to the clean command point.

        We have to do this clean before we start processing the elements.
        """
        index_first_clean_command = None
        for index in reversed(list(range(0, len(self._commands)))):
            element = self._commands[index]
            if element.command_id == NParameterCommandId.clean:
                index_first_clean_command = index
                break
        if index_first_clean_command is not None:
            del(self._commands[0:(index_first_clean_command + 1)])
            self._processed_state_settings = {}

    def _process_clean(self, command):
        _ = command  # noqa
        raise RuntimeError("Trying the process a Clean command. The clean command should have removed itself and "
                           "all previous commands. If it is still here, then this is a bug")

    def _process_reduction_dimensionality(self, command):
        _ = command  # noqa
        reduction_dimensionality = command.values[0]
        new_state_entries = {OtherId.reduction_dimensionality: reduction_dimensionality}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_sample_offset(self, command):
        sample_offset = command.values[0]
        new_state_entries = {SampleId.offset: sample_offset}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_detector(self, command):
        reduction_mode = command.values[0]
        new_state_entries = {DetectorId.reduction_mode: reduction_mode}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_gravity(self, command):
        use_gravity = command.values[0]
        extra_length = command.values[1]
        new_state_entries = {GravityId.on_off: use_gravity,
                             GravityId.extra_length: extra_length}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_centre(self, command):
        pos1 = command.values[0]
        pos2 = command.values[1]
        detector_type = command.values[2]
        new_state_entries = {SetId.centre: position_entry(pos1=pos1, pos2=pos2, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_trans_fit(self, command):
        def fit_type_to_data_type(fit_type_to_convert):
            return DataType.Can if fit_type_to_convert is FitData.Can else DataType.Sample

        fit_data = command.values[0]
        wavelength_low = command.values[1]
        wavelength_high = command.values[2]
        fit_type = command.values[3]
        polynomial_order = command.values[4]
        if fit_data is FitData.Both:
            data_to_fit = [FitData.Sample, FitData.Can]
        else:
            data_to_fit = [fit_data]

        new_state_entries = {}
        for element in data_to_fit:
            data_type = fit_type_to_data_type(element)
            new_state_entries.update({FitId.general: fit_general(start=wavelength_low, stop=wavelength_high,
                                                                 fit_type=fit_type, data_type=data_type,
                                                                 polynomial_order=polynomial_order)})
        self.add_to_processed_state_settings(new_state_entries)

    def _process_front_detector_rescale(self, command):
        scale = command.values[0]
        shift = command.values[1]
        fit_scale = command.values[2]
        fit_shift = command.values[3]
        q_min = command.values[4]
        q_max = command.values[5]

        # Set the scale and the shift
        new_state_entries = {DetectorId.rescale: scale, DetectorId.shift: shift}

        # Set the fit for the scale
        new_state_entries.update({DetectorId.rescale_fit: det_fit_range(start=q_min, stop=q_max, use_fit=fit_scale)})

        # Set the fit for shift
        new_state_entries.update({DetectorId.shift_fit: det_fit_range(start=q_min, stop=q_max, use_fit=fit_shift)})

        self.add_to_processed_state_settings(new_state_entries)

    def _process_event_slices(self, command):
        event_slice_value = command.values
        new_state_entries = {OtherId.event_slices: event_binning_string_values(value=event_slice_value)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_flood_file(self, command):
        file_path = command.values[0]
        detector_type = command.values[1]
        new_state_entries = {MonId.flat: monitor_file(file_path=file_path, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_phi_limit(self, command):
        phi_min = command.values[0]
        phi_max = command.values[1]
        use_phi_mirror = command.values[2]
        new_state_entries = {LimitsId.angle: mask_angle_entry(min=phi_min, max=phi_max, use_mirror=use_phi_mirror)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavelength_correction_file(self, command):
        file_path = command.values[0]
        detector_type = command.values[1]
        new_state_entries = {MonId.direct: monitor_file(file_path=file_path, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_mask_radius(self, command):
        radius_min = command.values[0]
        radius_max = command.values[1]
        new_state_entries = {LimitsId.radius: range_entry(start=radius_min, stop=radius_max)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavelength_limit(self, command):
        wavelength_low = command.values[0]
        wavelength_high = command.values[1]
        wavelength_step = command.values[2]
        wavelength_step_type = command.values[3]
        new_state_entries = {LimitsId.wavelength: simple_range(start=wavelength_low, stop=wavelength_high,
                                                               step=wavelength_step, step_type=wavelength_step_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavrange(self, command):
        wavelength_low = command.values[0]
        wavelength_high = command.values[1]
        full_wavelength_range = command.values[2]
        reduction_mode = command.values[3]

        # Update the lower and the upper wavelength values. Note that this is considered an incomplete setting, since
        # not step or step type have been specified. This means we need to update one of the processed commands, which
        # is not nice but the command interface forces us to do so. We take a copy of the last LimitsId.wavelength
        # entry, we copy it and then change the desired settings. This means it has to be set at this point, else
        # something is wrong
        if LimitsId.wavelength in self._processed_state_settings:
            last_entry = self._processed_state_settings[LimitsId.wavelength][-1]

            new_wavelength_low = wavelength_low if wavelength_low is not None else last_entry.start
            new_wavelength_high = wavelength_high if wavelength_high is not None else last_entry.stop
            new_range = simple_range(start=new_wavelength_low, stop=new_wavelength_high, step=last_entry.step,
                                     step_type=last_entry.step_type)

            if wavelength_low is not None or wavelength_high is not None:
                copied_entry = {LimitsId.wavelength: new_range}
                self.add_to_processed_state_settings(copied_entry)
        else:
            raise RuntimeError("CommandInterfaceStateDirector: Setting the lower and upper wavelength bounds is not"
                               " possible. We require also a step and step range")

        if full_wavelength_range is not None:
            full_wavelength_range_entry = {OtherId.use_full_wavelength_range: full_wavelength_range}
            self.add_to_processed_state_settings(full_wavelength_range_entry)

        if reduction_mode is not None:
            reduction_mode_entry = {DetectorId.reduction_mode: reduction_mode}
            self.add_to_processed_state_settings(reduction_mode_entry)

    def _process_qxy_limit(self, command):
        q_min = command.values[0]
        q_max = command.values[1]
        q_step = command.values[2]
        q_step_type = command.values[3]
        new_state_entries = {LimitsId.qxy: simple_range(start=q_min, stop=q_max, step=q_step, step_type=q_step_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_compatibility_mode(self, command):
        use_compatibility_mode = command.values[0]
        new_state_entries = {OtherId.use_compatibility_mode: use_compatibility_mode}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_detector_offsets(self, command):
        detector_type = command.values[0]
        x = command.values[1]
        y = command.values[2]
        z = command.values[3]
        rotation = command.values[4]
        radius = command.values[5]
        side = command.values[6]
        x_tilt = command.values[7]
        y_tilt = command.values[8]

        # Set the offsets
        new_state_entries = {DetectorId.correction_x: single_entry_with_detector(entry=x, detector_type=detector_type),
                             DetectorId.correction_y: single_entry_with_detector(entry=y, detector_type=detector_type),
                             DetectorId.correction_z: single_entry_with_detector(entry=z, detector_type=detector_type),
                             DetectorId.correction_rotation:
                                 single_entry_with_detector(entry=rotation, detector_type=detector_type),
                             DetectorId.correction_radius:
                                 single_entry_with_detector(entry=radius, detector_type=detector_type),
                             DetectorId.correction_translation:
                                 single_entry_with_detector(entry=side, detector_type=detector_type),
                             DetectorId.correction_x_tilt:
                                 single_entry_with_detector(entry=x_tilt, detector_type=detector_type),
                             DetectorId.correction_y_tilt:
                                 single_entry_with_detector(entry=y_tilt, detector_type=detector_type),
                             }
        self.add_to_processed_state_settings(new_state_entries)

    def _process_save(self, command):
        save_algorithms = command.values[0]
        save_as_zero_error_free = command.values[1]
        new_state_entries = {OtherId.save_types: save_algorithms,
                             OtherId.save_as_zero_error_free: save_as_zero_error_free}
        self.add_to_processed_state_settings(new_state_entries,  treat_list_as_element=True)

    def _process_user_specified_output_name(self, command):
        user_specified_output_name = command.values[0]
        new_state_entry = {OtherId.user_specified_output_name: user_specified_output_name}
        self.add_to_processed_state_settings(new_state_entry)

    def _process_user_specified_output_name_suffix(self, command):
        user_specified_output_name_suffix = command.values[0]
        new_state_entry = {OtherId.user_specified_output_name_suffix: user_specified_output_name_suffix}
        self.add_to_processed_state_settings(new_state_entry)

    def _process_use_reduction_mode_as_suffix(self, command):
        use_reduction_mode_as_suffix = command.values[0]
        new_state_entry = {OtherId.use_reduction_mode_as_suffix: use_reduction_mode_as_suffix}
        self.add_to_processed_state_settings(new_state_entry)

    def remove_last_user_file(self):
        """
        Removes the last added user file from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(NParameterCommandId.user_file)

    def remove_last_scatter_sample(self):
        """
        Removes the last added scatter sample from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.sample_scatter)

    def remove_last_sample_transmission_and_direct(self):
        """
        Removes the last added scatter transmission and direct from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.sample_transmission)
        self._remove_last_element(DataCommandId.sample_direct)

    def remove_last_scatter_can(self):
        """
        Removes the last added scatter can from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.can_scatter)

    def remove_last_can_transmission_and_direct(self):
        """
        Removes the last added can transmission and direct from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.can_transmission)
        self._remove_last_element(DataCommandId.can_direct)

    def _remove_last_element(self, command_id):
        """
        Removes the last instance of a command associated with the command_id.

        This method is vital for batch reduction.
        TODO: more explanation
        @param command_id: the command_id of the command which whose last instance we want to remove
        """
        index_to_remove = None
        for index, element in reversed(list(enumerate(self._commands))):
            if element.command_id == command_id:
                index_to_remove = index
                break
        if index_to_remove is not None:
            del(self._commands[index_to_remove])
        else:
            raise RuntimeError("Tried to delete the last instance of {0}, but none was present in the list of "
                               "commands".format(command_id))
