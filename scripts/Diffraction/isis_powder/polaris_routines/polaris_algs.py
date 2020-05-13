# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math

import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup
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
                    sample_details=None, delta_r=None, delta_q=None, pdf_type="G(r)", lorch_filter=None,
                    freq_params=None):
    focused_ws = _obtain_focused_run(run_number, focus_file_path)
    focused_ws = mantid.ConvertUnits(InputWorkspace=focused_ws, Target="MomentumTransfer", EMode='Elastic')

    raw_ws = mantid.Load(Filename='POLARIS'+str(run_number)+'.nxs')
    sample_geometry = common.generate_sample_geometry(sample_details)
    sample_material = common.generate_sample_material(sample_details)
    self_scattering_correction = mantid.TotScatCalculateSelfScattering(InputWorkspace=raw_ws,
                                                                       CalFileName=cal_file_name,
                                                                       SampleGeometry=sample_geometry,
                                                                       SampleMaterial=sample_material)

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
    if delta_q:
        focused_ws = mantid.Rebin(InputWorkspace=focused_ws, Params=delta_q)
    if merge_banks:
        q_min, q_max = _load_qlims(q_lims)
        merged_ws = mantid.MatchAndMergeWorkspaces(InputWorkspaces=focused_ws, XMin=q_min, XMax=q_max,
                                                   CalculateScale=False)
        fast_fourier_filter(merged_ws, freq_params=freq_params)
        pdf_output = mantid.PDFFourierTransform(Inputworkspace="merged_ws", InputSofQType="S(Q)-1", PDFType=pdf_type,
                                                Filter=lorch_filter, DeltaR=delta_r,
                                                rho0=sample_details.material_object.crystal_density)
    else:
        for ws in focused_ws:
            fast_fourier_filter(ws, freq_params=freq_params)
        pdf_output = mantid.PDFFourierTransform(Inputworkspace='focused_ws', InputSofQType="S(Q)-1", PDFType=pdf_type,
                                                Filter=lorch_filter, DeltaR=delta_r,
                                                rho0=sample_details.material_object.crystal_density)
        pdf_output = mantid.RebinToWorkspace(WorkspaceToRebin=pdf_output, WorkspaceToMatch=pdf_output[4],
                                             PreserveEvents=True)
    common.remove_intermediate_workspace('self_scattering_correction')
    # Rename output ws
    if 'merged_ws' in locals():
        mantid.RenameWorkspace(InputWorkspace='merged_ws', OutputWorkspace=run_number + '_merged_Q')
    mantid.RenameWorkspace(InputWorkspace='focused_ws', OutputWorkspace=run_number+'_focused_Q')
    if isinstance(focused_ws, WorkspaceGroup):
        for i in range(len(focused_ws)):
            mantid.RenameWorkspace(InputWorkspace=focused_ws[i], OutputWorkspace=run_number+'_focused_Q_'+str(i+1))
    mantid.RenameWorkspace(InputWorkspace='pdf_output', OutputWorkspace=run_number+'_pdf_R')
    if isinstance(pdf_output, WorkspaceGroup):
        for i in range(len(pdf_output)):
            mantid.RenameWorkspace(InputWorkspace=pdf_output[i], OutputWorkspace=run_number+'_pdf_R_'+str(i+1))
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


def _load_qlims(q_lims):
    if isinstance(q_lims, str):
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
        except IOError as exc:
            raise RuntimeError("q_lims path is not valid: {}".format(exc))
    elif isinstance(q_lims, (list, tuple)) or isinstance(q_lims, np.ndarray):
        q_min = q_lims[0]
        q_max = q_lims[1]
    else:
        raise RuntimeError("q_lims type is not valid. Expected a string filename or an array.")
    return q_min, q_max


def _determine_chopper_mode(ws):
    if ws.getRun().hasProperty('Frequency'):
        frequency = ws.getRun()['Frequency'].timeAverageValue()
        print("Found chopper frequency of {} in log file.".format(frequency))
        if math.isclose(frequency, 50, abs_tol=1):
            print("Automatically chose Rietveld mode")
            return 'Rietveld', polaris_advanced_config.rietveld_focused_cropping_values
        if math.isclose(frequency, 0, abs_tol=1):
            print("Automatically chose PDF mode")
            return 'PDF', polaris_advanced_config.pdf_focused_cropping_values
    else:
        raise ValueError("Chopper frequency not in log data. Please specify a chopper mode")


def fast_fourier_filter(ws, freq_params=None):
    if freq_params:
        x_range = ws.dataX(0)
        q_max = x_range[-1]
        q_delta = (x_range[1] - x_range[0])
        r_min = freq_params[0]
        # If no maximum r is given a high r_max prevents loss of detail on the output.
        if len(freq_params) > 1:
            r_max = min(freq_params[1], 1000)
        else:
            r_max = 1000
        ws_name = str(ws)
        mantid.PDFFourierTransform(Inputworkspace=ws_name, OutputWorkspace=ws_name, SofQType="S(Q)-1", PDFType="G(r)",
                                   Filter=False, DeltaR=0.01, Rmax=r_max, Direction='Forward')
        mantid.PDFFourierTransform(Inputworkspace=ws_name, OutputWorkspace=ws_name, SofQType="S(Q)-1", PDFType="G(r)",
                                   Filter=False, Qmax=q_max, deltaQ=q_delta, Rmin=r_min, Rmax=r_max,
                                   Direction='Backward')
