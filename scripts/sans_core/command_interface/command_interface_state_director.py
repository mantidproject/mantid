# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum
from sans_core.common.enums import DataType
from sans_core.common.file_information import SANSFileInformationFactory
from sans_core.state.AllStates import AllStates
from sans_core.state.StateObjects.StateData import get_data_builder
from sans_core.user_file.settings_tags import (
    MonId,
    monitor_spectrum,
    OtherId,
    SampleId,
    GravityId,
    SetId,
    position_entry,
    fit_general,
    FitId,
    monitor_file,
    mask_angle_entry,
    LimitsId,
    range_entry,
    simple_range,
    q_xy_range,
    DetectorId,
    event_binning_string_values,
    det_fit_range,
    single_entry_with_detector,
)
from sans_core.user_file.toml_parsers.toml_parser import TomlParser
from sans_core.user_file.txt_parsers.CommandInterfaceAdapter import CommandInterfaceAdapter

from sans_core.user_file.user_file_parser import UserFileParser
from sans_core.user_file.user_file_reader import UserFileReader


# ----------------------------------------------------------------------------------------------------------------------
# Commands
# ----------------------------------------------------------------------------------------------------------------------


# ------------------
# IDs for commands. We use here serializable_enum since enum is not available in the current Python configuration.
# ------------------
class DataCommandId(Enum):
    CAN_DIRECT = "can_direct"
    CAN_SCATTER = "can_scatter"
    CAN_TRANSMISSION = "can_transmission"

    SAMPLE_SCATTER = "sample_scatter"
    SAMPLE_TRANSMISSION = "sample_transmission"
    SAMPLE_DIRECT = "sample_direct"


class NParameterCommandId(object):
    CENTRE = "centre"
    CLEAN = "clean"
    COMPATIBILITY_MODE = "compatibility_mode"
    DETECTOR = "detector"
    DETECTOR_OFFSETS = "detector_offsets"
    EVENT_SLICES = "event_slices"
    FLOOD_FILE = "flood_file"
    FRONT_DETECTOR_RESCALE = "front_detector_rescale"
    GRAVITY = "gravity"
    INCIDENT_SPECTRUM = "incident_spectrum"
    MASK = "mask"
    MASK_RADIUS = "mask_radius"
    REDUCTION_DIMENSIONALITY = "reduction_dimensionality"
    PHI_LIMIT = "phi_limit"
    QXY_LIMIT = "qxy_limit"
    SAMPLE_OFFSET = "sample_offset"
    SAVE = "save"
    TRANS_FIT = "trans_fit"
    USER_FILE = "user_file"
    USE_REDUCTION_MODE_AS_SUFFIX = "use_reduction_mode_as_suffix"
    USER_SPECIFIED_OUTPUT_NAME = "user_specified_output_name"
    USER_SPECIFIED_OUTPUT_NAME_SUFFIX = "user_specified_output_name_suffix"
    WAVELENGTH_CORRECTION_FILE = "wavelength_correction_file"
    WAVELENGTH_LIMIT = "wavelength_limit"
    WAV_RANGE_SETTINGS = "wavrange_settings"


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
        self._data_info = None

        self._processed_state_settings = {}
        self._processed_state_obj: AllStates = None

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
        self._data_info = self._get_data_state()

        # 2. Go through
        state = self._process_command_queue()

        # 3. Leave commands in place put clear the list of processed commands, else they will be reused.
        self._processed_state_settings = {}

        # 4. Provide the state
        return state

    def get_commands(self):
        return self._commands

    def _get_data_state(self):
        # Get the data commands
        data_commands = self._get_data_commands()
        data_elements = self._get_elements_with_key(DataCommandId.SAMPLE_SCATTER, data_commands)
        data_element = data_elements[-1]
        file_name = data_element.file_name
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)

        # Build the state data
        data_builder = get_data_builder(self._facility, file_information)
        self._set_data_element(
            data_builder.set_sample_scatter, data_builder.set_sample_scatter_period, DataCommandId.SAMPLE_SCATTER, data_commands
        )
        self._set_data_element(
            data_builder.set_sample_transmission,
            data_builder.set_sample_transmission_period,
            DataCommandId.SAMPLE_TRANSMISSION,
            data_commands,
        )
        self._set_data_element(
            data_builder.set_sample_direct, data_builder.set_sample_direct_period, DataCommandId.SAMPLE_DIRECT, data_commands
        )
        self._set_data_element(data_builder.set_can_scatter, data_builder.set_can_scatter_period, DataCommandId.CAN_SCATTER, data_commands)
        self._set_data_element(
            data_builder.set_can_transmission, data_builder.set_can_transmission_period, DataCommandId.CAN_TRANSMISSION, data_commands
        )
        self._set_data_element(data_builder.set_can_direct, data_builder.set_can_direct_period, DataCommandId.CAN_DIRECT, data_commands)

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

    def _process_command_queue(self):
        """
        Process the command queue sequentially as FIFO structure

        @param data_state: the data state.
        @return: a SANSState object.
        """
        file_name = self._data_info.sample_scatter
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)

        # If we have a clean instruction in there, then we should apply it to all commands
        self._apply_clean_if_required()

        # Evaluate all commands which adds them to the _processed_state_settings dictionary,
        # except for DataCommands which we deal with separately
        for command in self._commands:
            if isinstance(command, DataCommand):
                continue

            command_id = command.command_id
            if NParameterCommandId.USER_FILE in command_id:
                self._processed_state_obj = self._process_user_file(command, file_information=file_information)
                continue

            process_function = self._method_map[command_id]
            process_function(command)

        state_builder_adapter = CommandInterfaceAdapter(
            file_information=file_information, processed_state=self._processed_state_settings, existing_state_obj=self._processed_state_obj
        )
        states = state_builder_adapter.get_all_states(file_information=file_information)
        states.data = self._data_info
        return states

    def _set_up_method_map(self):
        """
        Sets up a mapping between command ids and the adequate processing methods which can handle the command.
        """
        self._method_map = {
            NParameterCommandId.USER_FILE: self._process_user_file,
            NParameterCommandId.MASK: self._process_mask,
            NParameterCommandId.INCIDENT_SPECTRUM: self._process_incident_spectrum,
            NParameterCommandId.CLEAN: self._process_clean,
            NParameterCommandId.REDUCTION_DIMENSIONALITY: self._process_reduction_dimensionality,
            NParameterCommandId.SAMPLE_OFFSET: self._process_sample_offset,
            NParameterCommandId.DETECTOR: self._process_detector,
            NParameterCommandId.GRAVITY: self._process_gravity,
            NParameterCommandId.CENTRE: self._process_centre,
            NParameterCommandId.TRANS_FIT: self._process_trans_fit,
            NParameterCommandId.FRONT_DETECTOR_RESCALE: self._process_front_detector_rescale,
            NParameterCommandId.EVENT_SLICES: self._process_event_slices,
            NParameterCommandId.FLOOD_FILE: self._process_flood_file,
            NParameterCommandId.PHI_LIMIT: self._process_phi_limit,
            NParameterCommandId.WAVELENGTH_CORRECTION_FILE: self._process_wavelength_correction_file,
            NParameterCommandId.MASK_RADIUS: self._process_mask_radius,
            NParameterCommandId.WAVELENGTH_LIMIT: self._process_wavelength_limit,
            NParameterCommandId.QXY_LIMIT: self._process_qxy_limit,
            NParameterCommandId.WAV_RANGE_SETTINGS: self._process_wavrange,
            NParameterCommandId.COMPATIBILITY_MODE: self._process_compatibility_mode,
            NParameterCommandId.DETECTOR_OFFSETS: self._process_detector_offsets,
            NParameterCommandId.SAVE: self._process_save,
            NParameterCommandId.USER_SPECIFIED_OUTPUT_NAME: self._process_user_specified_output_name,
            NParameterCommandId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX: self._process_user_specified_output_name_suffix,
            NParameterCommandId.USE_REDUCTION_MODE_AS_SUFFIX: self._process_use_reduction_mode_as_suffix,
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
                    raise RuntimeError(
                        f"CommandInterfaceStateDirector: Trying to insert {value} which is a list into {old_values} "
                        "which is collection of non-list elements"
                    )
            elif isinstance(value, list) and treat_list_as_element:
                self._processed_state_settings.update({key: [value]})
            elif isinstance(value, list):
                self._processed_state_settings.update({key: value})
            else:
                self._processed_state_settings.update({key: [value]})

    def _process_user_file(self, command, file_information):
        """
        Processes a user file and retain the parased tags

        @param command: the command with the user file path
        """
        file_name = command.values[0]

        if file_name.casefold().endswith(".toml".casefold()):
            toml_file_reader = TomlParser()
            new_state_entries = toml_file_reader.parse_toml_file(toml_file_path=file_name, file_information=file_information)
        else:
            # Now comes the fun part where we try to coerce this to put out a State* object
            user_file_reader = UserFileReader(file_name)
            old_param_mapping = user_file_reader.read_user_file()
            command_adapter = CommandInterfaceAdapter(processed_state=old_param_mapping, file_information=file_information)
            new_state_entries = command_adapter.get_all_states(file_information=file_information)

        new_state_entries.data = self._data_info
        return new_state_entries

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
        new_state_entries = {MonId.SPECTRUM: monitor_spectrum(spectrum=incident_monitor, is_trans=is_trans, interpolate=interpolate)}
        self.add_to_processed_state_settings(new_state_entries)

    def _apply_clean_if_required(self):
        """
        The cleans all commands up to the clean command point.

        We have to do this clean before we start processing the elements.
        """
        index_first_clean_command = None
        for index in reversed(list(range(0, len(self._commands)))):
            element = self._commands[index]
            if element.command_id == NParameterCommandId.CLEAN:
                index_first_clean_command = index
                break
        if index_first_clean_command is not None:
            del self._commands[0 : (index_first_clean_command + 1)]
            self._processed_state_settings = {}

    def _process_clean(self, command):
        _ = command
        raise RuntimeError(
            "Trying the process a Clean command. The clean command should have removed itself and "
            "all previous commands. If it is still here, then this is a bug"
        )

    def _process_reduction_dimensionality(self, command):
        _ = command
        reduction_dimensionality = command.values[0]
        new_state_entries = {OtherId.REDUCTION_DIMENSIONALITY: reduction_dimensionality}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_sample_offset(self, command):
        sample_offset = command.values[0]
        new_state_entries = {SampleId.OFFSET: sample_offset}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_detector(self, command):
        reduction_mode = command.values[0]
        new_state_entries = {DetectorId.REDUCTION_MODE: reduction_mode}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_gravity(self, command):
        use_gravity = command.values[0]
        extra_length = command.values[1]
        new_state_entries = {GravityId.ON_OFF: use_gravity, GravityId.EXTRA_LENGTH: extra_length}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_centre(self, command):
        pos1 = command.values[0]
        pos2 = command.values[1]
        detector_type = command.values[2]
        new_state_entries = {SetId.CENTRE: position_entry(pos1=pos1, pos2=pos2, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_trans_fit(self, command):
        def fit_type_to_data_type(fit_type_to_convert):
            return DataType.CAN if fit_type_to_convert is FitData.Can else DataType.SAMPLE

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
            new_state_entries.update(
                {
                    FitId.GENERAL: fit_general(
                        start=wavelength_low,
                        stop=wavelength_high,
                        fit_type=fit_type,
                        data_type=data_type,
                        polynomial_order=polynomial_order,
                    )
                }
            )
        self.add_to_processed_state_settings(new_state_entries)

    def _process_front_detector_rescale(self, command):
        scale = command.values[0]
        shift = command.values[1]
        fit_scale = command.values[2]
        fit_shift = command.values[3]
        q_min = command.values[4]
        q_max = command.values[5]

        # Set the scale and the shift
        new_state_entries = {DetectorId.RESCALE: scale, DetectorId.SHIFT: shift}

        # Set the fit for the scale
        new_state_entries.update({DetectorId.RESCALE_FIT: det_fit_range(start=q_min, stop=q_max, use_fit=fit_scale)})

        # Set the fit for shift
        new_state_entries.update({DetectorId.SHIFT_FIT: det_fit_range(start=q_min, stop=q_max, use_fit=fit_shift)})

        self.add_to_processed_state_settings(new_state_entries)

    def _process_event_slices(self, command):
        event_slice_value = command.values
        new_state_entries = {OtherId.EVENT_SLICES: event_binning_string_values(value=event_slice_value)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_flood_file(self, command):
        file_path = command.values[0]
        detector_type = command.values[1]
        new_state_entries = {MonId.FLAT: monitor_file(file_path=file_path, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_phi_limit(self, command):
        phi_min = command.values[0]
        phi_max = command.values[1]
        use_phi_mirror = command.values[2]
        new_state_entries = {LimitsId.ANGLE: mask_angle_entry(min=phi_min, max=phi_max, use_mirror=use_phi_mirror)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavelength_correction_file(self, command):
        file_path = command.values[0]
        detector_type = command.values[1]
        new_state_entries = {MonId.DIRECT: monitor_file(file_path=file_path, detector_type=detector_type)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_mask_radius(self, command):
        radius_min = command.values[0]
        radius_max = command.values[1]
        new_state_entries = {LimitsId.RADIUS: range_entry(start=radius_min, stop=radius_max)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavelength_limit(self, command):
        wavelength_low = command.values[0]
        wavelength_high = command.values[1]
        wavelength_step = command.values[2]
        wavelength_step_type = command.values[3]
        new_state_entries = {
            LimitsId.WAVELENGTH: simple_range(
                start=wavelength_low, stop=wavelength_high, step=wavelength_step, step_type=wavelength_step_type
            )
        }

        self.add_to_processed_state_settings(new_state_entries)

    def _process_wavrange_lists(self, wav_min, wav_max, existing_wavelength):
        """If a wavrange is given as lists of min/max values, then construct a binning string to set the
        wavelength ranges. Returns the wavelength step and step type; the latter is converted to a range type if
        the inputs are lists."""

        def is_set(val):
            return val is not None

        def is_list(val):
            return is_set(val) and isinstance(val, list)

        # If one input is a list, they must both be lists of the same length
        if is_list(wav_min) != is_list(wav_max):
            raise RuntimeError(
                "CommandInterfaceStateDirector: The lower and upper wavelength bounds must both be the same type (got"
                " a mixture of single value and list)"
            )
        if is_list(wav_min) and is_list(wav_max) and len(wav_min) != len(wav_max):
            raise RuntimeError("CommandInterfaceStateDirector: the wav_start and wav_end inputs must contain the same number of values")

        step = existing_wavelength.wavelength_interval.wavelength_step

        if not is_list(wav_min):
            return step, existing_wavelength.wavelength_step_type

        # Construct the binning string
        wav_pairs = []
        for a, b in zip(wav_min, wav_max):
            wav_pairs.append(str(a) + "-" + str(b))
        binning = ",".join(map(str, wav_pairs))
        self.add_to_processed_state_settings({OtherId.WAVELENGTH_RANGE: binning})

        return step, existing_wavelength.wavelength_step_type_range

    def _process_wavrange(self, command):
        wav_min = command.values[0]
        wav_max = command.values[1]
        full_wavelength_range = command.values[2]
        reduction_mode = command.values[3]

        # Update the lower and the upper wavelength values. Note that this is considered an incomplete setting, since
        # not step or step type have been specified. This means we need to update one of the processed commands, which
        # is not nice but the command interface forces us to do so. We take a copy of the last LimitsId.wavelength
        # entry, we copy it and then change the desired settings. This means it has to be set at this point, else
        # something is wrong
        if self._processed_state_obj and self._processed_state_obj.wavelength:
            existing_wavelength = self._processed_state_obj.wavelength
            new_wav_min = wav_min if wav_min else existing_wavelength.wavelength_interval.wavelength_full_range[0]
            new_wav_max = wav_max if wav_max else existing_wavelength.wavelength_interval.wavelength_full_range[1]

            if wav_min is not None or wav_max is not None:
                step, step_type = self._process_wavrange_lists(new_wav_min, new_wav_max, existing_wavelength)
                new_range = simple_range(start=new_wav_min, stop=new_wav_max, step=step, step_type=step_type)
                copied_entry = {LimitsId.WAVELENGTH: new_range}
                self.add_to_processed_state_settings(copied_entry)
        else:
            raise RuntimeError(
                "CommandInterfaceStateDirector: Setting the lower and upper wavelength bounds is not"
                " possible. We require also a step and step range"
            )

        if full_wavelength_range is not None:
            full_wavelength_range_entry = {OtherId.USE_FULL_WAVELENGTH_RANGE: full_wavelength_range}
            self.add_to_processed_state_settings(full_wavelength_range_entry)

        if reduction_mode is not None:
            reduction_mode_entry = {DetectorId.REDUCTION_MODE: reduction_mode}
            self.add_to_processed_state_settings(reduction_mode_entry)

    def _process_qxy_limit(self, command):
        q_min = command.values[0]
        q_max = command.values[1]
        q_step = command.values[2]
        new_state_entries = {LimitsId.QXY: q_xy_range(start=q_min, stop=q_max, step=q_step)}
        self.add_to_processed_state_settings(new_state_entries)

    def _process_compatibility_mode(self, command):
        use_compatibility_mode = command.values[0]
        new_state_entries = {OtherId.USE_COMPATIBILITY_MODE: use_compatibility_mode}
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
        new_state_entries = {
            DetectorId.CORRECTION_X: single_entry_with_detector(entry=x, detector_type=detector_type),
            DetectorId.CORRECTION_Y: single_entry_with_detector(entry=y, detector_type=detector_type),
            DetectorId.CORRECTION_Z: single_entry_with_detector(entry=z, detector_type=detector_type),
            DetectorId.CORRECTION_ROTATION: single_entry_with_detector(entry=rotation, detector_type=detector_type),
            DetectorId.CORRECTION_RADIUS: single_entry_with_detector(entry=radius, detector_type=detector_type),
            DetectorId.CORRECTION_TRANSLATION: single_entry_with_detector(entry=side, detector_type=detector_type),
            DetectorId.CORRECTION_X_TILT: single_entry_with_detector(entry=x_tilt, detector_type=detector_type),
            DetectorId.CORRECTION_Y_TILT: single_entry_with_detector(entry=y_tilt, detector_type=detector_type),
        }
        self.add_to_processed_state_settings(new_state_entries)

    def _process_save(self, command):
        save_algorithms = command.values[0]
        save_as_zero_error_free = command.values[1]
        new_state_entries = {OtherId.SAVE_TYPES: save_algorithms, OtherId.SAVE_AS_ZERO_ERROR_FREE: save_as_zero_error_free}
        self.add_to_processed_state_settings(new_state_entries, treat_list_as_element=True)

    def _process_user_specified_output_name(self, command):
        user_specified_output_name = command.values[0]
        new_state_entry = {OtherId.USER_SPECIFIED_OUTPUT_NAME: user_specified_output_name}
        self.add_to_processed_state_settings(new_state_entry)

    def _process_user_specified_output_name_suffix(self, command):
        user_specified_output_name_suffix = command.values[0]
        new_state_entry = {OtherId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX: user_specified_output_name_suffix}
        self.add_to_processed_state_settings(new_state_entry)

    def _process_use_reduction_mode_as_suffix(self, command):
        use_reduction_mode_as_suffix = command.values[0]
        new_state_entry = {OtherId.USE_REDUCTION_MODE_AS_SUFFIX: use_reduction_mode_as_suffix}
        self.add_to_processed_state_settings(new_state_entry)

    def remove_last_user_file(self):
        """
        Removes the last added user file from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(NParameterCommandId.USER_FILE)

    def remove_last_scatter_sample(self):
        """
        Removes the last added scatter sample from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.SAMPLE_SCATTER)

    def remove_last_sample_transmission_and_direct(self):
        """
        Removes the last added scatter transmission and direct from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.SAMPLE_TRANSMISSION)
        self._remove_last_element(DataCommandId.SAMPLE_DIRECT)

    def remove_last_scatter_can(self):
        """
        Removes the last added scatter can from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.CAN_SCATTER)

    def remove_last_can_transmission_and_direct(self):
        """
        Removes the last added can transmission and direct from the commands.

        See _remove_last_element for further explanation.
        """
        self._remove_last_element(DataCommandId.CAN_TRANSMISSION)
        self._remove_last_element(DataCommandId.CAN_DIRECT)

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
            del self._commands[index_to_remove]
        else:
            raise RuntimeError(
                "Tried to delete the last instance of {0}, but none was present in the list of " "commands".format(command_id)
            )
