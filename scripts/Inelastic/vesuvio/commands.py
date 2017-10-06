# pylint: disable=too-many-arguments,invalid-name,too-many-locals,too-many-branches
"""
Defines functions and classes to start the processing of Vesuvio data.
The main entry point that most users should care about is fit_tof().
"""
from __future__ import (absolute_import, division, print_function)
from six import iteritems

import copy
import re
import numpy as np

from mantid import mtd
from mantid.api import (AnalysisDataService, WorkspaceFactory, TextAxis)
from mantid.kernel import MaterialBuilder
from vesuvio.instrument import VESUVIO

import mantid.simpleapi as ms


# --------------------------------------------------------------------------------
# Functions
# --------------------------------------------------------------------------------

def fit_tof(runs, flags, iterations=1, convergence_threshold=None):
    """
    The main entry point for user scripts fitting in TOF.

    :param runs: A string specifying the runs to process
    :param flags: A dictionary of flags to control the processing
    :param iterations: Maximum number of iterations to perform
    :param convergence_threshold: Maximum difference in the cost
                                  function at which the parameters are accepted
                                  to have converged
    :return: Tuple of (fitted workspace, fitted params, number of iterations
             performed)
    """
    if iterations < 1:
        raise ValueError('Must perform at least one iteration')

    # Load
    spectra = flags['spectra']
    fit_mode = flags['fit_mode']
    sample_data = load_and_crop_data(runs, spectra, flags['ip_file'],
                                     flags['diff_mode'], fit_mode,
                                     flags.get('bin_parameters', None))

    # Load container runs if provided
    container_data = None
    if flags.get('container_runs', None) is not None:
        container_data = load_and_crop_data(flags['container_runs'], spectra,
                                            flags['ip_file'],
                                            flags['diff_mode'], fit_mode,
                                            flags.get('bin_parameters', None))

    # Check if multiple scattering flags have been defined
    if 'ms_flags' in flags:

        # Check if hydrogen constraints have been defined
        if 'HydrogenConstraints' in flags['ms_flags']:
            flags['ms_flags']['HydrogenConstraints'] = \
                _parse_ms_hydrogen_constraints(flags['ms_flags']['HydrogenConstraints'])
        else:
            flags['ms_flags']['HydrogenConstraints'] = dict()
    else:
        raise RuntimeError("Multiple scattering flags not provided. Set the ms_flag, 'ms_enabled' "
                           "to false, in order to disable multiple scattering corrections.")

    if spectra == 'backward' or spectra == 'forward':
        flags['back_scattering'] = spectra == 'backward'
    else:
        try:
            first_spec = int(spectra.split("-")[0])
            back_banks = VESUVIO().backward_banks
            flags['back_scattering'] = any([lower <= first_spec <= upper for lower, upper in back_banks])
        except:
            raise RuntimeError("Invalid value given for spectrum range: Range must either be 'forward', "
                               "'backward' or specified with the syntax 'a-b'.")

    last_results = None

    exit_iteration = 0

    index_to_symbol_map = filter(lambda x: 'symbol' in x[1], enumerate(flags['masses']))
    index_to_symbol_map = {str(k): v['symbol'] for k, v in index_to_symbol_map}

    hydrogen_indices = set()
    if flags['back_scattering']:
        hydrogen_indices = {int(k) for k, _ in filter(lambda x: x[1] == 'H',
                                                      index_to_symbol_map.items())}

        symbols = set()
        for symbol in flags['ms_flags']['HydrogenConstraints']:
            symbols.add(symbol)
        for symbol in index_to_symbol_map.values():
            symbols.discard(symbol)

        if symbols:
            raise RuntimeError("HydrogenConstraints contains references to the following,"
                               " undefined masses: " + ','.join(symbols))
    flags['index_to_symbol_map'] = index_to_symbol_map

    for iteration in range(1, iterations + 1):
        iteration_flags = copy.deepcopy(flags)
        iteration_flags['iteration'] = iteration

        if last_results is not None:
            iteration_flags['masses'] = _update_masses_from_params(copy.deepcopy(flags['masses']),
                                                                   last_results[2],
                                                                   ignore_indices=hydrogen_indices)

        print("=== Iteration {0} out of a possible {1}".format(iteration, iterations))
        results = fit_tof_iteration(sample_data, container_data, runs, iteration_flags)
        exit_iteration += 1

        if last_results is not None and convergence_threshold is not None:
            last_chi2 = np.array(last_results[3])
            chi2 = np.array(results[3])
            chi2_delta = last_chi2 - chi2
            max_chi2_delta = np.abs(np.max(chi2_delta))
            print("Cost function change: {0}".format(max_chi2_delta))

            if max_chi2_delta <= convergence_threshold:
                print("Stopped at iteration {0} due to minimal change in cost function".format(exit_iteration))
                last_results = results
                break

        last_results = results

    return last_results[0], last_results[2], last_results[3], exit_iteration


def fit_tof_iteration(sample_data, container_data, runs, flags):
    """
    Performs a single iterations of the time of flight corrections and fitting
    workflow.

    :param sample_data: Loaded sample data workspaces
    :param container_data: Loaded container data workspaces
    :param runs: A string specifying the runs to process
    :param flags: A dictionary of flags to control the processing
    :return: Tuple of (workspace group name, pre correction fit parameters,
             final fit parameters, chi^2 values)
    """
    # Transform inputs into something the algorithm can understand
    if isinstance(flags['masses'][0], list):
        mass_values, _, all_mass_values, _ = \
            _create_profile_strs_and_mass_list(copy.deepcopy(flags['masses'][0]))

        profiles_strs, all_profiles_strs = [], []
        for mass_spec in flags['masses']:
            _, profiles_str, _, all_profiles_str = _create_profile_strs_and_mass_list(mass_spec)
            profiles_strs.append(profiles_str)
            all_profiles_strs.append(all_profiles_str)
    else:
        mass_values, profiles_strs, all_mass_values, all_profiles_strs = \
            _create_profile_strs_and_mass_list(flags['masses'])

    background_str = _create_background_str(flags.get('background', None))
    intensity_constraints = _create_intensity_constraint_str(flags['intensity_constraints'])
    ties = _create_user_defined_ties_str(flags['masses'])

    num_spec = sample_data.getNumberHistograms()
    gamma_correct = flags.get('gamma_correct', False)
    pre_correct_pars_workspace = None
    pars_workspace = None
    fit_workspace = None
    max_fit_iterations = flags.get('max_fit_iterations', 5000)
    index_to_symbol_map = flags.get('index_to_symbol_map', dict())
    back_scattering = flags.get('back_scattering', False)
    output_groups = []
    chi2_values = []
    data_workspaces = []
    result_workspaces = []
    group_name = runs + '_result'

    # Create function for retrieving profiles by index outside of loop,
    # to reduce for loops done inside loop.
    get_profiles = _create_get_profiles_function(all_profiles_strs,
                                                 profiles_strs,
                                                 back_scattering)

    fit_masses = mass_values if back_scattering else all_mass_values

    for index in range(num_spec):
        all_profiles, fit_profiles = get_profiles(index)

        suffix = _create_fit_workspace_suffix(index,
                                              sample_data,
                                              flags['fit_mode'],
                                              flags['spectra'],
                                              flags.get('iteration', None))

        # Corrections
        corrections_args = dict()

        # Need to do a fit first to obtain the parameter table
        pre_correction_pars_name = runs + "_params_pre_correction" + suffix
        corrections_fit_name = "__vesuvio_corrections_fit"
        ms.VesuvioTOFFit(InputWorkspace=sample_data,
                         WorkspaceIndex=index,
                         Masses=fit_masses,
                         MassProfiles=fit_profiles,
                         Background=background_str,
                         IntensityConstraints=intensity_constraints,
                         Ties=ties,
                         OutputWorkspace=corrections_fit_name,
                         FitParameters=pre_correction_pars_name,
                         MaxIterations=max_fit_iterations,
                         Minimizer=flags['fit_minimizer'])
        ms.DeleteWorkspace(corrections_fit_name)
        corrections_args['FitParameters'] = pre_correction_pars_name

        if flags.get('ms_enabled', True):
            # Add the multiple scattering arguments
            corrections_args.update(flags['ms_flags'])

        corrected_data_name = runs + "_tof_corrected" + suffix
        linear_correction_fit_params_name = runs + "_correction_fit_scale" + suffix

        if flags.get('output_verbose_corrections', False):
            corrections_args["CorrectionWorkspaces"] = runs + "_correction" + suffix
            corrections_args["CorrectedWorkspaces"] = runs + "_corrected" + suffix

        if container_data is not None:
            corrections_args["ContainerWorkspace"] = container_data

        ms.VesuvioCorrections(InputWorkspace=sample_data,
                              OutputWorkspace=corrected_data_name,
                              LinearFitResult=linear_correction_fit_params_name,
                              WorkspaceIndex=index,
                              GammaBackground=gamma_correct,
                              Masses=all_mass_values,
                              MassProfiles=all_profiles,
                              MassIndexToSymbolMap=index_to_symbol_map,
                              IntensityConstraints=intensity_constraints,
                              MultipleScattering=flags.get('ms_enabled', True),
                              GammaBackgroundScale=flags.get('fixed_gamma_scaling', 0.0),
                              ContainerScale=flags.get('fixed_container_scaling', 0.0),
                              **corrections_args)

        # Final fit
        fit_ws_name = runs + "_data" + suffix
        pars_name = runs + "_params" + suffix
        fit_result = ms.VesuvioTOFFit(InputWorkspace=corrected_data_name,
                                      WorkspaceIndex=0,
                                      Masses=fit_masses,
                                      MassProfiles=fit_profiles,
                                      Background=background_str,
                                      IntensityConstraints=intensity_constraints,
                                      Ties=ties,
                                      OutputWorkspace=fit_ws_name,
                                      FitParameters=pars_name,
                                      MaxIterations=max_fit_iterations,
                                      Minimizer=flags['fit_minimizer'])
        chi2_values.append(fit_result[-1])
        ms.DeleteWorkspace(corrected_data_name)

        # Process parameter tables
        if pre_correct_pars_workspace is None:
            pre_correct_pars_workspace = _create_param_workspace(num_spec,
                                                                 mtd[pre_correction_pars_name])

        if pars_workspace is None:
            pars_workspace = _create_param_workspace(num_spec, mtd[pars_name])

        if fit_workspace is None:
            fit_workspace = _create_param_workspace(num_spec, mtd[linear_correction_fit_params_name])

        spec_no = sample_data.getSpectrum(index).getSpectrumNo()
        current_spec = 'spectrum_' + str(spec_no)

        _update_fit_params(pre_correct_pars_workspace,
                           index, mtd[pre_correction_pars_name],
                           current_spec)
        _update_fit_params(pars_workspace, index,
                           mtd[pars_name], current_spec)

        _update_fit_params(fit_workspace, index, mtd[linear_correction_fit_params_name], current_spec)

        ms.DeleteWorkspace(pre_correction_pars_name)
        ms.DeleteWorkspace(pars_name)
        ms.DeleteWorkspace(linear_correction_fit_params_name)

        # Process spectrum group
        # Note the ordering of operations here gives the order in the WorkspaceGroup
        output_workspaces = []
        data_workspaces.append(fit_ws_name)
        if flags.get('output_verbose_corrections', False):
            output_workspaces += mtd[corrections_args["CorrectionWorkspaces"]].getNames()
            output_workspaces += mtd[corrections_args["CorrectedWorkspaces"]].getNames()
            ms.UnGroupWorkspace(corrections_args["CorrectionWorkspaces"])
            ms.UnGroupWorkspace(corrections_args["CorrectedWorkspaces"])

            for workspace in output_workspaces:

                group_name = runs + '_iteration_' + str(flags.get('iteration', None))
                name = group_name + '_' + workspace.split('_')[1] + '_' + workspace.split('_')[-1]
                result_workspaces.append(name)
                if index == 0:
                    ms.RenameWorkspace(InputWorkspace=workspace, OutputWorkspace=name)
                else:
                    ms.ConjoinWorkspaces(InputWorkspace1=name, InputWorkspace2=workspace)

        # Output the parameter workspaces
        params_pre_corr = runs + "_params_pre_correction_iteration_" + str(flags['iteration'])
        params_name = runs + "_params_iteration_" + str(flags['iteration'])
        fit_name = runs + "_correction_fit_scale_iteration_" + str(flags['iteration'])
        AnalysisDataService.Instance().addOrReplace(params_pre_corr, pre_correct_pars_workspace)
        AnalysisDataService.Instance().addOrReplace(params_name, pars_workspace)
        AnalysisDataService.Instance().addOrReplace(fit_name, fit_workspace)

    if result_workspaces:
        output_groups.append(ms.GroupWorkspaces(InputWorkspaces=result_workspaces,
                                                OutputWorkspace=group_name))

    if data_workspaces:
        output_groups.append(ms.GroupWorkspaces(InputWorkspaces=data_workspaces,
                                                OutputWorkspace=group_name + '_data'))
    else:
        output_groups.append(fit_ws_name)

    if len(output_groups) > 1:
        result_ws = output_groups
    else:
        result_ws = output_groups[0]

    return result_ws, pre_correct_pars_workspace, pars_workspace, chi2_values


def load_and_crop_data(runs, spectra, ip_file, diff_mode='single',
                       fit_mode='spectra', rebin_params=None):
    """
    @param runs The string giving the runs to load
    @param spectra A list of spectra to load
    @param ip_file A string denoting the IP file
    @param diff_mode Either 'double' or 'single'
    @param fit_mode If bank then the loading is changed to summing each bank to a separate spectrum
    @param rebin_params Rebin parameter string to rebin data by (no rebin if None)
    """
    instrument = VESUVIO()
    load_banks = (fit_mode == 'bank')
    output_name = _create_tof_workspace_suffix(runs, spectra)

    if load_banks:
        sum_spectra = True
        if spectra == "forward":
            bank_ranges = instrument.forward_banks
        elif spectra == "backward":
            bank_ranges = instrument.backward_banks
        else:
            raise ValueError("Fitting by bank requires selecting either 'forward' or 'backward' "
                             "for the spectra to load")
        bank_ranges = ["{0}-{1}".format(x, y) for x, y in bank_ranges]
        spectra = ";".join(bank_ranges)
    else:
        sum_spectra = False
        if spectra == "forward":
            spectra = "{0}-{1}".format(*instrument.forward_spectra)
        elif spectra == "backward":
            spectra = "{0}-{1}".format(*instrument.backward_spectra)

    if diff_mode == "double":
        diff_mode = "DoubleDifference"
    else:
        diff_mode = "SingleDifference"

    kwargs = {"Filename": runs,
              "Mode": diff_mode, "InstrumentParFile": ip_file,
              "SpectrumList": spectra, "SumSpectra": sum_spectra,
              "OutputWorkspace": output_name}
    full_range = ms.LoadVesuvio(**kwargs)
    tof_data = ms.CropWorkspace(InputWorkspace=full_range,
                                XMin=instrument.tof_range[0],
                                XMax=instrument.tof_range[1],
                                OutputWorkspace=output_name)

    if rebin_params is not None:
        tof_data = ms.Rebin(InputWorkspace=tof_data,
                            OutputWorkspace=output_name,
                            Params=rebin_params)

    return tof_data


# --------------------------------------------------------------------------------
# Private Functions
# --------------------------------------------------------------------------------

def _create_get_profiles_function(all_profiles_strs, profiles_strs, back_scattering):
    """
    Create a function which takes an index and returns a tuple of the corresponding
    mass profiles and fitting mass profiles, where the fitting mass profiles are equal
    to all mass profiles, unless back_scattering is true, in which case, hydrogen profiles
    are removed from fitting mass profiles.

    :param all_profiles_strs:   All mass profile strings.
    :param profiles_strs:       Equivalent to all_profiles_strs with hydrogen profiles
                                removed.
    :param back_scattering:     Whether back_scattering spectra are being fit
    :return:                    A function for retrieving a tuple of all mass profile
                                strings and fitting mass profile strings for a given
                                spectrum index.
    """

    if isinstance(profiles_strs, list):

        if back_scattering:
            def get_profiles(idx):
                return all_profiles_strs[idx], profiles_strs[idx]
        else:
            def get_profiles(idx):
                return all_profiles_strs[idx], all_profiles_strs[idx]
    else:

        if back_scattering:
            def get_profiles(_):
                return all_profiles_strs, profiles_strs
        else:
            def get_profiles(_):
                return all_profiles_strs, all_profiles_strs

    return get_profiles


def _parse_ms_hydrogen_constraints(constraints):
    """
    Parses the specified hydrogen constraints from a list of constraints,
    to a dictionary of the chemical symbols of each constraint mapped to
    their corresponding constraint properties (factor and weight).

    :param constraints: The hydrogen constraints to parse.
    :return:            A dictionary mapping chemical symbol to constraint
                        properties.
    :raise:             A RuntimeError if a constraint doesn't hasn't been
                        given an associated chemical symbol.
    """
    if not isinstance(constraints, dict):
        parsed = dict()

        try:
            for constraint in constraints:
                parsed.update(_parse_ms_hydrogen_constraint(constraint))
        except AttributeError:
            raise RuntimeError("HydrogenConstraints are incorrectly formatted.")

        return parsed

    try:
        return _parse_ms_hydrogen_constraint(constraints)
    except AttributeError:
        raise RuntimeError("HydrogenConstraints are incorrectly formatted.")


def _parse_ms_hydrogen_constraint(constraint):
    symbol = constraint.pop("symbol", None)

    if symbol is None:
        raise RuntimeError("Invalid hydrogen constraint: " +
                           str(constraint) +
                           " - No symbol provided")
    return {symbol: constraint}


def _update_masses_from_params(old_masses, param_ws, ignore_indices=set()):
    """
    Update the masses flag based on the results of a fit.

    @param old_masses The existing masses dictionary
    @param param_ws The workspace to update from
    @param ignore_indices A set of mass indices, where the associated masses
                          should be ignored when updating the specified old
                          masses from the specified parameter workspace.
    @return The modified mass dictionary
    """
    mass_indices = []

    for idx, mass in enumerate(old_masses):
        for param in mass.keys():
            params_list = ['symbol', 'value', 'function', 'hermite_coeffs',
                           'k_free', 'sears_flag', 'width']
            if param.lower() not in params_list:
                del mass[param]
            elif param.lower() == 'width' and not isinstance(mass[param], list):
                del mass[param]

        if idx not in ignore_indices:
            mass_indices.append(idx)

    masses = []
    num_masses = len(old_masses)
    for _ in range(param_ws.blocksize()):
        masses.append(copy.deepcopy(old_masses))

    function_regex = re.compile("f([0-9]+).([A-z0-9_]+)")

    for spec_idx in range(param_ws.blocksize()):

        for idx, param in enumerate(param_ws.getAxis(1).extractValues()):
            # Ignore the cost function
            if param == "Cost function value":
                continue

            param_re = function_regex.match(param)
            mass_idx = mass_indices[int(param_re.group(1))]

            if mass_idx >= num_masses:
                continue

            param_name = param_re.group(2).lower()
            if param_name == 'width' and isinstance(masses[spec_idx][mass_idx].get(param_name, None), list):
                masses[spec_idx][mass_idx][param_name][1] = param_ws.dataY(idx)[spec_idx]
            elif 'symbol' not in masses[spec_idx][mass_idx] or param_name != 'mass':
                masses[spec_idx][mass_idx][param_name] = param_ws.dataY(idx)[spec_idx]

    return masses


def _create_param_workspace(num_spec, param_table):
    num_params = param_table.rowCount()
    param_workspace = WorkspaceFactory.Instance().create("Workspace2D",
                                                         num_params, num_spec,
                                                         num_spec)

    x_axis = TextAxis.create(num_spec)
    param_workspace.replaceAxis(0, x_axis)

    vert_axis = TextAxis.create(num_params)
    for idx, param_name in enumerate(param_table.column('Name')):
        vert_axis.setLabel(idx, param_name)
    param_workspace.replaceAxis(1, vert_axis)

    return param_workspace


def _update_fit_params(params_ws, spec_idx, params_table, name):
    params_ws.getAxis(0).setLabel(spec_idx, name)
    for idx in range(params_table.rowCount()):
        params_ws.dataX(idx)[spec_idx] = spec_idx
        params_ws.dataY(idx)[spec_idx] = params_table.column('Value')[idx]
        params_ws.dataE(idx)[spec_idx] = params_table.column('Error')[idx]


def _create_tof_workspace_suffix(runs, spectra):
    return runs + "_" + spectra + "_tof"


def _create_fit_workspace_suffix(index, tof_data, fit_mode, spectra, iteration=None):
    if fit_mode == "bank":
        suffix = "_" + spectra + "_bank_" + str(index + 1)
    else:
        spectrum = tof_data.getSpectrum(index)
        suffix = "_spectrum_" + str(spectrum.getSpectrumNo())

    if iteration is not None:
        suffix += "_iteration_" + str(iteration)

    return suffix


def _create_profile_strs_and_mass_list(profile_flags):
    """
    Create a string suitable for the algorithms out of the mass profile flags
    and a list of mass values
    :param profile_flags: A list of dict objects for the mass profile flags
    :return: A tuple of list of masses, a string to pass to the algorithm and
             a dictionary containing a map from mass index to a symbol - where
             chemical formula was used to define a mass.
    """
    material_builder = MaterialBuilder()
    mass_values, profiles = [], []
    all_mass_values, all_profiles = [], []
    for idx, mass_prop in enumerate(profile_flags):
        function_name = ("function=%s," % mass_prop.pop('function'))
        function_props = ["{0}={1}".format(key, value) for key, value in mass_prop.items()]
        function_props = ("%s,%s" % (function_name, (','.join(function_props))))

        mass_value = mass_prop.pop('value', None)
        if mass_value is None:
            symbol = mass_prop.pop('symbol', None)

            if symbol is None:
                raise RuntimeError('Invalid mass specified - ' + str(mass_prop)
                                   + " - either 'value' or 'symbol' must be given.")

            try:
                if symbol == 'H':
                    mass = material_builder.setFormula(symbol).build().relativeMolecularMass()
                    all_mass_values.append(mass)
                    all_profiles.append(function_props)
                    continue

                mass_value = material_builder.setFormula(symbol).build().relativeMolecularMass()
            except BaseException as exc:
                raise RuntimeError('Error when parsing mass - ' + str(mass_prop) + ": "
                                   + "\n" + str(exc))

        all_mass_values.append(mass_value)
        mass_values.append(mass_value)
        all_profiles.append(function_props)
        profiles.append(function_props)

    profiles = ";".join(profiles)
    all_profiles = ";".join(all_profiles)
    return mass_values, profiles, all_mass_values, all_profiles


def _create_background_str(background_flags):
    """
    Create a string suitable for the algorithms out of the background flags
    :param background_flags: A dict for the background (can be None)
    :return: A string to pass to the algorithm
    """
    if background_flags:
        background_props = ["function={0}".format(background_flags["function"])]
        del background_flags["function"]
        for key, value in iteritems(background_flags):
            background_props.append("{0}={1}".format(key, value))
        background_str = ",".join(background_props)
    else:
        background_str = ""

    return background_str


def _create_intensity_constraint_str(intensity_constraints):
    """
    Create a string suitable for the algorithms out of the intensity constraint flags
    :param intensity_constraints: A list of lists for the constraints (can be None)
    :return: A string to pass to the algorithm
    """
    if intensity_constraints:
        if not isinstance(intensity_constraints[0], list):
            intensity_constraints = [intensity_constraints]
        # Make each element a string and then join them together
        intensity_constraints = [str(c) for c in intensity_constraints]
        intensity_constraints_str = ";".join(intensity_constraints)
    else:
        intensity_constraints_str = ""

    return intensity_constraints_str


def _create_user_defined_ties_str(masses):
    """
    Creates the internal ties for each mass profile as defined by the user to be used when fitting the data
    @param masses   :: The mass profiles for the data which contain the the ties
    @return         :: A string to be passed as the Ties input to fitting
    """
    user_defined_ties = []
    for index, mass in enumerate(masses):
        if 'ties' in mass:
            ties = mass['ties'].split(',')
            function_identifier = 'f' + str(index) + '.'
            for t in ties:
                tie_str = function_identifier + t
                equal_pos = tie_str.index('=') + 1
                tie_str = tie_str[:equal_pos] + function_identifier + tie_str[equal_pos:]
                user_defined_ties.append(tie_str)
    user_defined_ties = ','.join(user_defined_ties)
    return user_defined_ties
