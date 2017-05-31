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

    last_results = None

    exit_iteration = 0

    for iteration in range(1, iterations + 1):
        iteration_flags = copy.deepcopy(flags)
        iteration_flags['iteration'] = iteration

        if last_results is not None:
            iteration_flags['masses'] = _update_masses_from_params(copy.deepcopy(flags['masses']),
                                                                   last_results[2])

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
        mass_values = _create_profile_strs_and_mass_list(copy.deepcopy(flags['masses'][0]))[0]
        profiles_strs = []
        for mass_spec in flags['masses']:
            profiles_strs.append(_create_profile_strs_and_mass_list(mass_spec)[1])
    else:
        mass_values, profiles_strs = _create_profile_strs_and_mass_list(flags['masses'])
    background_str = _create_background_str(flags.get('background', None))
    intensity_constraints = _create_intensity_constraint_str(flags['intensity_constraints'])
    ties = _create_user_defined_ties_str(flags['masses'])

    num_spec = sample_data.getNumberHistograms()
    pre_correct_pars_workspace = None
    pars_workspace = None
    fit_workspace = None
    max_fit_iterations = flags.get('max_fit_iterations', 5000)

    output_groups = []
    chi2_values = []
    data_workspaces = []
    result_workspaces = []
    group_name = runs + '_result'
    for index in range(num_spec):
        if isinstance(profiles_strs, list):
            profiles = profiles_strs[index]
        else:
            profiles = profiles_strs

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
                         Masses=mass_values,
                         MassProfiles=profiles,
                         Background=background_str,
                         IntensityConstraints=intensity_constraints,
                         Ties=ties,
                         OutputWorkspace=corrections_fit_name,
                         FitParameters=pre_correction_pars_name,
                         MaxIterations=max_fit_iterations,
                         Minimizer=flags['fit_minimizer'])
        ms.DeleteWorkspace(corrections_fit_name)
        corrections_args['FitParameters'] = pre_correction_pars_name

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
                              GammaBackground=flags.get('gamma_correct', False),
                              Masses=mass_values,
                              MassProfiles=profiles,
                              IntensityConstraints=intensity_constraints,
                              MultipleScattering=True,
                              GammaBackgroundScale=flags.get('fixed_gamma_scaling', 0.0),
                              ContainerScale=flags.get('fixed_container_scaling', 0.0),
                              **corrections_args)

        # Final fit
        fit_ws_name = runs + "_data" + suffix
        pars_name = runs + "_params" + suffix
        fit_result = ms.VesuvioTOFFit(InputWorkspace=corrected_data_name,
                                      WorkspaceIndex=0,
                                      Masses=mass_values,
                                      MassProfiles=profiles,
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

        spec_num_str = str(sample_data.getSpectrum(index).getSpectrumNo())
        current_spec = 'spectrum_' + spec_num_str

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


def _update_masses_from_params(old_masses, param_ws):
    """
    Update the masses flag based on the results of a fit.

    @param old_masses The existing masses dictionary
    @param param_ws The workspace to update from
    @return The modified mass dictionary
    """
    for mass in old_masses:
        for param in mass.keys():
            params_list = ['value', 'function', 'hermite_coeffs',
                           'k_free', 'sears_flag', 'width']
            if param.lower() not in params_list:
                del mass[param]
            elif param.lower() == 'width' and not isinstance(mass[param], list):
                del mass[param]

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
            mass_idx = int(param_re.group(1))
            if mass_idx >= num_masses:
                continue

            param_name = param_re.group(2).lower()
            if param_name == 'width' and isinstance(masses[spec_idx][mass_idx].get(param_name, None), list):
                masses[spec_idx][mass_idx][param_name][1] = param_ws.dataY(idx)[spec_idx]
            else:
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
    :return: A string to pass to the algorithm & a list of masses
    """
    mass_values, profiles = [], []
    for mass_prop in profile_flags:
        function_props = ["function={0}".format(mass_prop["function"])]
        del mass_prop["function"]
        for key, value in iteritems(mass_prop):
            if key == 'value':
                mass_values.append(value)
            else:
                function_props.append("{0}={1}".format(key, value))
        profiles.append(",".join(function_props))
    profiles = ";".join(profiles)

    return mass_values, profiles


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
