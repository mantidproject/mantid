# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import platform
import subprocess
import shutil
import time
import datetime
import json
import matplotlib.pyplot as plt
import numpy as np

from typing import List

from mantid.geometry import CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from mantid.simpleapi import (
    CreateWorkspace,
    LoadGSS,
    DeleteWorkspace,
    CreateEmptyTableWorkspace,
    Load,
    logger,
    LoadCIF,
    CreateSampleWorkspace,
)
from mantid.api import AnalysisDataService as ADS
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2 import parse_inputs
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import (
    SampleLogsGroupWorkspace,
    _generate_workspace_name,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.workspace_record import FittingWorkspaceRecordContainer


class GSAS2Model(object):
    def __init__(self):
        self.user_save_directory = None
        self.temporary_save_directory = None
        self.path_to_gsas2 = None
        self.gsas2_save_dirs = None
        self.project_name = None
        self.data_files = None
        self.instrument_files = None
        self.phase_filepaths = None
        self.x_min = None
        self.x_max = None
        self.data_x_min = None
        self.data_x_max = None
        self.number_of_regions = None
        self.number_histograms = None
        self.refinement_method = None
        self.dSpacing_min = 1.0
        self.timeout = 10
        self.refine_microstrain = None
        self.refine_sigma_one = None
        self.refine_gamma = None
        self.refine_background = None
        self.refine_unit_cell = None
        self.refine_histogram_scale_factor = None
        self.limits = None
        self.override_cell_length_string = None
        self.mantid_pawley_reflections = None
        self.crystal_structures = None
        self.chosen_cell_lengths = None
        self.out_call_gsas2 = None
        self.err_call_gsas2 = None
        self._data_workspaces = FittingWorkspaceRecordContainer()
        self._suffix = "_GSASII"
        self._sample_logs_workspace_group = SampleLogsGroupWorkspace(self._suffix)
        self.phase_names_list = None

    def clear_input_components(self):
        self.user_save_directory = None
        self.temporary_save_directory = None
        self.path_to_gsas2 = None
        self.gsas2_save_dirs = None
        self.project_name = None
        self.data_files = None
        self.instrument_files = None
        self.phase_filepaths = None
        self.x_min = None
        self.x_max = None
        self.data_x_min = None
        self.data_x_max = None
        self.number_of_regions = None
        self.number_histograms = None
        self.refinement_method = None
        self.dSpacing_min = 1.0
        self.timeout = 10
        self.refine_microstrain = None
        self.refine_sigma_one = None
        self.refine_gamma = None
        self.refine_background = None
        self.refine_unit_cell = None
        self.refine_histogram_scale_factor = None
        self.limits = None
        self.override_cell_length_string = None
        self.mantid_pawley_reflections = None
        self.crystal_structures = None
        self.chosen_cell_lengths = None
        self.out_call_gsas2 = None
        self.err_call_gsas2 = None
        self.phase_names_list = None

    def run_model(self, load_parameters, refinement_parameters, project_name, rb_num, user_x_limits):
        self.clear_input_components()
        if not self.initial_validation(project_name, load_parameters):
            return None
        self.set_components_from_inputs(load_parameters, refinement_parameters, project_name, rb_num)
        self.read_phase_files()
        self.generate_reflections_from_space_group()
        self.validate_x_limits(user_x_limits)
        if not self.further_validation():
            return None

        runtime = self.call_gsas2()
        if not runtime:
            return None
        gsas_result = self.report_on_outputs(runtime)
        if not gsas_result:
            return None
        self.load_basic_outputs(gsas_result)

        if self.number_of_regions > self.number_histograms:
            return self.number_of_regions
        return self.number_histograms

    # ===============
    # Prepare Inputs
    # ===============

    def set_components_from_inputs(self, load_params, refinement_params, project, rb_number):
        self.timeout = int(get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "timeout"))
        self.dSpacing_min = float(
            get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "dSpacing_min")
        )

        self.path_to_gsas2 = output_settings.get_path_to_gsas2()
        self.project_name = project
        self.organize_save_directories(rb_number)

        self.instrument_files = load_params[0]
        self.phase_filepaths = load_params[1]
        self.data_files = load_params[2]

        self.refinement_method = refinement_params[0]
        self.override_cell_length_string = refinement_params[1]
        self.refine_microstrain = refinement_params[2]
        self.refine_sigma_one = refinement_params[3]
        self.refine_gamma = refinement_params[4]

        # Currently hardcoded
        self.refine_background = True
        self.refine_unit_cell = True
        self.refine_histogram_scale_factor = True  # True by default

    # ===========
    # Validation
    # ===========

    def initial_validation(self, project, load_params):
        if not project:
            logger.error("* There must be a valid Project Name for GSASII refinement")
            return None
        if not load_params[0] or not load_params[1] or not load_params[2]:
            logger.error("* All filepaths for Instrument, Phase and Focused Data must be valid")
            return None
        return True

    def further_validation(self):
        if not self.limits:
            return None  # any errors logged in validate_x_limits()
        if self.refinement_method == "Pawley" and not self.mantid_pawley_reflections:
            logger.error("No Pawley Reflections were generated for the phases provided. Not calling GSAS-II.")
            return None
        if not self.path_to_gsas2:
            logger.error("The Path to GSAS2 setting is empty. Please provide a valid path in Engineering Diffraction Settings")
            return None
        return True

    # ===============
    # Calling GSASII
    # ===============

    def call_gsas2(self):
        command_string_list = self.format_gsas2_call()
        start = time.time()
        gsas2_called = self.call_subprocess(command_string_list)
        if not gsas2_called:
            return None
        self.out_call_gsas2, self.err_call_gsas2 = gsas2_called
        gsas_runtime = time.time() - start
        logger.notice(self.format_shell_output(title="Commandline output from GSAS-II", shell_output_string=self.out_call_gsas2))
        return gsas_runtime

    def format_gsas2_call(self):
        gsas2_inputs = parse_inputs.Gsas2Inputs(
            path_to_gsas2=self.path_to_gsas2,
            temporary_save_directory=self.temporary_save_directory,
            project_name=self.project_name,
            refinement_method=self.refinement_method,
            refine_background=self.refine_background,
            refine_microstrain=self.refine_microstrain,
            refine_sigma_one=self.refine_sigma_one,
            refine_gamma=self.refine_gamma,
            refine_histogram_scale_factor=self.refine_histogram_scale_factor,
            data_files=self.data_files,
            phase_files=self.phase_filepaths,
            instrument_files=self.instrument_files,
            limits=self.limits,
            mantid_pawley_reflections=self.mantid_pawley_reflections,
            override_cell_lengths=self.get_override_lattice_parameters(),
            refine_unit_cell=self.refine_unit_cell,
            d_spacing_min=self.dSpacing_min,
            number_of_regions=self.number_of_regions,
        )
        gsas2_python_path = os.path.join(self.path_to_gsas2, "bin", "python")
        if platform.system() == "Windows":
            gsas2_python_path = os.path.join(self.path_to_gsas2, "python.exe")
        call = [
            gsas2_python_path,
            os.path.abspath(os.path.join(os.path.dirname(__file__), "call_G2sc.py")),
            parse_inputs.Gsas2Inputs_to_json(gsas2_inputs),
        ]
        return call

    def call_subprocess(self, command_string_list):
        try:
            env = os.environ.copy()
            env["PYTHONHOME"] = self.path_to_gsas2
            # Search for binaries in GSASII directory before Mantid Conda environment
            # Need to activate GSASII conda environment by calling "activate" script for proper solution
            if platform.system() == "Windows":
                extra_paths = [
                    self.path_to_gsas2,
                    os.path.join(self.path_to_gsas2, "bin"),
                    os.path.join(self.path_to_gsas2, "Library", "bin"),
                    os.path.join(self.path_to_gsas2, "Library", "usr", "bin"),
                    os.path.join(self.path_to_gsas2, "Library", "mingw-w64", "bin"),
                    os.path.join(self.path_to_gsas2, "Scripts"),
                ]
            else:
                extra_paths = [os.path.join(self.path_to_gsas2, "bin")]
            env["PATH"] = ";".join(extra_paths + [env["PATH"]])
            # The PyCharm debugger attempts to debug into any python subprocesses spawned by Workbench
            # On Windows (see pydev_monkey.py) this results in the command line arguments being manipulated and
            # the GSASII json parameter string gets corrupted
            # Avoid this by passing the GSASII python exe in via the executable parameter instead of argv[0] and also
            # set argv[0] to something not containing the string 'python'
            # Note - the PyCharm behaviour can also be disabled by unchecking this property in File, Settings:
            # "Attach to subprocess automatically while debugging"
            empty_argv0 = "_"
            shell_process = subprocess.Popen(
                [empty_argv0] + command_string_list[1:],
                executable=command_string_list[0],
                shell=False,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                close_fds=True,
                universal_newlines=True,
                env=env,
            )

            shell_output = shell_process.communicate(timeout=self.timeout)

            if shell_process.returncode != 0:
                logger.error(f"GSAS-II call failed with error: {shell_output[-1]}")
                return None

            return shell_output
        except subprocess.TimeoutExpired:
            shell_process.terminate()
            logger.error(
                f"GSAS-II call did not complete after {self.timeout} seconds, so it was"
                f" aborted. Check the inputs, such as Refinement Method are correct. The"
                f" timeout interval can be increased in the Engineering Diffraction Settings."
            )
            return None
        except Exception as exc:
            logger.error(
                f"GSAS-II call failed with error: {str(exc)}. "
                "Please check the Path to GSASII in the Engineering Diffraction Settings is valid"
            )
            return None

    def format_shell_output(self, title, shell_output_string):
        double_line = "-" * (len(title) + 2) + "\n" + "-" * (len(title) + 2)
        return "\n" * 3 + double_line + "\n " + title + " \n" + double_line + "\n" + shell_output_string + "\n" + double_line + "\n" * 3

    # ===========
    # Read Files
    # ===========

    def find_in_file(self, file_path, marker_string, start_of_value, end_of_value, strip_separator=None):
        value_string = None
        if os.path.exists(file_path):
            with open(file_path, "rt", encoding="utf-8") as file:
                full_file_string = file.read().replace("\n", "")
                where_marker = full_file_string.rfind(marker_string)
                if where_marker != -1:
                    where_value_start = full_file_string.find(start_of_value, where_marker)
                    if where_value_start != -1:
                        where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                        value_string = full_file_string[where_value_start:where_value_end]
                        if strip_separator:
                            value_string = value_string.strip(strip_separator + " ")
                        else:
                            value_string = value_string.strip(" ")
        return value_string

    def get_crystal_params_from_instrument(self, instrument):
        crystal_params = []
        if not self.number_of_regions:
            crystal_params = [self.find_in_file(instrument, "ICONS", "S", "INS", strip_separator="ICONS\t")]

        else:
            for region_index in range(1, self.number_of_regions + 1):
                loop_crystal_params = self.find_in_file(instrument, f"{region_index} ICONS", "S", "INS", strip_separator="ICONS\t")
                crystal_params.append(loop_crystal_params)
        tof_min = []
        file_mismatch = f"Different number of banks in {instrument} and {self.data_files}"
        for loop_crystal_param_string in crystal_params:
            if not loop_crystal_param_string:
                logger.error(file_mismatch)
                return None
            list_crystal_params = loop_crystal_param_string.split(" ")
            if len(list_crystal_params) == 1:
                list_crystal_params = list_crystal_params[0].split("\t")
            list_crystal_params = list(filter(None, list_crystal_params))
            if len(list_crystal_params):
                dif_c = float(list_crystal_params[0] or 0.0)
                dif_a = float(list_crystal_params[1] or 0.0)
                t_zero = float(list_crystal_params[2] or 0.0)
                tof_min.append((dif_c * self.dSpacing_min) + (dif_a * (self.dSpacing_min**2)) + t_zero)
        if not tof_min:
            logger.error(file_mismatch)
            return None
        return tof_min

    # ===================
    # Pawley Reflections
    # ===================

    def create_pawley_reflections(self, crystal_structure):
        generator = ReflectionGenerator(crystal_structure)
        hkls = generator.getUniqueHKLsUsingFilter(self.dSpacing_min, 4.2, ReflectionConditionFilter.StructureFactor)
        dValues = generator.getDValues(hkls)
        pg = crystal_structure.getSpaceGroup().getPointGroup()
        # Make list of tuples and sort by d-values, descending, include point group for multiplicity.
        generated_reflections = sorted(
            [[list(hkl), d, len(pg.getEquivalents(hkl))] for hkl, d in zip(hkls, dValues)],
            key=lambda x: x[1] - x[0][0] * 1e-6,
            reverse=True,
        )
        return generated_reflections

    def read_phase_files(self):
        self.crystal_structures = []
        for phase_filepath in self.phase_filepaths:
            self.crystal_structures.append(self._read_cif_file(phase_filepath))
        if self.override_cell_length_string:
            # override lattice parameters of first file
            self.crystal_structures[0] = self.set_lattice_params_from_user_input(self.crystal_structures[0])

    def get_override_lattice_parameters(self):
        if self.override_cell_length_string:
            return [self._get_lattice_parameters_from_crystal_structure(xtal) for xtal in self.crystal_structures]
        else:
            None

    def _get_lattice_parameters_from_crystal_structure(self, crystal_structure):
        return [getattr(crystal_structure.getUnitCell(), method)() for method in ("a", "b", "c", "alpha", "beta", "gamma")]

    def _read_cif_file(self, phase_filepath):
        ws = CreateSampleWorkspace(StoreInADS=False)
        LoadCIF(ws, phase_filepath, StoreInADS=False)  # error if not StoreInADS=False even though no output
        return ws.sample().getCrystalStructure()

    def set_lattice_params_from_user_input(self, crystal_structure):
        # user can delimit with , or whitespace
        user_alatt = [param for param in self.override_cell_length_string.replace(",", " ").split()]
        if len(user_alatt) == 1:
            user_alatt = 3 * user_alatt  # assume cubic
        crystal_structure = CrystalStructure(
            " ".join(user_alatt), crystal_structure.getSpaceGroup().getHMSymbol(), ";".join(crystal_structure.getScatterers())
        )
        return crystal_structure

    def generate_reflections_from_space_group(self):
        self.mantid_pawley_reflections = []
        for crystal_structure in self.crystal_structures:
            self.mantid_pawley_reflections.append(self.create_pawley_reflections(crystal_structure))
        return

    # =========
    # X Limits
    # =========

    def understand_data_structure(self):
        self.data_x_min = []
        self.data_x_max = []
        number_of_regions = 0
        for input_file in self.data_files:
            loop_focused_workspace = LoadGSS(Filename=input_file, OutputWorkspace="GSASII_input_data", EnableLogging=False)
            for workspace_index in range(loop_focused_workspace.getNumberHistograms()):
                self.data_x_min.append(loop_focused_workspace.readX(workspace_index)[0])
                self.data_x_max.append(loop_focused_workspace.readX(workspace_index)[-1])
                number_of_regions += 1
            DeleteWorkspace(loop_focused_workspace)
        self.number_of_regions = number_of_regions

    def validate_x_limits(self, users_limits):
        self.understand_data_structure()
        if users_limits:
            if len(users_limits[0]) != self.number_of_regions:
                users_limits[0] *= self.number_of_regions
                users_limits[1] *= self.number_of_regions
        self.number_histograms = len(self.data_files)
        if len(self.instrument_files) != 1 and len(self.instrument_files) != self.number_histograms:
            logger.error(
                f"The number of instrument files ({len(self.instrument_files)}) must be 1 "
                f"or equal to the number of input histograms {self.number_histograms}"
            )
            return None
        if users_limits:
            self.x_min = [float(k) for k in users_limits[0]]
            self.x_max = [float(k) for k in users_limits[1]]
            if len(self.x_min) != self.number_histograms:
                if len(self.x_min) == 1:
                    self.x_min = self.x_min * self.number_histograms
                    self.x_max = self.x_max * self.number_histograms
        else:
            self.x_min = self.data_x_min
            self.x_max = self.data_x_max

        self.limits = [self.x_min, self.x_max]
        return True

    # ===============
    # Handle Outputs
    # ===============

    def read_gsas_lst_and_print_wR(self, result_filepath, histogram_data_files, test=False):
        with open(result_filepath, "rt", encoding="utf-8") as file:
            result_string = file.read().replace("\n", "")
            for loop_histogram in histogram_data_files:
                where_loop_histogram = result_string.rfind(loop_histogram)
                if where_loop_histogram != -1:
                    where_loop_histogram_wR = result_string.find("Final refinement wR =", where_loop_histogram)
                    if where_loop_histogram_wR != -1:
                        where_loop_histogram_wR_end = result_string.find("%", where_loop_histogram_wR)
                        logger.notice(loop_histogram)
                        logged_lst_result = result_string[where_loop_histogram_wR : where_loop_histogram_wR_end + 1]
                        logger.notice(logged_lst_result)
                        if test:
                            return logged_lst_result

    def find_phase_names_in_lst(self, file_path):
        phase_names = []
        marker_string = "Phase name"
        start_of_value = ":"
        end_of_value = "="
        strip_separator = ":"
        with open(file_path, "rt", encoding="utf-8") as file:
            full_file_string = file.read().replace("\n", "")
            where_marker = full_file_string.find(marker_string)
            while where_marker != -1:
                where_value_start = full_file_string.find(start_of_value, where_marker)
                if where_value_start != -1:
                    where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                    phase_names.append(full_file_string[where_value_start:where_value_end].strip(strip_separator + " "))
                    where_marker = full_file_string.find(marker_string, where_value_end)
                else:
                    where_marker = -1
        return phase_names

    def report_on_outputs(self, runtime, test=False):
        gsas_project_filepath = self.check_for_output_file(".gpx", "project")
        gsas_result_filepath = self.check_for_output_file(".lst", "result")
        if not gsas_project_filepath or not gsas_result_filepath:
            return None
        logged_success = f"\nGSAS-II call complete in {runtime} seconds.\n"
        logger.notice(logged_success)
        if test:
            return gsas_result_filepath, logged_success
        return gsas_result_filepath

    def check_for_output_file(self, file_extension, file_descriptor, test=False):
        gsas_output_filename = self.project_name + file_extension
        if gsas_output_filename not in os.listdir(self.temporary_save_directory):
            logged_failure = (
                f"GSAS-II call must have failed, as the output {file_descriptor} file was not found."
                + self.format_shell_output(title="Errors from GSAS-II", shell_output_string=self.err_call_gsas2)
            )
            logger.error(logged_failure)
            if test:
                return logged_failure
            return None
        return os.path.join(self.temporary_save_directory, gsas_output_filename)

    def organize_save_directories(self, rb_num_string):
        save_dir = os.path.join(output_settings.get_output_path())
        self.gsas2_save_dirs = [os.path.join(save_dir, "GSAS2", "")]
        save_directory = self.gsas2_save_dirs[0]
        if rb_num_string:
            self.gsas2_save_dirs.append(os.path.join(save_dir, "User", rb_num_string, "GSAS2", self.project_name, ""))
            # TODO: Once texture is supported, pass calibration observer like currently done for focus tab
            # if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            #     calib_dirs.pop(0)  # only save to RB directory to limit number files saved
        self.user_save_directory = os.path.join(save_directory, self.project_name)
        self.temporary_save_directory = os.path.join(
            save_directory, datetime.datetime.now().strftime("tmp_EngDiff_GSASII_%Y-%m-%d_%H-%M-%S")
        )
        os.makedirs(self.temporary_save_directory)

    def move_output_files_to_user_save_location(self):
        for new_directory in self.gsas2_save_dirs:
            os.makedirs(new_directory, exist_ok=True)

        save_success_message = f"\n\nOutput GSAS-II files saved in {self.user_save_directory}"

        exist_extra_save_dirs = False
        if len(self.gsas2_save_dirs) > 1:
            exist_extra_save_dirs = True
            self.gsas2_save_dirs.pop(0)

        if os.path.exists(self.user_save_directory):
            shutil.rmtree(self.user_save_directory)
        os.makedirs(self.user_save_directory, exist_ok=True)
        for output_file_index, output_file in enumerate(os.listdir(self.temporary_save_directory)):
            os.rename(os.path.join(self.temporary_save_directory, output_file), os.path.join(self.user_save_directory, output_file))
            if exist_extra_save_dirs:
                for extra_save_dir in self.gsas2_save_dirs:
                    shutil.copy(os.path.join(self.user_save_directory, output_file), os.path.join(extra_save_dir, output_file))
                    if output_file_index == 0:
                        save_success_message += f" and in {extra_save_dir}"
        os.rmdir(self.temporary_save_directory)
        return save_success_message

    # =============
    # Load Outputs
    # =============

    def load_basic_outputs(self, gsas_result_filepath):
        logger.notice(f"GSAS-II .lst result file found. Opening {self.project_name}.lst")
        self.read_gsas_lst_and_print_wR(gsas_result_filepath, self.data_files)
        save_message = self.move_output_files_to_user_save_location()
        logger.notice(save_message)

        self.phase_names_list = self.find_phase_names_in_lst(os.path.join(self.user_save_directory, self.project_name + ".lst"))
        self.create_lattice_parameter_table()
        self.create_instrument_parameter_table()
        self.create_reflections_table()

    def load_result_for_plot(self, index_histograms):
        workspace = self.load_gsas_histogram(index_histograms)
        reflections = None
        if self.refinement_method == "Pawley":
            reflections = self.load_gsas_reflections_per_histogram_for_plot(index_histograms)
        return workspace, reflections

    def chop_to_limits(self, input_array, x, min_x, max_x):
        input_array[x <= min_x] = np.nan
        input_array[x >= max_x] = np.nan
        return input_array

    def load_gsas_histogram(self, histogram_index):
        result_csv = os.path.join(self.user_save_directory, self.project_name + f"_{histogram_index}.csv")
        my_data = np.transpose(np.genfromtxt(result_csv, delimiter=",", skip_header=39))
        # x  y_obs	weight	y_calc	y_bkg	Q
        x_values = my_data[0]
        y_obs = self.chop_to_limits(np.array(my_data[1]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_calc = self.chop_to_limits(np.array(my_data[3]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_diff = y_obs - y_calc
        y_diff -= np.max(np.ma.masked_invalid(y_diff))
        y_bkg = self.chop_to_limits(np.array(my_data[4]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_data = np.concatenate((y_obs, y_calc, y_diff, y_bkg))

        gsas_histogram = CreateWorkspace(
            OutputWorkspace=f"gsas_histogram_{histogram_index}", DataX=np.tile(my_data[0], 4), DataY=y_data, NSpec=4, EnableLogging=False
        )
        return gsas_histogram

    def load_gsas_reflections_per_histogram_for_plot(self, histogram_index):
        loaded_reflections = []
        for phase_name in self.phase_names_list:
            result_reflections_txt = os.path.join(
                self.user_save_directory, self.project_name + f"_reflections_{histogram_index}_{phase_name}.txt"
            )
            if os.path.exists(result_reflections_txt):
                # omit first 2 lines in file (which are histogram and phase name)
                loaded_reflections.append(np.genfromtxt(result_reflections_txt, skip_header=2))
            else:
                logger.warning(f"No reflections found for phase {phase_name} within x-limits of the fit.")
        return loaded_reflections

    def load_gsas_reflections_all_histograms_for_table(self):
        result_reflections_rows = []
        output_reflections_files = self.get_txt_files_that_include("_reflections_")
        for file in output_reflections_files:
            # note delimiter="," specified so as to accept whitespace in phase name (2nd header row)
            loop_file_text = np.genfromtxt(file, delimiter=",", dtype="str")
            loop_histogram_name = loop_file_text[0]
            loop_phase_name = loop_file_text[1]
            loop_reflections_text = ",    ".join(str(num) for num in loop_file_text[2:])
            result_reflections_rows.append([loop_histogram_name, loop_phase_name, loop_reflections_text])
        return result_reflections_rows

    def create_reflections_table(self, test=False):
        table_rows = self.load_gsas_reflections_all_histograms_for_table()
        if not table_rows or self.refinement_method == "Rietveld":
            # cannot generate a valid reflections workspace. If one with the same project_name exists, remove it.
            # This is to cover the case where a user runs a Pawley then Rietveld Refinement for the same project_name.
            if ADS.doesExist(f"{self.project_name}_GSASII_reflections"):
                DeleteWorkspace(f"{self.project_name}_GSASII_reflections", EnableLogging=False)
            return None
        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_reflections", EnableLogging=False)
        table.addReadOnlyColumn("str", "Histogram name")
        table.addReadOnlyColumn("str", "Phase name")
        table.addReadOnlyColumn("str", "Reflections")
        for row in table_rows:
            table.addRow(row)
        if test:
            return table

    def get_txt_files_that_include(self, sub_string):
        output_files = []
        for _, _, filenames in os.walk(self.user_save_directory):
            for loop_filename in filenames:
                if sub_string in loop_filename and loop_filename[-4:] == ".txt":
                    output_files.append(os.path.join(self.user_save_directory, loop_filename))
        output_files.sort()
        return output_files

    def create_instrument_parameter_table(self, test=False):
        INST_TABLE_PARAMS = ["Histogram name", "Sigma-1", "Gamma (Y)"]
        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_instrument_parameters", EnableLogging=False)
        table.addReadOnlyColumn("str", "Histogram name")
        table.addReadOnlyColumn("double", "Sigma-1{}".format(" (Refined)" if self.refine_sigma_one else ""))
        table.addReadOnlyColumn("double", "Gamma (Y){}".format(" (Refined)" if self.refine_gamma else ""))
        table.addReadOnlyColumn("double", "Fit X Min")
        table.addReadOnlyColumn("double", "Fit X Max")

        output_inst_param_files = self.get_txt_files_that_include("_inst_parameters_")

        for inst_parameters_txt in output_inst_param_files:
            with open(inst_parameters_txt, "rt", encoding="utf-8") as file:
                full_file_string = file.read().replace("\n", "")
            inst_parameter_dict = json.loads(full_file_string)
            loop_inst_parameters = [inst_parameter_dict["Histogram name"]]
            for param in INST_TABLE_PARAMS[1:]:
                loop_inst_parameters.append(float(inst_parameter_dict[param][1]))

            bank_number_and_extension = str(list(inst_parameters_txt.split("_"))[-1])
            bank_number_from_gsas2_histogram_name = int(bank_number_and_extension.replace(".txt", ""))
            loop_inst_parameters.append(float(self.x_min[bank_number_from_gsas2_histogram_name - 1]))
            loop_inst_parameters.append(float(self.x_max[bank_number_from_gsas2_histogram_name - 1]))
            table.addRow(loop_inst_parameters)
        if test:
            return table

    def create_lattice_parameter_table(self, test=False):
        LATTICE_TABLE_PARAMS = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]

        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_lattice_parameters", EnableLogging=False)
        table.addReadOnlyColumn("str", "Phase name")

        for param in LATTICE_TABLE_PARAMS:
            table.addReadOnlyColumn("double", param.split("_")[-1])
        table.addReadOnlyColumn("double", "Microstrain{}".format(" (Refined)" if self.refine_microstrain else ""))
        for phase_name in self.phase_names_list:
            parameters_txt = os.path.join(self.user_save_directory, self.project_name + f"_cell_parameters_{phase_name}.txt")
            with open(parameters_txt, "rt", encoding="utf-8") as file:
                full_file_string = file.read().replace("\n", "")
            parameter_dict = json.loads(full_file_string)
            loop_parameters = [phase_name]
            for param in LATTICE_TABLE_PARAMS:
                loop_parameters.append(float(parameter_dict[param]))
            loop_parameters.append(float(parameter_dict["Microstrain"]))
            table.addRow(loop_parameters)
        if test:
            return table

    # ===========
    # Sample Logs
    # ===========

    def load_focused_nxs_for_logs(self, filenames):
        if len(filenames) == 1 and "all_banks" in filenames[0]:
            filenames = [filenames[0].replace("all_banks", "bank_1"), filenames[0].replace("all_banks", "bank_2")]
        for filename in filenames:
            filename = filename.replace(".gss", ".nxs")
            ws_name = _generate_workspace_name(filename, self._suffix)
            if ws_name not in self._data_workspaces.get_loaded_workpace_names():
                try:
                    ws = Load(filename, OutputWorkspace=ws_name)
                    if ws.getNumberHistograms() == 1:
                        self._data_workspaces.add(ws_name, loaded_ws=ws)
                    else:
                        logger.warning(f"Invalid number of spectra in workspace {ws_name}. Skipping loading of file.")
                except RuntimeError as e:
                    logger.error(f"Failed to load file: {filename}. Error: {e}. \n Continuing loading of other files.")
            else:
                logger.warning(f"File {ws_name} has already been loaded")
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def update_sample_log_workspace_group(self):
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def get_all_workspace_names(self):
        return self._data_workspaces.get_loaded_workpace_names() + self._data_workspaces.get_bgsub_workpace_names()

    def get_log_workspaces_name(self):
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        return [ws.name() for ws in current_log_workspaces] if current_log_workspaces else ""

    def set_log_workspaces_none(self):
        # to be used in the event of Ads clear, as trying to reference the deleted grp ws results in an error
        self._sample_logs_workspace_group.clear_log_workspaces()

    # handle ADS remove. name workspace has already been deleted
    def remove_workspace(self, name):
        ws_loaded = self._data_workspaces.get(name, None)
        if ws_loaded:
            bgsub_ws_name = self._data_workspaces[name].bgsub_ws_name
            removed = self._data_workspaces.pop(name).loaded_ws
            # deleting bg sub workspace will generate further remove_workspace event so ensure this is done after
            # removing record from _data_workspaces to avoid circular call
            if bgsub_ws_name:
                DeleteWorkspace(bgsub_ws_name)
            self.update_sample_log_workspace_group()
            return removed
        else:
            ws_loaded_name = self._data_workspaces.get_loaded_workspace_name_from_bgsub(name)
            if ws_loaded_name:
                removed = self._data_workspaces[ws_loaded_name].bgsub_ws
                self._data_workspaces[ws_loaded_name].bgsub_ws = None
                self._data_workspaces[ws_loaded_name].bgsub_ws_name = None
                self._data_workspaces[ws_loaded_name].bg_params = []
                return removed

    def replace_workspace(self, name, workspace):
        self._data_workspaces.replace_workspace(name, workspace)

    def update_workspace_name(self, old_name, new_name):
        if new_name not in self.get_all_workspace_names():
            self._data_workspaces.rename(old_name, new_name)
            current_log_values = self._sample_logs_workspace_group.get_log_values()
            if old_name in current_log_values:
                self._sample_logs_workspace_group.update_log_value(new_key=new_name, old_key=old_name)
        else:
            logger.warning(f"There already exists a workspace with name {new_name}.")
            self.update_sample_log_workspace_group()

    # handle ADS clear
    def clear_workspaces(self):
        self._data_workspaces.clear()
        self.set_log_workspaces_none()

    def delete_workspaces(self):
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        if current_log_workspaces:
            ws_name = current_log_workspaces.name()
            self._sample_logs_workspace_group.clear_log_workspaces()
            DeleteWorkspace(ws_name)
        removed_ws_list = []
        for ws_name in self._data_workspaces.get_loaded_workpace_names():
            removed_ws_list.extend(self.delete_workspace(ws_name))
        return removed_ws_list

    def delete_workspace(self, loaded_ws_name):
        removed = self._data_workspaces.pop(loaded_ws_name)
        removed_ws_list = [removed.loaded_ws]
        DeleteWorkspace(removed.loaded_ws)
        if removed.bgsub_ws:
            DeleteWorkspace(removed.bgsub_ws)
            removed_ws_list.append(removed.bgsub_ws)
            self.update_sample_log_workspace_group()
        return removed_ws_list

    # =========
    # Plotting
    # =========

    def plot_result(self, index_histograms, axis):
        gsas_histogram_workspace, reflections = self.load_result_for_plot(index_histograms)
        plot_window_title = self.plot_gsas_histogram(axis, gsas_histogram_workspace, reflections, index_histograms)
        return plot_window_title

    def plot_gsas_histogram(self, axis, gsas_histogram, reflection_positions, histogram_index):
        axis.plot(gsas_histogram, color="#1105f0", label="observed", linestyle="None", marker="+", wkspIndex=0)
        axis.plot(gsas_histogram, color="#246b01", label="calculated", wkspIndex=1)
        axis.plot(gsas_histogram, color="#09acb8", label="difference", wkspIndex=2)
        axis.plot(gsas_histogram, color="#ff0000", label="background", wkspIndex=3)
        _, y_max = axis.get_ylim()

        reflection_labels = self._create_reflection_labels(reflection_positions)

        for i_phase, positions in enumerate(reflection_positions):
            if len(positions) > 0:
                y_offset_positions = [(i_phase + 1) * -0.05 * y_max] * len(positions)
                axis.plot(
                    positions,
                    y_offset_positions,
                    label=reflection_labels[i_phase],
                    linestyle="None",
                    marker="|",
                    mew=1.5,
                    ms=8,
                )
        axis.set_xlabel("Time-of-flight ($\\mu s$)")
        axis.set_ylabel("Normalized Intensity")
        plt.show()

        input_file_name = ""
        if self.data_files:
            if len(self.data_files) == 1:
                input_file_name = os.path.basename(self.data_files[0])
            else:
                input_file_name = os.path.basename(self.data_files[histogram_index - 1])
        return "GSAS-II Refinement " + input_file_name

    def _create_reflection_labels(self, reflection_positions: List[np.array]) -> List[str]:
        """Create the labels used to identify different phase reflection lists
        The primary format, if the number of phase names equals the number of positions, is 'reflections_{phase_name}'
        The fallback format is 'reflections_phase_{index}' where index starts at 1
        """
        reflection_labels = [f"reflections_{phase_name}" for phase_name in self.phase_names_list]
        if len(reflection_labels) != len(reflection_positions):
            return [f"reflections_phase_{i}" for i in range(1, len(reflection_positions) + 1)]
        return reflection_labels
