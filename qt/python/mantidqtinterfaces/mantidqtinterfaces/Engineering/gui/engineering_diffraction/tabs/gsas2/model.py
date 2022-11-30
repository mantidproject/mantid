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
from mantid.geometry import CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from mantid.simpleapi import CreateWorkspace, LoadGSS, DeleteWorkspace, CreateEmptyTableWorkspace, logger
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2 import parse_inputs
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting


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
        self.chosen_cell_lengths = None
        self.out_call_gsas2 = None
        self.err_call_gsas2 = None

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
        self.chosen_cell_lengths = None
        self.out_call_gsas2 = None
        self.err_call_gsas2 = None

    def run_model(self, load_parameters, refinement_parameters, project_name, rb_num, user_limits):

        self.clear_input_components()
        if not self.initial_validation(project_name, load_parameters):
            return None
        self.set_components_from_inputs(load_parameters, refinement_parameters, project_name, rb_num)
        self.evaluate_further_inputs(user_limits)
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
        self.timeout = int(get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                                       "timeout"))
        self.dSpacing_min = float(get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                              output_settings.ENGINEERING_PREFIX,
                                              "dSpacing_min"))

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

    def evaluate_further_inputs(self, user_x_limits):
        if not self.loop_phase_files() or not self.validate_x_limits(user_x_limits):
            return None
        return True

    def loop_phase_files(self):
        self.mantid_pawley_reflections = []
        for loop_phase_filepath in self.phase_filepaths:
            self.chosen_cell_lengths = self.choose_cell_lengths(loop_phase_filepath)
            if not self.chosen_cell_lengths:
                return None
            if self.refinement_method == 'Pawley':
                generated_reflections = self.generate_reflections_from_space_group(loop_phase_filepath,
                                                                                   self.chosen_cell_lengths)
                if not generated_reflections:
                    return None
                self.mantid_pawley_reflections.extend(generated_reflections)
        return True

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
        if self.refinement_method == 'Pawley' and not self.mantid_pawley_reflections:
            logger.error("No Pawley Reflections were generated for the phases provided. Not calling GSAS-II.")
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
        logger.notice(
            self.format_shell_output(title="Commandline output from GSAS-II", shell_output_string=self.out_call_gsas2))
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
            override_cell_lengths=[float(x) for x in self.chosen_cell_lengths.split(" ")],
            refine_unit_cell=self.refine_unit_cell,
            d_spacing_min=self.dSpacing_min,
            number_of_regions=self.number_of_regions
        )
        gsas2_python_path = os.path.join(self.path_to_gsas2, "bin", "python")
        if platform.system() == "Windows":
            gsas2_python_path = os.path.join(self.path_to_gsas2, "python")
        call = [gsas2_python_path,
                os.path.abspath(os.path.join(os.path.dirname(__file__), "call_G2sc.py")),
                parse_inputs.Gsas2Inputs_to_json(gsas2_inputs)]
        return call

    def call_subprocess(self, command_string_list):
        try:
            env = os.environ.copy()
            env['PYTHONHOME'] = self.path_to_gsas2
            shell_process = subprocess.Popen(command_string_list,
                                             shell=False,
                                             stdin=None,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE,
                                             close_fds=True,
                                             universal_newlines=True,
                                             env=env)
            shell_output = shell_process.communicate(timeout=self.timeout)
            return shell_output
        except subprocess.TimeoutExpired:
            shell_process.terminate()
            logger.error(f"GSAS-II call did not complete after {self.timeout} seconds, so it was"
                         f" aborted. Check the inputs, such as Refinement Method are correct. The"
                         f" timeout interval can be increased in the Engineering Diffraction Settings.")
            return None

    def format_shell_output(self, title, shell_output_string):
        double_line = "-" * (len(title) + 2) + "\n" + "-" * (len(title) + 2)
        return "\n" * 3 + double_line + "\n " + title + " \n" + double_line + "\n" \
               + shell_output_string + "\n" + double_line + "\n" * 3

    # ===========
    # Read Files
    # ===========

    def find_in_file(self, file_path, marker_string, start_of_value, end_of_value, strip_separator=None):
        value_string = None
        if os.path.exists(file_path):
            with open(file_path, 'rt', encoding='utf-8') as file:
                full_file_string = file.read().replace('\n', '')
                where_marker = full_file_string.rfind(marker_string)
                if where_marker != -1:
                    where_value_start = full_file_string.find(start_of_value, where_marker)
                    if where_value_start != -1:
                        where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                        value_string = full_file_string[where_value_start: where_value_end]
                        if strip_separator:
                            value_string = value_string.strip(strip_separator + " ")
                        else:
                            value_string = value_string.strip(" ")
        return value_string

    def find_basis_block_in_file(self, file_path, marker_string, start_of_value, end_of_value):
        value_string = None
        if os.path.exists(file_path):
            with open(file_path, 'rt', encoding='utf-8') as file:
                full_file_string = file.read()
                where_marker = full_file_string.find(marker_string)
                if where_marker != -1:
                    where_first_digit = -1
                    index = int(where_marker)
                    while index < len(full_file_string):
                        if full_file_string[index].isdecimal():
                            where_first_digit = index
                            break
                        index += 1
                    if where_first_digit != -1:
                        where_start_of_line = full_file_string.rfind(start_of_value, 0, where_first_digit - 1)
                        if where_start_of_line != -1:
                            where_end_of_block = full_file_string.find(end_of_value, where_start_of_line)
                            # if "loop" not found then assume the end of the file is the end of this block
                            value_string = full_file_string[where_start_of_line: where_end_of_block]
        return value_string

    def read_basis(self, phase_file_path):
        basis_string = self.find_basis_block_in_file(phase_file_path, "atom", "\n", "loop")
        if not basis_string:
            logger.error(f"Invalid Phase file format in {phase_file_path}")
            return None
        list_of_lines = basis_string.split("\n")
        list_of_lines = [k for k in list_of_lines if k != ""]
        basis = []
        for line in list_of_lines:
            split_line = line.split()
            split_line = self.remove_elements_after_the_first_that_are_alphabetic(split_line)
            split_line = self.format_element_symbol(split_line)
            basis.append(" ".join(split_line[0:6]))
        return basis

    def remove_elements_after_the_first_that_are_alphabetic(self, line_split):
        indices_to_remove = []
        for index in range(1, len(line_split)):
            if line_split[index].isalpha():
                indices_to_remove.append(index)
        if indices_to_remove:
            indices_to_remove = sorted(indices_to_remove, reverse=True)
            for index in indices_to_remove:
                del line_split[index]
        return line_split

    def format_element_symbol(self, line_split):
        # Now convert FE1 to Fe
        line_split[0] = ''.join([i for i in line_split[0] if not i.isdigit()])
        if len(line_split[0]) == 2:
            line_split[0] = ''.join([line_split[0][0].upper(), line_split[0][1].lower()])
        return line_split

    def read_space_group(self, phase_file_path):
        space_group = self.find_in_file(phase_file_path, "_symmetry_space_group_name_H-M", '"', '"',
                                        strip_separator='"')
        return space_group

    def insert_minus_before_first_digit(self, original_space_group):
        # https://docs.mantidproject.org/nightly/concepts/PointAndSpaceGroups.html
        roto_inverted_space_group = original_space_group
        for character_index, character in enumerate(roto_inverted_space_group):
            if character.isdigit():
                split_string = list(roto_inverted_space_group)
                split_string.insert(character_index, "-")
                roto_inverted_space_group = "".join(split_string)
                break  # only apply to first digit
        return roto_inverted_space_group

    def get_crystal_params_from_instrument(self, instrument):
        crystal_params = []
        if not self.number_of_regions:
            crystal_params = [self.find_in_file(instrument, "ICONS", "S", "INS", strip_separator="ICONS\t")]

        else:
            for region_index in range(1, self.number_of_regions+1):
                loop_crystal_params = self.find_in_file(instrument, f"{region_index} ICONS", "S", "INS",
                                                        strip_separator="ICONS\t")
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
                tof_min.append((dif_c * self.dSpacing_min) + (dif_a * (self.dSpacing_min ** 2)) + t_zero)
        if not tof_min:
            logger.error(file_mismatch)
            return None
        return tof_min

    # ===================
    # Pawley Reflections
    # ===================

    def choose_cell_lengths(self, phase_file_path: str):
        if self.override_cell_length_string:
            if "," not in self.override_cell_length_string:
                overriding_cell_lengths_list = [float(self.override_cell_length_string)] * 3
            else:
                overriding_cell_lengths_list = [float(x) for x in self.override_cell_length_string.split(",")]

            if len(overriding_cell_lengths_list) != 3:
                logger.error(f"The number of Override Cell Length values ({len(overriding_cell_lengths_list)}) "
                             f"must be 1 or 3 (and separated by commas).")
                return None

            cell_lengths = " ".join([str(overriding_cell_lengths_list[0]),
                                     str(overriding_cell_lengths_list[1]),
                                     str(overriding_cell_lengths_list[2])])
        else:
            cell_length_a = self.find_in_file(phase_file_path, '_cell_length_a', ' ', '_cell_length_b')
            cell_length_b = self.find_in_file(phase_file_path, '_cell_length_b', ' ', '_cell_length_c')
            cell_length_c = self.find_in_file(phase_file_path, '_cell_length_c', ' ', '_cell')
            if not cell_length_a or not cell_length_b or not cell_length_c:
                logger.error(f"Invalid Phase file format in {phase_file_path}")
                return None
            cell_lengths = " ".join([cell_length_a, cell_length_b, cell_length_c])
        return cell_lengths

    def create_pawley_reflections(self, cell_lengths, space_group, basis):
        generated_reflections = []
        try:
            for atom in basis:
                structure = CrystalStructure(cell_lengths, space_group, atom)

                generator = ReflectionGenerator(structure)

                hkls = generator.getUniqueHKLsUsingFilter(self.dSpacing_min, 3.0, ReflectionConditionFilter.StructureFactor)
                dValues = generator.getDValues(hkls)
                pg = structure.getSpaceGroup().getPointGroup()
                # Make list of tuples and sort by d-values, descending, include point group for multiplicity.
                loop_reflections = sorted([[list(hkl), d, len(pg.getEquivalents(hkl))] for hkl, d in zip(hkls, dValues)],
                                          key=lambda x: x[1] - x[0][0] * 1e-6, reverse=True)
                generated_reflections.extend(loop_reflections)
        except RuntimeError:
            logger.error(f"Check the Refinement Method (now {self.refinement_method}) is set correctly. "
                         f"The current inputs are causing an unidentifiable C++ exception.")
            return None
        return generated_reflections

    def generate_reflections_from_space_group(self, phase_filepath, cell_lengths):

        space_group = self.read_space_group(phase_filepath)
        found_basis = self.read_basis(phase_filepath)
        if not found_basis:
            return None
        try:
            mantid_pawley_reflections = self.create_pawley_reflections(cell_lengths, space_group,
                                                                       found_basis)
        except ValueError:
            roto_inversion_space_group = self.insert_minus_before_first_digit(space_group)
            try:
                mantid_pawley_reflections = self.create_pawley_reflections(cell_lengths, roto_inversion_space_group,
                                                                           found_basis)
                if mantid_pawley_reflections:
                    logger.warning(f"Note the roto-inversion space group {roto_inversion_space_group} has been "
                                   f"used rather than {space_group} read from {phase_filepath} as "
                                   f"it is not in the accepted list of space groups.")
            except ValueError:
                from mantid.geometry import SpaceGroupFactory
                space_group_list = SpaceGroupFactory.getAllSpaceGroupSymbols()
                logger.error(f"Space group {space_group} read from {phase_filepath} and its "
                             f"roto_inversion {roto_inversion_space_group} are not in the accepted list"
                             f"of space groups: \n\n {space_group_list} \n\n The phase file may need to be"
                             f"edited. For more information see: "
                             f"https://docs.mantidproject.org/nightly/concepts/PointAndSpaceGroups.html")

        return mantid_pawley_reflections

    # =========
    # X Limits
    # =========

    def determine_x_limits(self):
        tof_min = self.determine_tof_min()
        if not tof_min:
            return None
        for workspace_index in range(len(self.x_min)):
            if tof_min[workspace_index] > self.x_min[workspace_index]:
                self.x_min[workspace_index] = tof_min[workspace_index]
        return True

    def determine_tof_min(self):
        tof_min = []
        if self.number_of_regions > 1 and len(self.instrument_files) == 1:
            tof_min = self.get_crystal_params_from_instrument(self.instrument_files[0])
            if not tof_min:
                return None
        elif self.number_of_regions == len(self.instrument_files):
            for input_instrument in self.instrument_files:
                loop_instrument_tof_min = self.get_crystal_params_from_instrument(input_instrument)
                if not loop_instrument_tof_min:
                    return None
                tof_min.append(loop_instrument_tof_min[0])
        return tof_min

    def understand_data_structure(self):
        self.data_x_min = []
        self.data_x_max = []
        number_of_regions = 0
        for input_file in self.data_files:
            loop_focused_workspace = LoadGSS(Filename=input_file, OutputWorkspace="GSASII_input_data")
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
            logger.error(f"The number of instrument files ({len(self.instrument_files)}) must be 1 "
                         f"or equal to the number of input histograms {self.number_histograms}")
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
            success = self.determine_x_limits()
            if not success:
                return None
            if not self.x_min:
                logger.error("Could not determine Minimum and Maximum X values from input files. "
                             "Please set these values in the Plot Widget on the GSAS-II tab.")
                return None
        self.limits = [self.x_min, self.x_max]
        return True

    # ===============
    # Handle Outputs
    # ===============

    def read_gsas_lst_and_print_wR(self, result_filepath, histogram_data_files, test=False):
        with open(result_filepath, 'rt', encoding='utf-8') as file:
            result_string = file.read().replace('\n', '')
            for loop_histogram in histogram_data_files:
                where_loop_histogram = result_string.rfind(loop_histogram)
                if where_loop_histogram != -1:
                    where_loop_histogram_wR = result_string.find('Final refinement wR =', where_loop_histogram)
                    if where_loop_histogram_wR != -1:
                        where_loop_histogram_wR_end = result_string.find('%', where_loop_histogram_wR)
                        logger.notice(loop_histogram)
                        logged_lst_result = result_string[where_loop_histogram_wR: where_loop_histogram_wR_end + 1]
                        logger.notice(logged_lst_result)
                        if test:
                            return logged_lst_result

    def find_phase_names_in_lst(self, file_path):
        phase_names = []
        marker_string = "Phase name"
        start_of_value = ":"
        end_of_value = "="
        strip_separator = ":"
        with open(file_path, 'rt', encoding='utf-8') as file:
            full_file_string = file.read().replace('\n', '')
            where_marker = full_file_string.find(marker_string)
            while where_marker != -1:
                where_value_start = full_file_string.find(start_of_value, where_marker)
                if where_value_start != -1:
                    where_value_end = full_file_string.find(end_of_value, where_value_start + 1)
                    phase_names.append(
                        full_file_string[where_value_start: where_value_end].strip(strip_separator + " "))
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
            logged_failure = f"GSAS-II call must have failed, as the output {file_descriptor} file was not found." \
                + self.format_shell_output(title="Errors from GSAS-II", shell_output_string=self.err_call_gsas2)
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
        self.temporary_save_directory = os.path.join(save_directory,
                                                     datetime.datetime.now().strftime(
                                                         'tmp_EngDiff_GSASII_%Y-%m-%d_%H-%M-%S'))
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
            os.rename(os.path.join(self.temporary_save_directory, output_file),
                      os.path.join(self.user_save_directory, output_file))
            if exist_extra_save_dirs:
                for extra_save_dir in self.gsas2_save_dirs:
                    shutil.copy(os.path.join(self.user_save_directory, output_file),
                                os.path.join(extra_save_dir, output_file))
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

        self.create_lattice_parameter_table()

    def load_result(self, index_histograms):
        workspace = self.load_gsas_histogram(index_histograms)
        phase_names_list = self.find_phase_names_in_lst(
            os.path.join(self.user_save_directory, self.project_name + ".lst"))

        reflections = None
        if self.refinement_method == "Pawley":
            reflections = self.load_gsas_reflections(index_histograms, phase_names_list)
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
        y_obs = self.chop_to_limits(np.array(my_data[1]), x_values, self.x_min[histogram_index - 1],
                                    self.x_max[histogram_index - 1])
        y_calc = self.chop_to_limits(np.array(my_data[3]), x_values, self.x_min[histogram_index - 1],
                                     self.x_max[histogram_index - 1])
        y_diff = y_obs - y_calc
        y_diff -= np.max(np.ma.masked_invalid(y_diff))
        y_bkg = self.chop_to_limits(np.array(my_data[4]), x_values, self.x_min[histogram_index - 1],
                                    self.x_max[histogram_index - 1])
        y_data = np.concatenate((y_obs, y_calc, y_diff, y_bkg))

        gsas_histogram = CreateWorkspace(OutputWorkspace=f"gsas_histogram_{histogram_index}",
                                         DataX=np.tile(my_data[0], 4), DataY=y_data, NSpec=4)
        return gsas_histogram

    def load_gsas_reflections(self, histogram_index, phase_names):
        loaded_reflections = []
        for phase_name in phase_names:
            result_reflections_txt = os.path.join(self.user_save_directory,
                                                  self.project_name + f"_reflections_{histogram_index}_{phase_name}.txt")
            loaded_reflections.append(np.loadtxt(result_reflections_txt))
        return loaded_reflections

    def create_lattice_parameter_table(self, test=False):
        LATTICE_TABLE_PARAMS = ["length_a", "length_b", "length_c",
                                "angle_alpha", "angle_beta", "angle_gamma", "volume"]
        phase_names_list = self.find_phase_names_in_lst(
            os.path.join(self.user_save_directory, self.project_name + ".lst"))

        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_lattice_parameters")
        table.addColumn("str", "Phase")
        for param in LATTICE_TABLE_PARAMS:
            table.addColumn("double", param.split("_")[-1])
        for phase_name in phase_names_list:
            parameters_txt = os.path.join(self.user_save_directory,
                                          self.project_name + f"_cell_parameters_{phase_name}.txt")
            with open(parameters_txt, 'rt', encoding='utf-8') as file:
                full_file_string = file.read().replace('\n', '')
            parameter_dict = json.loads(full_file_string)
            loop_parameters = [phase_name]
            for param in LATTICE_TABLE_PARAMS:
                loop_parameters.append(float(parameter_dict[param]))
            table.addRow(loop_parameters)
        if test:
            return table

    # =========
    # Plotting
    # =========

    def plot_result(self, index_histograms, axis):
        gsas_histogram_workspace, reflections = self.load_result(index_histograms)
        plot_window_title = self.plot_gsas_histogram(axis, gsas_histogram_workspace, reflections,
                                                     index_histograms, self.data_files)
        return plot_window_title

    def plot_gsas_histogram(self, axis, gsas_histogram, reflection_positions, histogram_index, data_file_list):
        axis.plot(gsas_histogram, color='#1105f0', label='observed', linestyle='None', marker='+', wkspIndex=0)
        axis.plot(gsas_histogram, color='#246b01', label='calculated', wkspIndex=1)
        axis.plot(gsas_histogram, color='#09acb8', label='difference', wkspIndex=2)
        axis.plot(gsas_histogram, color='#ff0000', label='background', wkspIndex=3)
        _, y_max = axis.get_ylim()
        if reflection_positions:
            axis.plot(reflection_positions, [-0.10 * y_max] * len(reflection_positions),
                      color='#1105f0', label='reflections', linestyle='None', marker='|', mew=1.5, ms=8)
        axis.set_xlabel('Time-of-flight ($\\mu s$)')
        axis.set_ylabel('Normalized Intensity')
        plt.show()

        input_file_name = ""
        if data_file_list:
            if len(data_file_list) == 1:
                input_file_name = os.path.basename(data_file_list[0])
            else:
                input_file_name = os.path.basename(data_file_list[histogram_index - 1])
        return 'GSAS-II Refinement ' + input_file_name
