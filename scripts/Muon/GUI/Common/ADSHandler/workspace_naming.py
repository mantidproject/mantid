# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re

group_str = "; Group; "
pair_str = "; Pair Asym; "
phaseQuad_str = '; PhaseQuad; '
PHASEQUAD_RE = '_Re_' # use _ on both sides to prevent accidental ID (e.g. Red)
PHASEQUAD_IM = "_Im_"
TF_ASYMMETRY_PREFIX = "TFAsymmetry"
REBIN_STR = 'Rebin'
FFT_STR = 'FFT'
MAXENT_STR = 'MaxEnt'


def get_raw_data_workspace_name(instrument, run, multi_period, period='1', workspace_suffix=' MA'):
    if multi_period:
        return _base_run_name(instrument, run) + "_raw_data" + "_period_" + period + workspace_suffix
    else:
        return _base_run_name(instrument, run) + "_raw_data" + workspace_suffix


def get_deadtime_data_workspace_name(instrument, run, workspace_suffix=' MA'):
    return _base_run_name(instrument, run) + "_deadtime" + workspace_suffix


def _base_run_name(instrument, run=None):
    return str(instrument) + run


def get_group_data_workspace_name(context, group_name, run, period_string, rebin):
    name = context.data_context._base_run_name(run) + group_str + group_name + "; Counts;"

    if rebin:
        name += "".join([' ', REBIN_STR, ';'])

    name += context.workspace_suffix

    return name


def get_group_asymmetry_name(context, group_name, run, period_string, rebin):
    name = context.data_context._base_run_name(run) + group_str + group_name + "; Asymmetry;"

    if rebin:
        name += "".join([' ', REBIN_STR, ';'])

    name += context.workspace_suffix
    return name


def get_pair_asymmetry_name(context, pair_name, run, rebin):
    name = context.data_context._base_run_name(run) + pair_str + pair_name + ";"

    if rebin:
        name += "".join([' ', REBIN_STR, ';'])

    name += context.workspace_suffix
    return name


def check_phasequad_name(group_or_pair):
    if (PHASEQUAD_IM in group_or_pair or PHASEQUAD_RE in group_or_pair):
        return True
    return False


def add_phasequad_extensions(pair_name):
    return pair_name+PHASEQUAD_RE+PHASEQUAD_IM


def get_pair_phasequad_name(context, pair_name, run, rebin):

    name = context.data_context._base_run_name(run) + phaseQuad_str + pair_name +";"

    if rebin:
        name += "".join([' ', REBIN_STR, ';'])

    name += context.workspace_suffix
    return name


def get_group_or_pair_from_name(name):
    if group_str in name:
        index = name.find(group_str) + len(group_str)
        end = name.find(";", index)
        group_found = name[index: end]
        return group_found.replace(" ", "")
    elif pair_str in name:
        index = name.find(pair_str) + len(pair_str)
        end = name.find(";", index)
        pair_found = name[index: end]
        return pair_found.replace(" ", "")
    elif phaseQuad_str in name:
        index = name.find(phaseQuad_str) + len(phaseQuad_str)
        end = name.find(";", index)
        pair_found = name[index: end]
        return pair_found.replace(" ", "")
    return ""


def get_group_asymmetry_unnorm_name(context, group_name, run, periods, rebin):
    return '__' + get_group_asymmetry_name(context, group_name, run, periods, rebin) + '_unnorm'


def get_base_data_directory(context, run):
    return context.data_context._base_run_name(run) + context.workspace_suffix + "/"


def get_raw_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + "; Raw Data" + context.workspace_suffix + "/"
    else:
        return context.data_context._base_run_name(run) + " Raw Data" + context.workspace_suffix + "/"


def get_phase_table_workspace_name(raw_workspace, forward_group, backward_group):
    workspace_name = raw_workspace.replace('_raw_data', '; PhaseTable')
    workspace_name += '; ' + forward_group + '; ' + backward_group
    return workspace_name


def get_base_run_name(run, instrument):
    if isinstance(run, int):
        return str(instrument) + str(run)
    else:
        return str(instrument) + run


def get_phase_table_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + ' Phase Tab' + workspace_suffix + '/'

    return group


def get_fft_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + "".join([' ', FFT_STR]) + workspace_suffix + '/'

    return group


def get_phase_quad_workspace_name(input_workspace, phase_table):
    return input_workspace.replace('_raw_data', phaseQuad_str) + ' ' + phase_table


def get_fitting_workspace_name(base_name):
    return base_name + '; fit_information'


def get_fft_workspace_name(input_workspace, imaginary_input_workspace):
    if imaginary_input_workspace:
        return 'FFT; Re ' + input_workspace + '; Im ' + imaginary_input_workspace
    else:
        return 'FFT; Re ' + input_workspace


def get_maxent_workspace_name(input_workspace):
    return input_workspace + "".join(['; ', MAXENT_STR])


def get_fft_component_from_workspace_name(input_workspace):
    if 'FD_Re' in input_workspace:
        fft_component = "".join([FFT_STR, ';', 'Re'])
    elif 'FD_Im' in input_workspace:
        fft_component = "".join([FFT_STR, ';', 'Im'])
    else:
        fft_component = "".join([FFT_STR, ';', 'Mod'])

    return fft_component


def get_run_number_from_workspace_name(workspace_name, instrument):
    run = re.findall(r'%s(\d+)' % instrument, workspace_name)
    if run:
        return run[0]


def get_run_numbers_as_string_from_workspace_name(workspace_name, instrument):
    # workspace_name of format "INST999; abc; def;" or "INST999_abc_def MA"
    # or for multiple runs "INST1-3,5; abc; def;" etc.
    # need to strip all parts except run numbers part
    name = workspace_name.split(' ')[0]
    name = name.split('_')[0]
    name = name.split(';')[0]
    runs = name.replace(instrument, '')
    return runs


def get_first_run_from_run_string(run_string):
    """
    run_string will only be in the format 1-2,5,7,9-11
    Return the first number only
    """
    return_string = run_string
    index = -1
    index_1 = run_string.find('-')
    index_2 = run_string.find(',')
    if index_1 != -1 and index_2 != -1:
        index = min(index_1, index_2)
    elif index_1 != -1:
        index = index_1
    elif index_2 != -1:
        index = index_2
    if index != -1:
        return_string = return_string[:index]
    return return_string


def get_maxent_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + "".join([' ', MAXENT_STR]) + workspace_suffix + '/'

    return group


def get_fit_workspace_directory(group_name, suffix, base_name, workspace_suffix):
    return base_name + '/' + group_name + workspace_suffix + '/' + group_name + suffix + workspace_suffix + '/'


def create_fitted_workspace_name(input_workspace_name, function_name):
    directory = input_workspace_name + '; Fitted;' + function_name + '/'
    name = input_workspace_name + '; Fitted;' + function_name + '; Workspace'

    return name, directory


def get_fit_function_name_from_workspace(workspace_name):
    workspace_labels = workspace_name.split(';')
    try:
        fitted_index = workspace_labels.index(" Fitted")
        function_name = workspace_labels[fitted_index + 1]
        return function_name.strip()
    except ValueError:
        return None


def create_multi_domain_fitted_workspace_name(input_workspace, function_name):
    directory = input_workspace + '; Fitted;' + function_name + '/'
    name = input_workspace + '+ ...; Fitted;' + function_name

    return name, directory


def create_parameter_table_name(input_workspace_name, function_name):
    directory = input_workspace_name + '; Fitted;' + function_name + '/'
    name = input_workspace_name + '; Fitted Parameters;' + function_name

    return name, directory


def remove_rebin_from_name(name):
    if REBIN_STR not in name:
        return name
    index =  [ch.start() for ch in re.finditer(r";",name)]
    return name[:index[-2]]+name[index[-1]:]


def add_rebin_to_name(name):
    if REBIN_STR in name:
        return name
    index =  [ch.start() for ch in re.finditer(r";",name)]
    return name[:index[-1]]+"; "+REBIN_STR+name[index[-1]:]
