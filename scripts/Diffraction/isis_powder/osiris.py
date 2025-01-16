# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from isis_powder.abstract_inst import AbstractInst
from isis_powder.osiris_routines import osiris_advanced_config, osiris_algs, osiris_param_mapping
from isis_powder.routines import instrument_settings, common_output, common, focus, absorb_corrections, common_enums
import copy
import mantid.simpleapi as mantid
import os


class Osiris(AbstractInst):
    def __init__(self, **kwargs):
        self._drange_sets = {}
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=osiris_param_mapping.attr_mapping, adv_conf_dict=osiris_advanced_config.get_all_adv_variables(), kwargs=kwargs
        )
        self._default_inst_settings = copy.deepcopy(self._inst_settings)

        super(Osiris, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="OSIRIS",
        )
        self._run_details_cached_obj = {}
        self._sample_details = None

    def set_sample_details(self, **kwargs):
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs,
            key=kwarg_name,
            exception_msg=f"The argument containing sample details was not found. Please set the following argument: {kwarg_name}",
        )
        self._sample_details = sample_details_obj

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._setup_drange_sets()

        vanadiums = []
        for drange_idx, drange in self._drange_sets.items():
            run_number_string = drange.get_samples_string()
            vanadium_d = self._create_vanadium(
                run_number_string=run_number_string,
                do_absorb_corrections=False,
                do_spline=False,
            )

            vanadium_d = mantid.CropWorkspace(
                InputWorkspace=vanadium_d,
                XMin=osiris_algs.d_range_alice[drange_idx][0],
                XMax=osiris_algs.d_range_alice[drange_idx][1],
                OutputWorkspace=str(vanadium_d),
            )

            vanadium_d = mantid.ConvertUnits(InputWorkspace=vanadium_d, Target="TOF")

            run_details = self._get_run_details(run_number_string=run_number_string)
            common.save_unsplined_vanadium(
                vanadium_ws=vanadium_d,
                output_path=run_details.unsplined_vanadium_file_path,
                keep_unit=True,
            )
            vanadiums.append(vanadium_d)

        return vanadiums

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)

        if not self._drange_sets:
            self._setup_drange_sets()

        focussed_runs = []

        for drange in self._drange_sets.values():
            run_number_string = drange.get_samples_string()

            self._inst_settings.per_detector_vanadium = None
            processed = self._focus(
                run_number_string=run_number_string,
                do_van_normalisation=self._inst_settings.van_norm,
                do_absorb_corrections=self._inst_settings.absorb_corrections,
                sample_details=self._sample_details,
                empty_can_subtraction_method=self._inst_settings.empty_can_subtraction_method,
                paalman_pings_events_per_point=self._inst_settings.paalman_pings_events_per_point,
            )

            processed = [
                [mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="dSpacing") for ws_group in processed for ws in ws_group]
            ]

            focussed_runs.extend(processed)

        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)

        if self._inst_settings.merge_drange:
            merged_runs = osiris_algs.merge_dspacing_runs(
                focussed_runs,
                self._drange_sets,
                self._inst_settings.run_number,
            )

            d_spacing_group, tof_group = self._output_focused_ws(merged_runs, run_details)

            common.remove_intermediate_workspace(merged_runs)
        else:
            d_spacing_group, tof_group = self._output_focused_ws([ws for ws_group in focussed_runs for ws in ws_group], run_details)

        common.remove_intermediate_workspace([ws for ws_group in focussed_runs for ws in ws_group])

        return d_spacing_group, tof_group

    def _focus(
        self,
        run_number_string,
        do_van_normalisation,
        do_absorb_corrections,
        sample_details=None,
        empty_can_subtraction_method=None,
        paalman_pings_events_per_point=None,
    ):
        """
        Focuses the user specified run(s) - should be called by the concrete instrument.
        :param run_number_string: The run number(s) to be processed.
        :param do_van_normalisation: Whether to divide by the vanadium run or not.
        :param do_absorb_corrections: Whether to apply absorption correction or not.
        :param sample_details: Sample details for the run number(s).
        :param empty_can_subtraction_method: The method for absorption correction. Can be 'Simple' or 'PaalmanPings'.
        :param paalman_pings_events_per_point: The number of events used in Paalman Pings Monte Carlo absorption correction.
        :return: the focussed run(s).
        """
        self._is_vanadium = False
        return focus.focus(
            run_number_string=run_number_string,
            perform_vanadium_norm=do_van_normalisation,
            instrument=self,
            absorb=do_absorb_corrections,
            sample_details=sample_details,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )

    def _setup_drange_sets(self):
        self._drange_sets = osiris_algs.create_drange_sets(self._inst_settings.run_number, self, self._inst_settings.file_extension)

    def _get_run_details(self, run_number_string):
        """
        Returns a RunDetails object with various properties related to the current run set
        :param run_number_string: The run number to look up the properties of
        :return: A RunDetails object containing attributes relevant to that run_number_string
        """
        run_number_string_key = self._generate_run_details_fingerprint(
            run_number_string, self._inst_settings.file_extension, self._is_vanadium
        )

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        drange = self._get_drange_for_run_number(run_number_string)

        self._run_details_cached_obj[run_number_string_key] = osiris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium, drange=drange
        )

        return self._run_details_cached_obj[run_number_string_key]

    def _get_drange_for_run_number(self, run_number_string):
        for drange_name, drange_object in self._drange_sets.items():
            if drange_object.get_samples_string() == run_number_string:
                return drange_name
        return None

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        """
        Generates absorption corrections using monte carlo absorption.
        :param ws_to_correct: workspace that needs to be corrected.
        :param run_details: the run details of the workspace. Unused parameter added for API compatibility.
        :return: A workspace containing the corrections.
        """
        self._check_sample_details()
        if self._inst_settings.simple_events_per_point:
            events_per_point = int(self._inst_settings.simple_events_per_point)
        else:
            events_per_point = 1000

        container_geometry = self._sample_details.generate_container_geometry()
        container_material = self._sample_details.generate_container_material()
        if container_geometry and container_material:
            mantid.SetSample(
                ws_to_correct,
                Geometry=self._sample_details.generate_sample_geometry(),
                Material=self._sample_details.generate_sample_material(),
                ContainerGeometry=container_geometry,
                ContainerMaterial=container_material,
            )

        else:
            mantid.SetSample(
                ws_to_correct,
                Geometry=self._sample_details.generate_sample_geometry(),
                Material=self._sample_details.generate_sample_material(),
            )

        previous_units = ws_to_correct.getAxis(0).getUnit().unitID()
        ws_units = common_enums.WORKSPACE_UNITS

        if previous_units != ws_units.wavelength:
            ws_to_correct = mantid.ConvertUnits(
                InputWorkspace=ws_to_correct,
                OutputWorkspace=ws_to_correct,
                Target=ws_units.wavelength,
            )

        corrections = mantid.MonteCarloAbsorption(InputWorkspace=ws_to_correct, EventsPerPoint=events_per_point)

        ws_to_correct = ws_to_correct / corrections

        if self._inst_settings.multiple_scattering:
            ws_to_correct = self._apply_discus_multiple_scattering(ws_to_correct)

        if previous_units != ws_units.wavelength:
            ws_to_correct = mantid.ConvertUnits(
                InputWorkspace=ws_to_correct,
                Target=previous_units,
                OutputWorkspace=ws_to_correct,
            )

        common.remove_intermediate_workspace(corrections)

        return ws_to_correct

    def _apply_paalmanpings_absorb_and_subtract_empty(self, workspace, summed_empty, sample_details, paalman_pings_events_per_point=None):
        """
        Applies the Paalman Pings Monte Carlo absorption to the workspace.

        :param workspace: The input workspace containing the data to be corrected.
        :param summed_empty:The workspace containing empty container run data.
        :param sample_details: The details of the sample being corrected.
        :param paalman_pings_events_per_point: The number of events per point for the Paalman-Pings correction.

        :return: The corrected workspace.
        """
        mantid.SetInstrumentParameter(Workspace=workspace, ParameterName="deltaE-mode", Value="Elastic")
        paalman_corrected = absorb_corrections.apply_paalmanpings_absorb_and_subtract_empty(
            workspace=workspace,
            summed_empty=summed_empty,
            sample_details=sample_details,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )

        if self._inst_settings.multiple_scattering:
            return self._apply_discus_multiple_scattering(paalman_corrected)

        return paalman_corrected

    def _apply_discus_multiple_scattering(self, ws_to_correct):
        if self._inst_settings.neutron_paths_single:
            neutron_paths_single = int(self._inst_settings.neutron_paths_single)
        else:
            neutron_paths_single = 100

        if self._inst_settings.neutron_paths_multiple:
            neutron_paths_multiple = int(self._inst_settings.neutron_paths_multiple)
        else:
            neutron_paths_multiple = 100

        X = [1.0]
        Y = [1.0]
        Sofq_isotropic = mantid.CreateWorkspace(DataX=X, DataY=Y, UnitX="MomentumTransfer")

        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="Momentum")

        mantid.DiscusMultipleScatteringCorrection(
            InputWorkspace=ws_to_correct,
            StructureFactorWorkspace=Sofq_isotropic,
            NeutronPathsSingle=neutron_paths_single,
            NeutronPathsMultiple=neutron_paths_multiple,
            OutputWorkspace="MSResults",
        )

        ratio = mantid.mtd["MSResults_Ratio_Single_To_All"]

        return ws_to_correct * ratio

    def apply_drange_cropping(self, run_number_string, focused_ws):
        """
        Applies dspacing range cropping to a focused workspace.
        :param run_number_string: The run number to look up for the drange
        :param focused_ws: The workspace to be cropped
        :return: The cropped workspace in its drange
        """

        drange = self._get_drange_for_run_number(run_number_string)

        return mantid.CropWorkspace(
            InputWorkspace=focused_ws,
            XMin=osiris_algs.d_range_alice[drange][0],
            XMax=osiris_algs.d_range_alice[drange][1],
            OutputWorkspace=str(focused_ws),
        )

    def get_vanadium_path(self, run_details):
        """
        Returns the vanadium path from the run details
        :param run_details: The run details of the run number
        :return: the vanadium path
        """

        return run_details.unsplined_vanadium_file_path

    def _output_focused_ws(self, processed_spectra, run_details):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing group of the processed output workspaces
        """
        d_spacing_group, tof_group, q_squared_group = common_output.split_into_tof_d_spacing_groups(
            run_details=run_details, processed_spectra=processed_spectra, include_q_squared=True
        )
        for unit, group in {"_d_spacing": d_spacing_group, "_tof": tof_group, "_q_squared": q_squared_group}.items():
            save_data(workspace_group=group, output_paths=self._generate_out_file_paths(run_details=run_details, unit=unit))

        return d_spacing_group, tof_group

    def _generate_out_file_paths(self, run_details, unit=""):
        """
        Generates the various output paths and file names to be used during saving or as workspace names
        :param run_details: The run details associated with this run
        :return: A dictionary containing the various output paths and generated output name
        """
        output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        output_directory = os.path.abspath(os.path.expanduser(output_directory))
        dat_files_directory = output_directory
        if self._inst_settings.dat_files_directory:
            dat_files_directory = os.path.join(output_directory, self._inst_settings.dat_files_directory)

        out_file_names = {"output_folder": output_directory}
        format_options = {
            "inst": self._inst_prefix,
            "instlow": self._inst_prefix.lower(),
            "instshort": self._inst_prefix_short,
            "runno": run_details.output_run_string,
            "suffix": run_details.output_suffix if run_details.output_suffix else "",
            "unit": unit,
        }
        format_options = self._add_formatting_options(format_options)

        output_formats = {
            "nxs_filename": output_directory,
            "gss_filename": output_directory,
            "xye_filename": dat_files_directory,
        }
        for key, output_dir in output_formats.items():
            filepath = os.path.join(output_dir, getattr(self._inst_settings, key).format(**format_options))
            out_file_names[key] = filepath

        out_file_names["output_name"] = os.path.splitext(os.path.basename(out_file_names["nxs_filename"]))[0]
        return out_file_names

    def should_subtract_empty_inst(self):
        """
        :return: Whether the empty run should be subtracted from a run being focused
        """
        return False


def save_data(workspace_group, output_paths):
    """
    Saves out data into nxs, GSAS and .dat formats. Requires the grouped workspace
    and the dictionary of output paths generated by abstract_inst.
    :param workspace_group: The workspace group
    :param tof_group: The focused workspace group in TOF
    :param output_paths: A dictionary containing the full paths to save to
    :return: None
    """

    def ensure_dir_exists(filename):
        dirname = os.path.dirname(filename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        return filename

    mantid.SaveGSS(InputWorkspace=workspace_group, Filename=ensure_dir_exists(output_paths["gss_filename"]), SplitFiles=False, Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=workspace_group, Filename=ensure_dir_exists(output_paths["nxs_filename"]), Append=False)

    for bank_index, ws in enumerate(workspace_group):
        bank_index += 1  # Ensure we start at 1 when saving out
        mantid.SaveFocusedXYE(
            InputWorkspace=ws,
            Filename=ensure_dir_exists(output_paths["xye_filename"]).format(bankno=bank_index),
            SplitFiles=False,
            IncludeHeader=False,
        )
