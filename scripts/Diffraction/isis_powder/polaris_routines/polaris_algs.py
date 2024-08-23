# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math
from typing import Union

import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup, MatrixWorkspace
from mantid.kernel import MaterialBuilder
from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_van_absorb_corrections(ws_to_correct, multiple_scattering, is_vanadium, msevents):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(1, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    sample_details_obj = absorb_corrections.create_vanadium_sample_details_obj(config_dict=absorb_dict)
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct,
        multiple_scattering=multiple_scattering,
        sample_details_obj=sample_details_obj,
        is_vanadium=is_vanadium,
        msevents=msevents,
    )
    return ws_to_correct


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key, append_to_error_message=err_message)


def _get_current_mode_dictionary(run_number_string, inst_settings):
    mapping_dict = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    if inst_settings.mode is None:
        ws = mantid.Load("POLARIS" + run_number_string)
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

    empty_runs = common.cal_map_dictionary_key_helper(mode_run_numbers, key="empty_run_numbers", append_to_error_message=err_message)
    vanadium_runs = common.cal_map_dictionary_key_helper(mode_run_numbers, key="vanadium_run_numbers", append_to_error_message=err_message)

    grouping_file_name = inst_settings.grouping_file_name

    return create_run_details_object(
        run_number_string=run_number_string,
        inst_settings=inst_settings,
        is_vanadium_run=is_vanadium_run,
        empty_inst_run_number=empty_runs,
        vanadium_string=vanadium_runs,
        grouping_file_name=grouping_file_name,
    )


def generate_ts_pdf(
    run_number,
    focus_file_path,
    sample_details,
    placzek_order,
    sample_temp,
    merge_banks=False,
    q_lims=None,
    cal_file_name=None,
    delta_r=None,
    delta_q=None,
    pdf_type="G(r)",
    lorch_filter=None,
    freq_params=None,
    per_detector=False,
    debug=False,
    pdf_output_name=None,
):
    if sample_details is None:
        raise RuntimeError(
            "A SampleDetails object was not set. Please create a SampleDetails object and set the "
            "relevant properties it. Then set the new sample by calling set_sample_details()"
        )
    focused_ws = _obtain_focused_run(run_number, focus_file_path)
    focused_ws = mantid.ConvertUnits(InputWorkspace=focused_ws, Target="MomentumTransfer", EMode="Elastic")

    if not per_detector:
        # per bank routine
        # in the per_detector routine, the placzek (and vanadium) corrections were applied before focussing
        focused_ws = apply_placzek_correction_per_bank(focused_ws, run_number, sample_details, cal_file_name, placzek_order, sample_temp)
    if debug:
        dcs_corrected = mantid.CloneWorkspace(InputWorkspace=focused_ws)

    # convert diff cross section to S(Q) - 1
    material_builder = MaterialBuilder()
    sample = material_builder.setFormula(sample_details.material_object.chemical_formula).build()
    sample_total_scatter_cross_section = sample.totalScatterXSection()
    sample_coh_scatter_cross_section = sample.cohScatterXSection()
    focused_ws = focused_ws - sample_total_scatter_cross_section / (4 * math.pi)
    focused_ws = focused_ws * 4 * math.pi / sample_coh_scatter_cross_section
    if debug:
        s_of_q_minus_one = mantid.CloneWorkspace(InputWorkspace=focused_ws)

    if delta_q:
        focused_ws = mantid.Rebin(InputWorkspace=focused_ws, Params=delta_q)
    if merge_banks:
        q_min, q_max = _load_qlims(q_lims)
        merged_ws = mantid.MatchAndMergeWorkspaces(InputWorkspaces=focused_ws, XMin=q_min, XMax=q_max, CalculateScale=False)
        fast_fourier_filter(merged_ws, rho0=sample_details.material_object.number_density, freq_params=freq_params)
        pdf_output = mantid.PDFFourierTransform(
            Inputworkspace="merged_ws",
            InputSofQType="S(Q)-1",
            PDFType=pdf_type,
            Filter=lorch_filter,
            DeltaR=delta_r,
            rho0=sample_details.material_object.number_density,
        )
    else:
        for ws in focused_ws:
            fast_fourier_filter(ws, rho0=sample_details.material_object.number_density, freq_params=freq_params)
        pdf_output = mantid.PDFFourierTransform(
            Inputworkspace="focused_ws",
            InputSofQType="S(Q)-1",
            PDFType=pdf_type,
            Filter=lorch_filter,
            DeltaR=delta_r,
            rho0=sample_details.material_object.number_density,
        )
        pdf_output = mantid.RebinToWorkspace(WorkspaceToRebin=pdf_output, WorkspaceToMatch=pdf_output[4], PreserveEvents=True)
    if not per_detector and not debug:
        common.remove_intermediate_workspace("self_scattering_correction")
    # Rename output ws
    if "merged_ws" in locals():
        mantid.RenameWorkspace(InputWorkspace="merged_ws", OutputWorkspace=run_number + "_merged_Q")

    mantid.RenameWorkspace(InputWorkspace="focused_ws", OutputWorkspace=run_number + "_focused_Q")
    if isinstance(focused_ws, WorkspaceGroup):
        target_focus_ws_name = run_number + "_focused_Q_"
        for i in range(len(focused_ws)):
            if str(focused_ws[i]) != (target_focus_ws_name + str(i + 1)):
                mantid.RenameWorkspace(InputWorkspace=focused_ws[i], OutputWorkspace=target_focus_ws_name + str(i + 1))

    target_pdf_ws_name = f"{run_number}_pdf_{pdf_type}" if not pdf_output_name else pdf_output_name
    mantid.RenameWorkspace(InputWorkspace="pdf_output", OutputWorkspace=target_pdf_ws_name)
    if isinstance(pdf_output, WorkspaceGroup):
        for i in range(len(pdf_output)):
            if str(pdf_output[i]) != (target_pdf_ws_name + str(i + 1)):
                mantid.RenameWorkspace(InputWorkspace=pdf_output[i], OutputWorkspace=f"{target_pdf_ws_name}_{str(i + 1)}")
    return pdf_output


def _obtain_focused_run(run_number, focus_file_path) -> Union[MatrixWorkspace, WorkspaceGroup]:
    """
    Searches for the focused workspace to use (based on user specified run number) in the ADS and then the output
    directory.
    If unsuccessful, a ValueError exception is thrown.
    :param run_number: The run number to search for.
    :param focus_file_path: The expected file path for the focused file.
    :return: The focused workspace.
    """
    # Try the ADS first to avoid undesired loading
    if mantid.mtd.doesExist("%s-ResultTOF" % run_number):
        focused_workspaces = mantid.mtd["%s-ResultTOF" % run_number]
    elif mantid.mtd.doesExist("%s-ResultD" % run_number):
        focused_workspaces = mantid.mtd["%s-ResultD" % run_number]
    else:
        # Check output directory
        print("No loaded focused files found. Searching in output directory...")
        try:
            focused_workspaces = mantid.LoadNexus(Filename=focus_file_path, OutputWorkspace="focused_workspaces").OutputWorkspace
        except ValueError:
            raise ValueError(
                "Could not find focused file for run number:%s\n"
                "Please ensure a focused file has been produced and is located in the output directory." % run_number
            )
    return focused_workspaces


def apply_placzek_correction_per_bank(
    focused_ws,
    run_number,
    sample_details,
    cal_file_name,
    placzek_order,
    sample_temp,
):
    raw_ws = mantid.Load(Filename="POLARIS" + str(run_number))
    sample_geometry_json = sample_details.generate_sample_geometry()
    sample_material_json = sample_details.generate_sample_material()

    self_scattering_correction = mantid.TotScatCalculateSelfScattering(
        InputWorkspace=raw_ws,
        CalFileName=cal_file_name,
        SampleGeometry=sample_geometry_json,
        SampleMaterial=sample_material_json,
        PlaczekOrder=placzek_order,
        SampleTemp=sample_temp,
        ApplyPerDetector=False,
    )
    common.remove_intermediate_workspace(raw_ws)

    ws_group_list = []
    for i in range(self_scattering_correction.getNumberHistograms()):
        ws_name = "correction_" + str(i)
        mantid.ExtractSpectra(InputWorkspace=self_scattering_correction, OutputWorkspace=ws_name, WorkspaceIndexList=[i])
        ws_group_list.append(ws_name)
    self_scattering_correction = mantid.GroupWorkspaces(InputWorkspaces=ws_group_list)
    self_scattering_correction = mantid.RebinToWorkspace(WorkspaceToRebin=self_scattering_correction, WorkspaceToMatch=focused_ws)
    if not compare_ws_compatibility(focused_ws, self_scattering_correction):
        raise RuntimeError("To use create_total_scattering_pdf you need to run focus with " "do_van_normalisation=true first.")
    focused_ws = mantid.Subtract(LHSWorkspace=focused_ws, RHSWorkspace=self_scattering_correction)
    return focused_ws


def apply_placzek_correction_per_detector(
    input_workspace,
    sample_details,
    run_details,
    placzek_order,
    sample_temp,
):
    # this correction should only be applied before focussing in the per_detector case
    raw_ws = mantid.Load(Filename="POLARIS" + str(run_details.run_number) + ".nxs")
    sample_geometry_json = sample_details.generate_sample_geometry()
    sample_material_json = sample_details.generate_sample_material()
    self_scattering_correction = mantid.TotScatCalculateSelfScattering(
        InputWorkspace=raw_ws,
        CalFileName=run_details.grouping_file_path,
        SampleGeometry=sample_geometry_json,
        SampleMaterial=sample_material_json,
        PlaczekOrder=placzek_order,
        SampleTemp=sample_temp,
        ApplyPerDetector=True,
    )
    common.remove_intermediate_workspace(raw_ws)

    input_workspace = mantid.ConvertUnits(
        InputWorkspace=input_workspace, Target="MomentumTransfer", EMode="Elastic", OutputWorkspace=input_workspace
    )
    input_workspace, self_scattering_correction = common._remove_masked_and_monitor_spectra(
        data_workspace=input_workspace, correction_workspace=self_scattering_correction, run_details=run_details
    )
    # Match workspaces
    self_scattering_correction = mantid.RebinToWorkspace(WorkspaceToRebin=self_scattering_correction, WorkspaceToMatch=input_workspace)
    input_workspace = mantid.Subtract(
        LHSWorkspace=input_workspace,
        RHSWorkspace=self_scattering_correction,
        OutputWorkspace=input_workspace,
        AllowDifferentNumberSpectra=True,
    )
    input_workspace = mantid.ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target="TOF", EMode="Elastic")
    return input_workspace


def _load_qlims(q_lims):
    if isinstance(q_lims, str):
        q_min = []
        q_max = []
        try:
            with open(q_lims, "r") as f:
                line_list = [line.rstrip("\n") for line in f]
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
    if ws.getRun().hasProperty("Frequency"):
        frequency = ws.getRun().getTimeAveragedValue("Frequency")
        print("Found chopper frequency of {} in log file.".format(frequency))
        if math.isclose(frequency, 50, abs_tol=1):
            print("Automatically chose Rietveld mode")
            return "Rietveld", polaris_advanced_config.rietveld_focused_cropping_values
        if math.isclose(frequency, 0, abs_tol=1):
            print("Automatically chose PDF mode")
            return "PDF", polaris_advanced_config.pdf_focused_cropping_values
    else:
        raise ValueError("Chopper frequency not in log data. Please specify a chopper mode")


def fast_fourier_filter(ws, rho0, freq_params=None):
    # To be improved - input workspace doesn't have regular bins but output from this filter process does (and typically
    # has a lot more bins since the width is taken from the narrowest bin in the input)
    if freq_params:
        q_data = ws.dataX(0)
        q_max = q_data[-1]
        q_delta = q_data[1] - q_data[0]
        r_min = freq_params[0]
        # If no maximum r is given a high r_max prevents loss of detail on the output.
        if len(freq_params) > 1:
            r_max = freq_params[1]
        else:
            r_max = 200
        ws_name = str(ws)
        mantid.PDFFourierTransform(
            Inputworkspace=ws_name,
            OutputWorkspace=ws_name,
            SofQType="S(Q)-1",
            PDFType="g(r)",
            Filter=True,
            DeltaR=0.01,
            Rmax=r_max,
            Direction="Forward",
            rho0=rho0,
        )
        # apply filter so that g(r)=0 for r < rmin => RDF(r)=0, G(r)~-r
        ws = mantid.mtd[ws_name]
        r_data = ws.dataX(0)
        y_data = ws.dataY(0)
        for i in range(len(r_data)):
            if r_data[i] < r_min and i < len(y_data):  # ws will be points but cope if it's bin edges
                y_data[i] = 0.0
        mantid.PDFFourierTransform(
            Inputworkspace=ws_name,
            OutputWorkspace=ws_name,
            SofQType="S(Q)-1",
            PDFType="g(r)",
            Filter=True,
            Qmax=q_max,
            deltaQ=q_delta,
            Rmax=r_max,
            Direction="Backward",
            rho0=rho0,
        )


def compare_ws_compatibility(ws1, ws2):
    """
    Compares the YUnit and the distribution-type of the first workspaces of two groups for compatibility
    """
    ws1_YUnit = ws1.getItem(0).YUnit()
    ws2_YUnit = ws2.getItem(0).YUnit()
    ws1_Distribution = ws1.getItem(0).isDistribution()
    ws2_Distribution = ws2.getItem(0).isDistribution()
    return ws1_YUnit == ws2_YUnit and ws1_Distribution == ws2_Distribution
