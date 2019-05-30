# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from six import iterkeys
import warnings

import mantid.kernel as kernel
import mantid.simpleapi as mantid
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS


def cal_map_dictionary_key_helper(dictionary, key, append_to_error_message=None):
    """
    Provides a light wrapper around the dictionary key helper and uses case insensitive lookup.
    This also provides a generic error message stating the following key could not be found
    in the calibration mapping file. As several instruments will use this message it makes
    sense to localise it to common. If a message is passed in append_to_error_message it
    will append that to the end of the generic error message in its own line when an
    exception is raised.
    :param dictionary: The dictionary to search in for the key
    :param key: The key to search for
    :param append_to_error_message: (Optional) The message to append to the end of the error message
    :return: The found key if it exists
    """
    err_message = "The field '" + str(key) + "' is required within the calibration file but was not found."
    err_message += '\n' + str(append_to_error_message) if append_to_error_message else ''

    return dictionary_key_helper(dictionary=dictionary, key=key, throws=True,
                                 case_insensitive=True, exception_msg=err_message)


def crop_banks_using_crop_list(bank_list, crop_values_list):
    """
    Crops each bank by the specified tuple values from a list of tuples in TOF. The number
    of tuples must match the number of banks to crop. A list of [(100,200), (150,250)] would crop
    bank 1 to the values 100, 200 and bank 2 to 150 and 250 in TOF.
    :param bank_list: The list of workspaces each containing one bank of data to crop
    :param crop_values_list: The cropping values to apply to each bank
    :return: A list of cropped workspaces
    """
    if not isinstance(crop_values_list, list):
        raise ValueError("The cropping values were not in a list type")
    elif not isinstance(bank_list, list):
        # This error is probably internal as we control the bank lists
        raise RuntimeError("Attempting to use list based cropping on a single workspace not in a list")

    num_banks = len(bank_list)
    num_crop_vals = len(crop_values_list)

    # Finally check the number of elements are equal
    if num_banks != num_crop_vals:
        raise RuntimeError("The number of TOF cropping values does not match the number of banks for this instrument.\n"
                           "{} cropping windows were supplied for {} banks".format(num_crop_vals, num_banks))

    output_list = []
    for spectra, cropping_values in zip(bank_list, crop_values_list):
        output_list.append(crop_in_tof(ws_to_crop=spectra, x_min=cropping_values[0], x_max=cropping_values[-1]))

    return output_list


def crop_in_tof(ws_to_crop, x_min=None, x_max=None):
    """
    Crops workspaces to the specified minimum and maximum values in TOF. If x_min or x_max is
    not specified the lower/upper end of the workspace is not modified
    :param ws_to_crop: The workspace to apply the cropping. This can either be a list of single workspace.
    :param x_min: (Optional) The minimum value in TOF to crop to
    :param x_max: (Optional) The maximum value in TOF to crop to
    :return: The cropped workspace
    """
    if isinstance(ws_to_crop, list):
        cropped_ws = []
        for ws in ws_to_crop:
            cropped_ws.append(_crop_single_ws_in_tof(ws, x_max=x_max, x_min=x_min))
    else:
        cropped_ws = _crop_single_ws_in_tof(ws_to_crop, x_max=x_max, x_min=x_min)

    return cropped_ws


def dictionary_key_helper(dictionary, key, throws=True, case_insensitive=False, exception_msg=None):
    """
    Checks if the key is in the dictionary and performs various different actions if it is not depending on
    the user parameters. If set to not throw it will return none. Otherwise it will throw a custom user message
    or the default python exception depending on what the user has specified.
    :param dictionary: The dictionary to search for the key
    :param key: The key to search for in the dictionary
    :param throws: (Optional) Defaults to true, whether this should throw on a key not being present
    :param case_insensitive (Optional) Defaults to false, if set to true it accounts for mixed case but is O(n) time
    :param exception_msg: (Optional) The error message to print in the KeyError instead of the default Python message
    :return: The key if it was found, None if throws was set to false and the key was not found.
    """
    if key in dictionary:
        # Try to use hashing first
        return dictionary[key]

    # If we still couldn't find it use the O(n) method
    if case_insensitive:
        # Convert key to str
        lower_key = str(key).lower()
        for dict_key in iterkeys(dictionary):
            if str(dict_key).lower() == lower_key:
                # Found it
                return dictionary[dict_key]

    # It doesn't exist at this point lets go into our error handling
    if not throws:
        return None
    elif exception_msg:
        # Print user specified message
        raise KeyError(exception_msg)
    else:
        # Raise default python key error:
        this_throws = dictionary[key]
        return this_throws  # Never gets this far just makes linters happy


def extract_ws_spectra(ws_to_split):
    """
    Extracts individual spectra from the workspace into a list of workspaces. Each workspace will contain
    one of the spectra from the workspace and will have the form of "<ws_name>_<spectrum number>".
    :param ws_to_split: The workspace to split into individual workspaces
    :return: A list of extracted workspaces - one per spectra in the original workspace
    """
    num_spectra = ws_to_split.getNumberHistograms()
    spectra_bank_list = []
    for i in range(0, num_spectra):
        output_name = ws_to_split.name() + "-" + str(i + 1)
        # Have to use crop workspace as extract single spectrum struggles with the variable bin widths
        spectra_bank_list.append(mantid.CropWorkspace(InputWorkspace=ws_to_split, OutputWorkspace=output_name,
                                                      StartWorkspaceIndex=i, EndWorkspaceIndex=i))
    return spectra_bank_list


def generate_run_numbers(run_number_string):
    """
    Generates a list of run numbers as a list from the input. This input can be either a string or int type
    and uses the same syntax that Mantid supports i.e. 1-10 generates 1,2,3...9,10 inclusive and commas can specify
    breaks between runs
    :param run_number_string: The string or int to convert into a list of run numbers
    :return: A list of run numbers generated from the string
    """
    # Check its not a single run
    if isinstance(run_number_string, int):
        # Cast into a list and return
        return [run_number_string]
    elif isinstance(run_number_string, str) and run_number_string.isdigit():
        # We can let Python handle the conversion in this case
        return [int(run_number_string)]

    # If its a string we must parse it
    run_number_string = run_number_string.strip()
    run_boundaries = run_number_string.replace('_', '-')  # Accept either _ or - delimiters
    run_list = _run_number_generator(processed_string=run_boundaries)
    return run_list


def _generate_vanadium_name(vanadium_string, is_spline, *args):
    """
    :param vanadium_string: The number of the run being processed
    :param is_spline: True if the workspace to save out is a spline
    :param args: Any other strings to append to the filename
    :return: A filename for the vanadium
    """
    out_name = 'Van'
    if is_spline:
        out_name += 'Splined'

    out_name += '_' + str(vanadium_string)
    for passed_arg in args:
        if isinstance(passed_arg, list):
            for arg in passed_arg:
                out_name += '_' + str(arg)
        else:
            out_name += '_' + (str(passed_arg))

    out_name += ".nxs"
    return out_name


def generate_splined_name(vanadium_string, *args):
    """
    Generates a unique splined vanadium name which encapsulates
    any properties passed into this method so that the vanadium
    can be later loaded. This acts as a fingerprint for the vanadium
    as some properties (such as offset file used) can impact
    on the correct splined vanadium file to use.
    :param vanadium_string: The name of this vanadium run
    :param args: Any identifying properties to append to the name
    :return: The splined vanadium name
    """
    return _generate_vanadium_name(vanadium_string, True, *args)


def generate_unsplined_name(vanadium_string, *args):
    """
    Generates a unique unsplined vanadium name which encapsulates
    any properties passed into this method so that the vanadium
    can be later loaded. This acts as a fingerprint for the vanadium
    as some properties (such as offset file used) can impact
    on the correct splined vanadium file to use.
    :param vanadium_string: The name of this vanadium run
    :param args: Any identifying properties to append to the name
    :return: The splined vanadium name
    """
    return _generate_vanadium_name(vanadium_string, False, *args)


def get_first_run_number(run_number_string):
    """
    Takes a run number string and returns the first user specified run from that string
    :param run_number_string: The string to parse to find the first user rnu
    :return: The first run for the user input of runs
    """
    run_numbers = generate_run_numbers(run_number_string=run_number_string)

    if not run_numbers:
        raise RuntimeError("Attempted to load empty set of workspaces. Please input at least one valid run number")

    run_numbers = run_numbers[0]

    return run_numbers


def extract_single_spectrum(ws_to_process, spectrum_number_to_extract):
    """
    Extracts the monitor spectrum into its own individual workspaces from the input workspace
    based on the number of the spectrum given
    :param ws_to_process: The workspace to extract the monitor from
    :param spectrum_number_to_extract: The spectrum of the workspace to extract
    :return: The extracted monitor as a workspace
    """
    load_monitor_ws = mantid.ExtractSingleSpectrum(InputWorkspace=ws_to_process,
                                                   WorkspaceIndex=spectrum_number_to_extract)
    return load_monitor_ws


def keep_single_ws_unit(d_spacing_group, tof_group, unit_to_keep):
    """
    Takes variables to the output workspaces in d-spacing and TOF and removes one
    of them depending on what the user has selected as their unit to keep.
    If a workspace has been deleted it additionally deletes the variable.
    If a unit they want to keep has not been specified it does nothing.
    :param d_spacing_group: The output workspace group in dSpacing
    :param tof_group: The output workspace group in TOF
    :param unit_to_keep: The unit to keep from the WorkspaceUnits enum
    :return: None
    """
    if not unit_to_keep:
        # If they do not specify which unit to keep don't do anything
        return

    if unit_to_keep == WORKSPACE_UNITS.d_spacing:
        remove_intermediate_workspace(tof_group)
        del tof_group

    elif unit_to_keep == WORKSPACE_UNITS.tof:
        remove_intermediate_workspace(d_spacing_group)
        del d_spacing_group

    else:
        raise ValueError("The user specified unit to keep is unknown")


def load_current_normalised_ws_list(run_number_string, instrument, input_batching=None):
    """
    Loads a workspace using Mantid and then performs current normalisation on it. Additionally it will either
    load a range of runs individually or summed depending on the user specified behaviour queried from the instrument.
    This can behaviour can be overridden by using the optional parameter input_batching. For
    example if the caller must have the workspaces summed regardless of user selection. The input_batching must be
    from common_enums.InputBatchingEnum
    :param run_number_string: The run number string to turn into a list of run(s) to load
    :param instrument: The instrument to query for the behaviour regarding summing workspaces
    :param input_batching: (Optional) Used to override the user specified choice where a specific batching is required
    :return: The normalised workspace(s) as a list.
    """
    if not input_batching:
        input_batching = instrument._get_input_batching_mode()

    run_information = instrument._get_run_details(run_number_string=run_number_string)
    file_ext = run_information.file_extension
    raw_ws_list = _load_raw_files(run_number_string=run_number_string, instrument=instrument, file_ext=file_ext)

    if input_batching == INPUT_BATCHING.Summed and len(raw_ws_list) > 1:
        summed_ws = _sum_ws_range(ws_list=raw_ws_list)
        remove_intermediate_workspace(raw_ws_list)
        raw_ws_list = [summed_ws]

    instrument.mask_prompt_pulses_if_necessary(raw_ws_list)

    normalised_ws_list = _normalise_workspaces(ws_list=raw_ws_list, run_details=run_information,
                                               instrument=instrument)

    return normalised_ws_list


def rebin_workspace(workspace, new_bin_width, start_x=None, end_x=None):
    """
    Rebins the specified workspace with the specified new bin width. Allows the user
    to also set optionally the first and final bin boundaries of the histogram too.
    If the bin boundaries are not set they are preserved from the original workspace
    :param workspace: The workspace to rebin
    :param new_bin_width: The new bin width to use across the workspace
    :param start_x: (Optional) The first x bin to crop to
    :param end_x: (Optional) The final x bin to crop to
    :return: The rebinned workspace
    """

    # Find the starting and ending bin boundaries if they were not set
    if start_x is None:
        start_x = workspace.readX(0)[0]
    if end_x is None:
        end_x = workspace.readX(0)[-1]

    rebin_string = str(start_x) + ',' + str(new_bin_width) + ',' + str(end_x)
    workspace = mantid.Rebin(InputWorkspace=workspace, OutputWorkspace=workspace, Params=rebin_string)
    return workspace


def rebin_workspace_list(workspace_list, bin_width_list, start_x_list=None, end_x_list=None):
    """
    Rebins a list of workspaces with the specified bin widths in the list provided.
    The number of bin widths and workspaces in the list must match. Additionally if
    the optional parameters for start_x_list or end_x_list are provided these must
    have the same length too.
    :param workspace_list: The list of workspaces to rebin in place
    :param bin_width_list: The list of new bin widths to apply to each workspace
    :param start_x_list: The list of starting x boundaries to rebin to
    :param end_x_list: The list of ending x boundaries to rebin to
    :return: List of rebinned workspace
    """
    if not isinstance(workspace_list, list) or not isinstance(bin_width_list, list):
        raise RuntimeError("One of the types passed to rebin_workspace_list was not a list")

    ws_list_len = len(workspace_list)
    if ws_list_len != len(bin_width_list):
        raise ValueError("The number of bin widths found to rebin to does not match the number of banks")
    if start_x_list and len(start_x_list) != ws_list_len:
        raise ValueError("The number of starting bin values does not match the number of banks")
    if end_x_list and len(end_x_list) != ws_list_len:
        raise ValueError("The number of ending bin values does not match the number of banks")

    # Create a list of None types of equal length to make using zip iterator easy
    start_x_list = [None] * ws_list_len if start_x_list is None else start_x_list
    end_x_list = [None] * ws_list_len if end_x_list is None else end_x_list

    output_list = []
    for ws, bin_width, start_x, end_x in zip(workspace_list, bin_width_list, start_x_list, end_x_list):
        output_list.append(rebin_workspace(workspace=ws, new_bin_width=bin_width,
                                           start_x=start_x, end_x=end_x))

    return output_list


def remove_intermediate_workspace(workspaces):
    """
    Removes the specified workspace(s) from the ADS. Can accept lists of workspaces. It
    does not perform any checks whether the ADS contains those workspaces and it is up
    to the caller to ensure this is a safe operation
    :param workspaces: The workspace(s) either individual or a list of to remove from the ADS
    :return: None
    """
    if isinstance(workspaces, list):
        for ws in workspaces:
            mantid.DeleteWorkspace(ws)
    else:
        mantid.DeleteWorkspace(workspaces)


def run_normalise_by_current(ws):
    """
    Runs the Normalise By Current algorithm on the input workspace. If the workspace has no current, return it unchanged
    :param ws: The workspace to run normalise by current on
    :return: The current normalised workspace
    """
    if workspace_has_current(ws):
        ws = mantid.NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws)
    else:
        warnings.warn(
            "Run {} had no current. NormaliseByCurrent will not be run on it, and empty will not be subtracted".
            format(ws.getRunNumber()))
    return ws


def runs_overlap(run_string1, run_string2):
    """
    Get whether two runs, specified using the usual run string format (eg 123-125 referring to 123, 124 and 125)
    contain any individual runs in common
    """
    return len(set(generate_run_numbers(run_string1)).intersection(generate_run_numbers(run_string2))) > 0


def spline_vanadium_workspaces(focused_vanadium_spectra, spline_coefficient):
    """
    Returns a splined vanadium workspace from the focused vanadium bank list.
    This runs both StripVanadiumPeaks and SplineBackgrounds on the input
    workspace list and returns a list of the stripped and splined workspaces
    :param focused_vanadium_spectra: The vanadium workspaces to process
    :param spline_coefficient: The coefficient to use when creating the splined vanadium workspaces
    :return: The splined vanadium workspace
    """
    splined_workspaces = spline_workspaces(focused_vanadium_spectra, num_splines=spline_coefficient)
    return splined_workspaces


def spline_workspaces(focused_vanadium_spectra, num_splines):
    """
    Splines a list of workspaces in TOF and returns the splines in new workspaces in a
    list of said splined workspaces. The input workspaces should have any Bragg peaks
    masked before performing this step.
    :param focused_vanadium_spectra: The workspaces to spline as a list
    :param num_splines: The coefficient to use within SplineBackground
    :return: A list of splined workspaces
    """
    tof_ws_list = []
    for bank_index, ws in enumerate(focused_vanadium_spectra):
        out_name = "spline_bank_" + str(bank_index + 1)
        tof_ws_list.append(mantid.ConvertUnits(InputWorkspace=ws, Target="TOF", OutputWorkspace=out_name))

    splined_ws_list = []
    for ws in tof_ws_list:
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=ws, OutputWorkspace=ws, NCoeff=num_splines))

    return splined_ws_list


def subtract_summed_runs(ws_to_correct, empty_sample_ws_string, instrument, scale_factor=None):
    """
    Loads the list of empty runs specified by the empty_sample_ws_string and subtracts
    them from the workspace specified. Returns the subtracted workspace.
    :param ws_to_correct: The workspace to subtract the empty instrument runs from
    :param empty_sample_ws_string: The empty run numbers to subtract from the workspace
    :param instrument: The instrument object these runs belong to
    :param scale_factor: The percentage to scale the loaded runs by
    :return: The workspace with the empty runs subtracted
    """
    # Skip this step if an empty string was not specified
    # or if the workspace has no current, as subtracting empty would give us negative counts
    if empty_sample_ws_string is None or not workspace_has_current(ws_to_correct):
        return ws_to_correct

    empty_sample = load_current_normalised_ws_list(run_number_string=empty_sample_ws_string, instrument=instrument,
                                                   input_batching=INPUT_BATCHING.Summed)
    empty_sample = empty_sample[0]
    if scale_factor:
        empty_sample = mantid.Scale(InputWorkspace=empty_sample, OutputWorkspace=empty_sample, Factor=scale_factor,
                                    Operation="Multiply")
    try:
        mantid.Minus(LHSWorkspace=ws_to_correct, RHSWorkspace=empty_sample, OutputWorkspace=ws_to_correct)
    except ValueError:
        raise ValueError("The empty run(s) specified for this file do not have matching binning. Do the TOF windows of"
                         " the empty and sample match?")

    remove_intermediate_workspace(empty_sample)

    return ws_to_correct


def _crop_single_ws_in_tof(ws_to_rebin, x_max, x_min):
    """
    Implementation of cropping the single workspace in TOF. First converts to TOF, crops then converts
    back to the original unit.
    :param ws_to_rebin: The workspace to crop
    :param x_max: (Optional) The minimum TOF values to crop to
    :param x_min: (Optional) The maximum TOF values to crop to
    :return: The cropped workspace with the original units
    """
    previous_units = ws_to_rebin.getAxis(0).getUnit().unitID()
    if previous_units != "TOF":
        ws_to_rebin = mantid.ConvertUnits(InputWorkspace=ws_to_rebin, Target="TOF", OutputWorkspace=ws_to_rebin)
    cropped_ws = mantid.CropWorkspace(InputWorkspace=ws_to_rebin, OutputWorkspace=ws_to_rebin,
                                      XMin=x_min, XMax=x_max)
    if previous_units != "TOF":
        cropped_ws = mantid.ConvertUnits(InputWorkspace=cropped_ws, Target=previous_units, OutputWorkspace=cropped_ws)
    return cropped_ws


def _normalise_workspaces(ws_list, instrument, run_details):
    """
    Normalises the workspace list by current by calling the instrument implementation
    :param ws_list: A list of workspace to perform normalisation on
    :param instrument: The instrument these runs belong to
    :param run_details: The run details object associated to this run
    :return: The list of workspaces normalised by current.
    """
    output_list = []
    for ws in ws_list:
        output_list.append(instrument._normalise_ws_current(ws_to_correct=ws))

    return output_list


def _check_load_range(list_of_runs_to_load):
    """
    Checks the length of the list of runs which are about to be loaded to determine
    if they are larger than a threshold. Currently this is 1000 as it allows
    users to automatically process all runs within a cycle but detects if they
    have missed a digit which is usually 10,000 runs.
    :param list_of_runs_to_load: The generated run numbers to load as a list
    :return: None - If the list is greater than the specified maximum a ValueError is raised.
    """
    maximum_range_len = 1000  # If more than this number of runs is entered probably wrong
    if len(list_of_runs_to_load) > maximum_range_len:
        raise ValueError("More than " + str(maximum_range_len) + " runs were selected."
                         " Found " + str(len(list_of_runs_to_load)) + " Aborting.")


def _load_raw_files(run_number_string, instrument, file_ext=None):
    """
    Uses the run number string to generate a list of run numbers to load in
    :param run_number_string: The run number string to generate
    :param instrument: The instrument to generate the prefix filename for these runs
    :return: A list of loaded workspaces
    """
    run_number_list = generate_run_numbers(run_number_string=run_number_string)
    load_raw_ws = _load_list_of_files(run_number_list, instrument, file_ext=file_ext)
    return load_raw_ws


def _load_list_of_files(run_numbers_list, instrument, file_ext=None):
    """
    Loads files based on the list passed to it. If the list is
    greater than the maximum range it will raise an exception
    see _check_load_range for more details
    :param run_numbers_list: The list of runs to load
    :param instrument: The instrument to generate the prefix for
    :return: The loaded workspaces as a list
    """
    read_ws_list = []
    _check_load_range(list_of_runs_to_load=run_numbers_list)

    for run_number in run_numbers_list:
        file_name = instrument._generate_input_file_name(run_number=run_number)
        file_name = file_name + str(file_ext) if file_ext else file_name
        read_ws = mantid.Load(Filename=file_name)
        read_ws_list.append(mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=file_name))

    return read_ws_list


def _strip_vanadium_peaks(workspaces_to_strip):
    out_list = []
    for i, ws in enumerate(workspaces_to_strip):
        out_name = ws.name() + "_toSpline-" + str(i+1)
        out_list.append(mantid.StripVanadiumPeaks(InputWorkspace=ws, OutputWorkspace=out_name))
    return out_list


def _sum_ws_range(ws_list):
    """
    Sums a list of workspaces into a single workspace. This will take the name
    of the first and last workspaces in the list and take the form: "summed_<first>_<last>"
    :param ws_list: The workspaces as a list to sum into a single workspace
    :return: A single summed workspace
    """
    # Sum all workspaces
    out_ws_name = "summed_" + ws_list[0].name() + '_' + ws_list[-1].name()
    summed_ws = mantid.MergeRuns(InputWorkspaces=ws_list, OutputWorkspace=out_ws_name)
    return summed_ws


def _run_number_generator(processed_string):
    """
    Uses Mantid to generate a list of run numbers from a string input
    :param processed_string: The string representation of run numbers to convert into a list
    :return: A list of run numbers based on the input string
    """
    try:
        number_generator = kernel.IntArrayProperty('array_generator', processed_string)
        return number_generator.value.tolist()
    except RuntimeError:
        raise ValueError("Could not generate run numbers from this input: " + processed_string)


def generate_sample_geometry(sample_details):
    """
    Generates the expected input for sample geometry using the SampleDetails class
    :param sample_details: Instance of SampleDetails containing details about sample geometry and material
    :return: A map of the sample geometry
    """
    return {'Shape': 'Cylinder',
            'Height': sample_details.height(),
            'Radius': sample_details.radius(),
            'Center': sample_details.center()}


def generate_sample_material(sample_details):
    """
    Generates the expected input for sample material using the SampleDetails class
    :param sample_details: Instance of SampleDetails containing details about sample geometry and material
    :return: A map of the sample material
    """
    material = sample_details.material_object
    # See SetSampleMaterial for documentation on this dictionary
    material_json = {'ChemicalFormula': material.chemical_formula}
    if material.number_density:
        material_json["SampleNumberDensity"] = material.number_density
    if material.absorption_cross_section:
        material_json["AttenuationXSection"] = material.absorption_cross_section
    if material.scattering_cross_section:
        material_json["ScatteringXSection"] = material.scattering_cross_section

    return material_json


def workspace_has_current(ws):
    """
    Gat whether the total charge for this run was greater than 0
    """
    charge = ws.run().getProtonCharge()
    return charge is not None and charge > 0
