# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import numpy as np

import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.common_enums import WORKSPACE_UNITS
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_van_absorb_corrections(ws_to_correct, multiple_scattering, is_vanadium):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(1, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    sample_details_obj = absorb_corrections.create_vanadium_sample_details_obj(config_dict=absorb_dict)
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, sample_details_obj=sample_details_obj,
        is_vanadium=is_vanadium)
    return ws_to_correct


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key,
                                                append_to_error_message=err_message)


def _get_current_mode_dictionary(run_number_string, inst_settings):
    mapping_dict = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    if inst_settings.mode is None:
        ws = mantid.Load('POLARIS'+run_number_string+'.nxs')
        mode, cropping_vals = _determine_chopper_mode(ws)
        inst_settings.mode = mode
        inst_settings.focused_cropping_values = cropping_vals
        mantid.DeleteWorkspace(ws)
    # Get the current mode "Rietveld" or "PDF" run numbers
    return common.cal_map_dictionary_key_helper(mapping_dict, inst_settings.mode)


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    mode_run_numbers = _get_current_mode_dictionary(run_number_string, inst_settings)

    # Get empty and vanadium
    err_message = "this must be under the relevant Rietveld or PDF mode."

    empty_runs = common.cal_map_dictionary_key_helper(mode_run_numbers,
                                                      key="empty_run_numbers", append_to_error_message=err_message)
    vanadium_runs = common.cal_map_dictionary_key_helper(mode_run_numbers, key="vanadium_run_numbers",
                                                         append_to_error_message=err_message)

    grouping_file_name = inst_settings.grouping_file_name

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium_run, empty_run_number=empty_runs,
                                     vanadium_string=vanadium_runs, grouping_file_name=grouping_file_name)


def save_unsplined_vanadium(vanadium_ws, output_path):
    converted_workspaces = []
    for ws_index in range(vanadium_ws.getNumberOfEntries()):
        ws = vanadium_ws.getItem(ws_index)
        previous_units = ws.getAxis(0).getUnit().unitID()

        if previous_units != WORKSPACE_UNITS.tof:
            ws = mantid.ConvertUnits(InputWorkspace=ws, Target=WORKSPACE_UNITS.tof)

        ws = mantid.RenameWorkspace(InputWorkspace=ws, OutputWorkspace="van_bank_{}".format(ws_index + 1))
        converted_workspaces.append(ws)

    converted_group = mantid.GroupWorkspaces(",".join(ws.name() for ws in converted_workspaces))
    mantid.SaveNexus(InputWorkspace=converted_group, Filename=output_path, Append=False)
    mantid.DeleteWorkspace(converted_group)


def generate_ts_pdf(run_number, focus_file_path, merge_banks=False, q_lims=None, cal_file_name=None,
                    sample_details=None):
    focused_ws = _obtain_focused_run(run_number, focus_file_path)
    focused_ws = mantid.ConvertUnits(InputWorkspace=focused_ws, Target="MomentumTransfer", EMode='Elastic')
    self_scattering_correction = _calculate_self_scattering_correction(run_number, cal_file_name, sample_details)

    ws_group_list = []
    for i in range(self_scattering_correction.getNumberHistograms()):
        ws_name = 'correction_' + str(i)
        mantid.ExtractSpectra(InputWorkspace=self_scattering_correction, OutputWorkspace=ws_name,
                              WorkspaceIndexList=[i])
        ws_group_list.append(ws_name)
    self_scattering_correction = mantid.GroupWorkspaces(InputWorkspaces=ws_group_list)
    self_scattering_correction = mantid.RebinToWorkspace(WorkspaceToRebin=self_scattering_correction,
                                                         WorkspaceToMatch=focused_ws)
    focused_ws = mantid.Subtract(LHSWorkspace=focused_ws, RHSWorkspace=self_scattering_correction)

    if merge_banks:
        merged_ws = _merge_workspace_with_limits(focused_ws, q_lims)
        pdf_output = mantid.PDFFourierTransform(Inputworkspace=merged_ws, InputSofQType="S(Q)-1", PDFType="G(r)",
                                                Filter=True)
    else:
        pdf_output = mantid.PDFFourierTransform(Inputworkspace='focused_ws', InputSofQType="S(Q)-1",
                                                PDFType="G(r)", Filter=True)
        pdf_output = mantid.RebinToWorkspace(WorkspaceToRebin=pdf_output, WorkspaceToMatch=pdf_output[4],
                                             PreserveEvents=True)
    common.remove_intermediate_workspace('self_scattering_correction')
    return pdf_output


def _obtain_focused_run(run_number, focus_file_path):
    """
    Searches for the focused workspace to use (based on user specified run number) in the ADS and then the output
    directory.
    If unsuccessful, a ValueError exception is thrown.
    :param run_number: The run number to search for.
    :param focus_file_path: The expected file path for the focused file.
    :return: The focused workspace.
    """
    # Try the ADS first to avoid undesired loading
    if mantid.mtd.doesExist('%s-Results-TOF-Grp' % run_number):
        focused_ws = mantid.mtd['%s-Results-TOF-Grp' % run_number]
    elif mantid.mtd.doesExist('%s-Results-D-Grp' % run_number):
        focused_ws = mantid.mtd['%s-Results-D-Grp' % run_number]
    else:
        # Check output directory
        print('No loaded focused files found. Searching in output directory...')
        try:
            focused_ws = mantid.LoadNexus(Filename=focus_file_path, OutputWorkspace='focused_ws').OutputWorkspace
        except ValueError:
            raise ValueError("Could not find focused file for run number:%s\n"
                             "Please ensure a focused file has been produced and is located in the output directory."
                             % run_number)
    return focused_ws


def _merge_workspace_with_limits(focused_ws, q_lims):
    min_x = np.inf
    max_x = -np.inf
    num_x = -np.inf
    for ws in focused_ws:
        x_data = ws.dataX(0)
        min_x = min(np.min(x_data), min_x)
        max_x = max(np.max(x_data), max_x)
        num_x = max(x_data.size, num_x)
    width_x = (max_x-min_x)/num_x
    binning = [min_x, width_x, max_x]
    focused_ws = mantid.Rebin(InputWorkspace=focused_ws, Params=binning)
    focused_ws_conjoined = mantid.ConjoinSpectra(InputWorkspaces=focused_ws)

    if type(q_lims) == str:
        q_min = []
        q_max = []
        try:
            with open(q_lims, 'r') as f:
                line_list = [line.rstrip('\n') for line in f]
                for line in line_list[1:]:
                    value_list = line.split()
                    q_min.append(float(value_list[2]))
                    q_max.append(float(value_list[3]))
            q_min = np.array(q_min)
            q_max = np.array(q_max)
        except IOError:
            raise RuntimeError("q_lims directory is not valid")
    elif type(q_lims) == list or type(q_lims) == np.ndarray:
        q_min = q_lims[0, :]
        q_max = q_lims[1, :]
    else:
        raise RuntimeError("q_lims type is not valid")
    bin_width = np.inf
    for i in range(q_min.size):
        pdf_x_array = focused_ws_conjoined.readX(i)
        tmp1 = np.where(pdf_x_array >= q_min[i])
        tmp2 = np.amin(tmp1)
        q_min[i] = pdf_x_array[tmp2]
        q_max[i] = pdf_x_array[np.amax(np.where(pdf_x_array <= q_max[i]))]
        bin_width = min(pdf_x_array[1] - pdf_x_array[0], bin_width)
    # TODO what is the best spectra to use to match in any case, how can we always pick the correct one
    mantid.MatchSpectra(InputWorkspace=focused_ws_conjoined,
                        OutputWorkspace=focused_ws_conjoined,
                        ReferenceSpectrum=5)
    focused_data_combined = mantid.CropWorkspaceRagged(InputWorkspace=focused_ws_conjoined, XMin=q_min, XMax=q_max)
    focused_data_combined = mantid.Rebin(InputWorkspace=focused_data_combined,
                                         Params=[min(q_min), bin_width, max(q_max)])
    focused_data_combined = mantid.SumSpectra(InputWorkspace=focused_data_combined,
                                              WeightedSum=True,
                                              MultiplyBySpectra=False)
    common.remove_intermediate_workspace(focused_ws_conjoined)
    return focused_data_combined


def _calculate_self_scattering_correction(run_number, cal_file_name, sample_details):
    raw_ws = mantid.Load(Filename='POLARIS'+str(run_number)+'.nxs')
    mantid.SetSample(InputWorkspace=raw_ws,
                     Geometry=common.generate_sample_geometry(sample_details),
                     Material=common.generate_sample_material(sample_details))
    # find the closest monitor to the sample for incident spectrum
    raw_spec_info = raw_ws.spectrumInfo()
    incident_index = None
    for i in range(raw_spec_info.size()):
        if raw_spec_info.isMonitor(i):
            l2 = raw_spec_info.position(i)[2]
            if not incident_index:
                incident_index = i
            else:
                if raw_spec_info.position(incident_index)[2] < l2 < 0:
                    incident_index = i
    monitor = mantid.ExtractSpectra(InputWorkspace=raw_ws, WorkspaceIndexList=[incident_index])
    monitor = mantid.ConvertUnits(InputWorkspace=monitor, Target="Wavelength")
    x_data = monitor.dataX(0)
    min_x = np.min(x_data)
    max_x = np.max(x_data)
    width_x = (max_x - min_x) / x_data.size
    fit_spectra = mantid.FitIncidentSpectrum(InputWorkspace=monitor,
                                             BinningForCalc=[min_x, 1 * width_x, max_x],
                                             BinningForFit=[min_x, 10 * width_x, max_x],
                                             FitSpectrumWith="CubicSpline")
    self_scattering_correction = mantid.CalculatePlaczekSelfScattering(InputWorkspace=raw_ws,
                                                                       IncidentSpecta=fit_spectra)
    cal_workspace = mantid.LoadCalFile(InputWorkspace=self_scattering_correction,
                                       CalFileName=cal_file_name,
                                       Workspacename='cal_workspace',
                                       MakeOffsetsWorkspace=False,
                                       MakeMaskWorkspace=False)
    self_scattering_correction = mantid.DiffractionFocussing(InputWorkspace=self_scattering_correction,
                                                             GroupingFilename=cal_file_name)
    n_pixel = np.zeros(self_scattering_correction.getNumberHistograms())
    for i in range(cal_workspace.getNumberHistograms()):
        grouping = cal_workspace.dataY(i)
        if grouping[0] > 0:
            n_pixel[int(grouping[0] - 1)] += 1
    correction_ws = mantid.CreateWorkspace(DataY=n_pixel, DataX=[0, 1],
                                           NSpec=self_scattering_correction.getNumberHistograms())
    self_scattering_correction = mantid.Divide(LHSWorkspace=self_scattering_correction, RHSWorkspace=correction_ws)
    mantid.ConvertToDistribution(Workspace=self_scattering_correction)
    self_scattering_correction = mantid.ConvertUnits(InputWorkspace=self_scattering_correction,
                                                     Target="MomentumTransfer", EMode='Elastic')
    common.remove_intermediate_workspace('cal_workspace_group')
    common.remove_intermediate_workspace(correction_ws)
    common.remove_intermediate_workspace(fit_spectra)
    common.remove_intermediate_workspace(monitor)
    common.remove_intermediate_workspace(raw_ws)
    return self_scattering_correction


def _determine_chopper_mode(ws):
    if ws.getRun().hasProperty('Frequency'):
        frequency = ws.getRun()['Frequency'].lastValue()
        print("No chopper mode provided")
        if frequency == 50:
            print("automatically chose Rietveld")
            return 'Rietveld', polaris_advanced_config.rietveld_focused_cropping_values
        if frequency == 0:
            print("automatically chose PDF")
            return 'PDF', polaris_advanced_config.pdf_focused_cropping_values
    else:
        raise ValueError("Chopper frequency not in log data. Please specify a chopper mode")
