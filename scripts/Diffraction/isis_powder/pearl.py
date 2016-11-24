from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_output, pearl_cycle_factory, pearl_spline


class Pearl(AbstractInst):

    # # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"



    _focus_tof_binning = "1500,-0.0006,19900"

    _create_van_first_tof_binning = "100,-0.0006,19990"
    _create_van_second_tof_binning = "150,-0.0006,19900"

    def __init__(self, user_name, tt_mode="TT88", calibration_dir=None, output_dir=None, **kwargs):

        super(Pearl, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                    output_dir=output_dir, kwargs=kwargs)

        self._tt_mode = tt_mode
        self._focus_mode = None

        # File names
        pearl_mc_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"  # TODO how often does this change
        self._attenuation_full_path = os.path.join(calibration_dir, pearl_mc_absorption_file_name)

    # --- Abstract Implementation ---- #

    def focus(self, run_number, focus_mode, do_attenuation=True, do_van_normalisation=True):
        self._focus_mode = focus_mode
        return self._focus(run_number=run_number,
                           do_attenuation=do_attenuation, do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, vanadium_runs, empty_runs, output_file_name=None, num_of_splines=60,
                                    do_absorb_corrections=True, gen_absorb_correction=False):

        self._create_calibration_vanadium(vanadium_runs=vanadium_runs, empty_runs=empty_runs,
                                          output_file_name=output_file_name, num_of_splines=num_of_splines,
                                          do_absorb_corrections=do_absorb_corrections,
                                          gen_absorb_correction=gen_absorb_correction)

    # Params #
    def _get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def _get_create_van_tof_binning(self):
        return_dict = {"1": self._create_van_first_tof_binning,
                       "2": self._create_van_second_tof_binning}
        return return_dict

    # Methods #

    def _get_run_details(self, run_number):
        # TODO once we migrate this to another format (i.e. not the if/elif/else) implement cached val
        cycle_dict = self._get_label_information(run_number=run_number)

        return pearl_algs.get_run_details(tt_mode=self._tt_mode, run_number_string=run_number,
                                          label=cycle_dict["cycle"], calibration_dir=self._calibration_dir)

    def _get_label_information(self, run_number):
        # TODO remove this when we move to combining CAL/RUN factories
        run_input = ""
        if isinstance(run_number, int) or run_number.isdigit():
            run_input = int(run_number)
        else:
            # Only take first valid number as it is probably of the form 12345_12350
            for character in run_number:
                if character.isdigit():
                    run_input += character
                else:
                    break

        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_input)

        cycle_information = {'cycle': cycle,
                             'instrument_version': instrument_version}
        return cycle_information

    @staticmethod
    def _get_instrument_alg_save_ranges(instrument=''):
        return pearl_algs.get_instrument_ranges(instrument_version=instrument)

    @staticmethod
    def _generate_inst_file_name(run_number):
        return generate_file_name(run_number=run_number)

    # Hook overrides

    def _attenuate_workspace(self, input_workspace):
        return self._run_attenuate_workspace(input_workspace=input_workspace)

    def _create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        input_ws = common.load_current_normalised_ws(run_number_string=calibration_runs, instrument=self)
        cycle_information = self._get_label_information(calibration_runs)

        if cycle_information["instrument_version"] == "new" or cycle_information["instrument_version"] == "new2":
            input_ws = mantid.Rebin(InputWorkspace=input_ws, Params="100,-0.0006,19950")

        d_spacing_cal = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
        d_spacing_cal = mantid.Rebin(InputWorkspace=d_spacing_cal, Params="1.8,0.002,2.1")

        if cycle_information["instrument_version"] == "new2":
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                                 WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)

        elif cycle_information["instrument_version"] == "new":
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                                 WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.8, XMax=2.1)
        else:
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=500,
                                                 WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=1.8, XMax=2.1)
        if self._old_api_uses_full_paths:  # Workaround for old API setting full paths
            grouping_file_path = grouping_file_name
            offset_file_path = offset_file_name
        else:
            offset_file_path = os.path.join(self.calibration_dir, offset_file_name)
            grouping_file_path = os.path.join(self.calibration_dir, grouping_file_name)

        # Ceo Cell refined to 5.4102(3) so 220 is 1.912795
        offset_output_path = mantid.GetDetectorOffsets(InputWorkspace=cross_cor_ws, Step=0.002, DReference=1.912795,
                                                       XMin=-200, XMax=200, GroupingFileName=offset_file_path)
        del offset_output_path  # This isn't used so delete it to keep linters happy
        aligned_ws = mantid.AlignDetectors(InputWorkspace=input_ws, CalibrationFile=offset_file_path)
        cal_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws, GroupingFileName=grouping_file_path)

        common.remove_intermediate_workspace(d_spacing_cal)
        common.remove_intermediate_workspace(cross_cor_ws)
        common.remove_intermediate_workspace(aligned_ws)
        common.remove_intermediate_workspace(cal_grouped_ws)

    def _create_calibration_silicon(self, calibration_runs, cal_file_name, grouping_file_name):
        self._do_silicon_calibration(calibration_runs, cal_file_name, grouping_file_name)

    def _normalise_ws(self, ws_to_correct, run_details=None):
        if not run_details:
            raise RuntimeError("Run details was not passed into PEARL: normalise_ws")
        # TODO remove the loading the monitor separately if possible
        monitor_ws = common.load_monitor(run_numbers=run_details.run_number, instrument=self)
        normalised_ws = pearl_algs.normalise_ws_current(ws_to_correct=ws_to_correct, monitor_ws=monitor_ws,
                                                        spline_coeff=20)
        common.remove_intermediate_workspace(monitor_ws)
        return normalised_ws

    def _get_monitor_spectra(self, run_number):
        return get_monitor_spectra(run_number=run_number, focus_mode=self._focus_mode)

    def _skip_appending_cycle_to_raw_dir(self):
        return self._disable_appending_cycle_to_raw_dir

    def _spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):
        # TODO move spline number into the class
        return pearl_spline.spline_vanadium_for_focusing(focused_vanadium_ws=focused_vanadium_ws,
                                                         spline_number=spline_number,
                                                         instrument_version=instrument_version)

    def _do_tof_rebinning_focus(self, input_workspace):
        input_workspace = mantid.Rebin(InputWorkspace=input_workspace, Params=self._focus_tof_binning)
        return input_workspace

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def _process_focus_output(self, processed_spectra, run_details, attenuate=False):
        return pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                           run_details=run_details, focus_mode=self._focus_mode,
                                                           perform_attenuation=attenuate)

    def _apply_van_calibration_tof_rebinning(self, vanadium_ws, tof_rebin_pass, return_units):
        tof_rebin_param_dict = self._get_create_van_tof_binning()
        tof_rebin_param = tof_rebin_param_dict[str(tof_rebin_pass)]

        out_ws = pearl_algs.apply_tof_rebinning(ws_to_rebin=vanadium_ws, tof_params=tof_rebin_param,
                                                return_units=return_units)

        return out_ws

    def _generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        raise NotImplementedError("Generating absorption corrections needs to be implemented correctly")

        # TODO are these values applicable to all instruments
        shape_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)
        mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                          <radius val="0.005" /> </sphere>')

        absorb_ws = \
            mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                        ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                        NumberOfWavelengthPoints="25", ElementSize="0.05")
        mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                         InputWorkspace=absorb_ws, Append=False)
        common.remove_intermediate_workspace(shape_ws)
        return absorb_ws

    def _calibration_rebin_to_workspace(self, ws_to_rebin, ws_to_match):
        rebinned_ws = mantid.RebinToWorkspace(WorkspaceToRebin=ws_to_rebin, WorkspaceToMatch=ws_to_match)
        common.remove_intermediate_workspace(ws_to_rebin)
        ws_to_rebin = rebinned_ws
        return ws_to_rebin

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        data_ws = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
        data_ws = mantid.ConvertUnits(InputWorkspace=data_ws, Target="TOF")
        data_ws = mantid.Rebin(InputWorkspace=data_ws, Params=self._focus_tof_binning)

        if vanadium_ws:
            data_processed = "van_processed" + str(index)  # Workaround for Mantid overwriting the WS in a loop
            vanadium_ws = mantid.Rebin(InputWorkspace=vanadium_ws, Params=self._focus_tof_binning)
            data_ws = mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_processed)
        else:
            data_processed = "processed-" + str(index)

        mantid.CropWorkspace(InputWorkspace=data_ws, XMin=0.1, OutputWorkspace=data_processed)

        if vanadium_ws:
            mantid.Scale(InputWorkspace=data_processed, Factor=10, OutputWorkspace=data_processed)

        common.remove_intermediate_workspace(data_ws)

        return data_processed

    # Implementation of instrument specific steps

    def _run_attenuate_workspace(self, input_workspace):
        if self._old_atten_file is None:  # For old API support
            attenuation_path = self._attenuation_full_path
        else:
            attenuation_path = self._old_atten_file

        wc_attenuated = mantid.PearlMCAbsorption(attenuation_path)
        wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
        wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=input_workspace,
                                                OutputWorkspace=wc_attenuated)
        pearl_attenuated_ws = mantid.Divide(LHSWorkspace=input_workspace, RHSWorkspace=wc_attenuated)
        common.remove_intermediate_workspace(workspace_name=wc_attenuated)
        return pearl_attenuated_ws

    def _do_silicon_calibration(self, runs_to_process, cal_file_name, grouping_file_name):
        # TODO fix all of this as the script is too limited to be useful
        create_si_ws = common.load_current_normalised_ws(run_number_string=runs_to_process, instrument=self)
        cycle_details = self._get_label_information(runs_to_process)
        instrument_version = cycle_details["instrument_version"]

        if instrument_version == "new" or instrument_version == "new2":
            create_si_ws = mantid.Rebin(InputWorkspace=create_si_ws, Params="100,-0.0006,19950")

        create_si_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=create_si_ws, Target="dSpacing")

        if instrument_version == "new2":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.71,0.002,2.1")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                            WorkspaceIndexMax=1063, XMin=1.71, XMax=2.1)
        elif instrument_version == "new":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.85,0.002,2.05")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                            WorkspaceIndexMax=943, XMin=1.85, XMax=2.05)
        elif instrument_version == "old":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="3,0.002,3.2")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=500, WorkspaceIndexMin=1,
                                                            WorkspaceIndexMax=1440, XMin=3, XMax=3.2)
        else:
            raise NotImplementedError("The instrument version is not supported for creating a silicon calibration")

        common.remove_intermediate_workspace(create_si_d_spacing_ws)
        common.remove_intermediate_workspace(create_si_d_spacing_rebin_ws)

        calibration_output_path = self.calibration_dir + cal_file_name
        create_si_offsets_ws = mantid.GetDetectorOffsets(InputWorkspace=create_si_cross_corr_ws,
                                                         Step=0.002, DReference=1.920127251, XMin=-200, XMax=200,
                                                         GroupingFileName=calibration_output_path)
        create_si_aligned_ws = mantid.AlignDetectors(InputWorkspace=create_si_ws,
                                                     CalibrationFile=calibration_output_path)
        grouping_output_path = self.calibration_dir + grouping_file_name
        create_si_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=create_si_aligned_ws,
                                                           GroupingFileName=grouping_output_path)
        del create_si_offsets_ws, create_si_grouped_ws


def get_monitor_spectra(run_number, focus_mode):
    if run_number < 71009:
        if focus_mode == "trans":
            monitor_spectra = 1081
        elif focus_mode == "all":
            monitor_spectra = 2721
        elif focus_mode == "novan":
            monitor_spectra = 2721
        else:
            raise ValueError("Mode not set or supported")
    else:
        monitor_spectra = 1
    return monitor_spectra


def generate_file_name(run_number):
    digit = len(str(run_number))

    if run_number < 71009:
        number_of_digits = 5
        filename = "PRL"
    else:
        number_of_digits = 8
        filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
