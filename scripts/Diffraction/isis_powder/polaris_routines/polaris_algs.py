# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import numpy as np

from mantid.api import WorkspaceGroup
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
    if vanadium_ws.id() != "Workspace2D":
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
    else:
        mantid.SaveNexus(InputWorkspace=vanadium_ws, Filename=output_path, Append=False)


def generate_ts_pdf(run_number, focus_file_path, merge_banks=False, q_lims=None, cal_file_name=None):
    focused_ws = _obtain_focused_run(run_number, focus_file_path)
    pdf_output = mantid.ConvertUnits(InputWorkspace=focused_ws.name(), Target="MomentumTransfer")

    if merge_banks:
        group_bin_min = None
        group_bin_max = None
        group_bin_width = None
        for i in range(pdf_output.getNumberOfEntries()):
            x_array = pdf_output.getItem(i).readX(0)
            bin_min = x_array[0]
            bin_max = x_array[-1]
            bin_width = (x_array[-1] - x_array[0])/x_array.size
            binning = [bin_min, bin_width, bin_max]
            if not group_bin_min:
                group_bin_min = bin_min
                group_bin_max = bin_max
                group_bin_width = bin_width
            else:
                group_bin_min = min(group_bin_min, bin_min)
                group_bin_max = min(group_bin_max, bin_max)
                group_bin_width = min(group_bin_width, bin_width)
            fit_spectra = mantid.FitIncidentSpectrum(InputWorkspace=pdf_output.getItem(i),
                                                     BinningForFit=binning,
                                                     BinningForCalc=binning,
                                                     FitSpectrumWith="GaussConvCubicSpline")
            placzek_self_scattering = mantid.CalculatePlaczekSelfScattering(InputWorkspace=fit_spectra)
            cal_workspace = mantid.LoadCalFile(
                InputWorkspace=placzek_self_scattering,
                CalFileName=cal_file_name,
                Workspacename='cal_workspace',
                MakeOffsetsWorkspace=False,
                MakeMaskWorkspace=False)
            focused_correction = None
            for spec_index in range(placzek_self_scattering.getNumberHistograms()):
                if cal_workspace.dataY(spec_index)[0] == i + 1:
                    if focused_correction is None:
                        focused_correction = placzek_self_scattering.dataY(spec_index)
                    else:
                        focused_correction = np.add(focused_correction, placzek_self_scattering.dataY(spec_index))
            mantid.CreateWorkspace(DataX=placzek_self_scattering.dataX(0),
                                   DataY=focused_correction,
                                   Distribution=True,
                                   UnitX='MomentumTransfer',
                                   OutputWorkspace='focused_correction_ws')
            mantid.Rebin(InputWorkspace=pdf_output.getItem(i), OutputWorkspace='pdf_rebined', Params=binning)
            mantid.Rebin(InputWorkspace='focused_correction_ws', OutputWorkspace='focused_correction_ws', Params=binning)
            mantid.Subtract(LHSWorkspace='pdf_rebined', RHSWorkspace='focused_correction_ws', OutputWorkspace=pdf_output.getItem(i))
        binning = [group_bin_min, group_bin_width, group_bin_max]
        pdf_output = mantid.Rebin(InputWorkspace=pdf_output, Params=binning)
        pdf_output_combined = mantid.ConjoinSpectra(InputWorkspaces='pdf_output')
        mantid.MatchSpectra(InputWorkspace=pdf_output_combined, OutputWorkspace=pdf_output_combined, ReferenceSpectrum=1)
        if type(q_lims) == str:
            q_min = []
            q_max = []
            try:
                with open(q_lims, 'r') as f:
                    line_list = [line.rstrip('\n') for line in f]
                    for line in line_list[:-1]:
                        value_list = line.split()
                        q_min.append(value_list[2])
                        q_max.append(value_list[3])
            except IOError:
                raise RuntimeError("q_lims is not valid")
        elif type(q_lims) == list or type(q_lims) == np.ndarray:
            q_min = q_lims[0, :]
            q_max = q_lims[1, :]
        else:
            raise RuntimeError("q_lims is not valid")
        pdf_x_array = pdf_output_combined.readX(0)
        for i in range(q_min.size):
            q_min[i] = pdf_x_array[np.amin(np.where(pdf_x_array >= q_min[i]))]
            q_max[i] = pdf_x_array[np.amax(np.where(pdf_x_array <= q_max[i]))]
        bin_width = pdf_x_array[1] - pdf_x_array[0]
        pdf_output_combined = mantid.CropWorkspaceRagged(InputWorkspace=pdf_output_combined, XMin=q_min, XMax=q_max)
        pdf_output_combined = mantid.Rebin(InputWorkspace=pdf_output_combined, Params=[min(q_min), bin_width, max(q_max)])
        pdf_output_combined = mantid.SumSpectra(InputWorkspace=pdf_output_combined, WeightedSum=True, MultiplyBySpectra=False)
        pdf_fourier_transform = mantid.PDFFourierTransform(Inputworkspace=pdf_output_combined, InputSofQType="S(Q)",
                                                           PDFType="G(r)", Filter=True)
        return pdf_fourier_transform

    pdf_output = mantid.PDFFourierTransform(Inputworkspace=pdf_output, InputSofQType="S(Q)", PDFType="G(r)",
                                            Filter=True)
    pdf_output = mantid.RebinToWorkspace(WorkspaceToRebin=pdf_output, WorkspaceToMatch=pdf_output[4],
                                         PreserveEvents=True)
    return pdf_output


def _generate_grouped_ts_pdf(focused_data, q_lims, cal_file_name):
    group_bin_min = None
    group_bin_max = None
    group_bin_width = None
    for i in range(focused_data.getNumberOfEntries()):
        x_array = focused_data.getItem(i).readX(0)
        bin_min = x_array[0]
        bin_max = x_array[-1]
        bin_width = (x_array[-1] - x_array[0]) / x_array.size
        binning = [bin_min, bin_width, bin_max]
        if not group_bin_min:
            group_bin_min = bin_min
            group_bin_max = bin_max
            group_bin_width = bin_width
        else:
            group_bin_min = min(group_bin_min, bin_min)
            group_bin_max = min(group_bin_max, bin_max)
            group_bin_width = min(group_bin_width, bin_width)
        fit_spectra = mantid.FitIncidentSpectrum(InputWorkspace=focused_data.getItem(i),
                                                 BinningForFit=binning,
                                                 BinningForCalc=binning,
                                                 FitSpectrumWith="GaussConvCubicSpline")
        placzek_self_scattering = mantid.CalculatePlaczekSelfScattering(InputWorkspace=fit_spectra)
        cal_workspace = mantid.LoadCalFile(
            InputWorkspace=placzek_self_scattering,
            CalFileName=cal_file_name,
            Workspacename='cal_workspace',
            MakeOffsetsWorkspace=False,
            MakeMaskWorkspace=False)
        focused_correction = None
        for spec_index in range(placzek_self_scattering.getNumberHistograms()):
            if cal_workspace.dataY(spec_index)[0] == i + 1:
                if focused_correction is None:
                    focused_correction = placzek_self_scattering.dataY(spec_index)
                else:
                    focused_correction = np.add(focused_correction, placzek_self_scattering.dataY(spec_index))
        mantid.CreateWorkspace(DataX=placzek_self_scattering.dataX(0),
                               DataY=focused_correction,
                               Distribution=True,
                               UnitX='MomentumTransfer',
                               OutputWorkspace='focused_correction_ws')
        mantid.Rebin(InputWorkspace=focused_data.getItem(i), OutputWorkspace='focused_rebined', Params=binning)
        mantid.Rebin(InputWorkspace='focused_correction_ws', OutputWorkspace='focused_correction_ws', Params=binning)
        mantid.Subtract(LHSWorkspace='focused_rebined', RHSWorkspace='focused_correction_ws',
                        OutputWorkspace=focused_data.getItem(i))
    binning = [group_bin_min, group_bin_width, group_bin_max]
    focused_data = mantid.Rebin(InputWorkspace=focused_data, Params=binning)
    pdf_output_combined = mantid.ConjoinSpectra(InputWorkspaces=focused_data)
    mantid.MatchSpectra(InputWorkspace=pdf_output_combined, OutputWorkspace=pdf_output_combined, ReferenceSpectrum=1)
    if type(q_lims) == str:
        q_min = []
        q_max = []
        try:
            with open(q_lims, 'r') as f:
                line_list = [line.rstrip('\n') for line in f]
                for line in line_list[:-1]:
                    value_list = line.split()
                    q_min.append(value_list[2])
                    q_max.append(value_list[3])
        except IOError:
            raise RuntimeError("q_lims is not valid")
    elif type(q_lims) == list or type(q_lims) == np.ndarray:
        q_min = q_lims[0, :]
        q_max = q_lims[1, :]
    else:
        raise RuntimeError("q_lims is not valid")
    pdf_x_array = pdf_output_combined.readX(0)
    for i in range(q_min.size):
        q_min[i] = pdf_x_array[np.amin(np.where(pdf_x_array >= q_min[i]))]
        q_max[i] = pdf_x_array[np.amax(np.where(pdf_x_array <= q_max[i]))]
    bin_width = pdf_x_array[1] - pdf_x_array[0]
    focused_data_combined = mantid.CropWorkspaceRagged(InputWorkspace=pdf_output_combined, XMin=q_min, XMax=q_max)
    focused_data_combined = mantid.Rebin(InputWorkspace=pdf_output_combined, Params=[min(q_min), bin_width, max(q_max)])
    focused_data_combined = mantid.SumSpectra(InputWorkspace=pdf_output_combined, WeightedSum=True,
                                            MultiplyBySpectra=False)
    pdf_output = mantid.PDFFourierTransform(Inputworkspace=pdf_output_combined, InputSofQType="S(Q)",
                                                       PDFType="G(r)", Filter=True)
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
