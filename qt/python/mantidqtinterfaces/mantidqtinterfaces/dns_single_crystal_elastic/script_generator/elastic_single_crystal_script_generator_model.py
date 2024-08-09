# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator model for elastic single crystal data.
"""

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset import DNSElasticDataset
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import get_normalisation
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_model import DNSScriptGeneratorModel
import numpy as np
from mantid.simpleapi import mtd

INDENT_SPACE = 4 * " "


class DNSElasticSCScriptGeneratorModel(DNSScriptGeneratorModel):
    def __init__(self, parent):
        super().__init__(parent)
        self._data_arrays = {}
        self._script = []
        self._plot_list = []
        self._sample_data = None
        self._standard_data = None
        self._loop = None
        self._spacing = None
        self._export_path = None
        self._ascii = None
        self._nexus = None
        self._norm = None

    def script_maker(self, options, paths, file_selector=None):
        self._script = []
        # shortcuts for options
        self._vana_correction = options["corrections"] and options["det_efficiency"]
        self._nicr_correction = options["corrections"] and options["flipping_ratio"]
        self._sample_background_correction = options["corrections"] and options["subtract_background_from_sample"]
        self._background_factor = options["background_factor"]
        self._ignore_vana = str(options["ignore_vana_fields"])
        self._sum_sf_nsf = str(options["sum_vana_sf_nsf"])
        self._corrections = self._sample_background_correction or self._vana_correction or self._nicr_correction
        self._export_path = paths["export_dir"]
        self._ascii = paths["ascii"] and paths["export"] and bool(self._export_path)
        self._nexus = paths["nexus"] and paths["export"] and bool(self._export_path)
        self._norm = get_normalisation(options)
        self._setup_sample_data(paths, file_selector)
        self._set_loop()
        # validate if input makes sense, otherwise return
        # an empty script and error message
        error = self._check_errors_in_selected_files(options)
        if error:
            self._script = [""]
            error_message = error
        else:
            error_message = ""
            # starting writing script
            self._add_lines_to_script(self._get_header_lines())
            self._add_lines_to_script(self._get_sample_data_lines())
            self._add_lines_to_script(self._get_param_lines(options))
            self._add_lines_to_script(self._get_binning_lines(options))
            self._add_lines_to_script(self._get_load_data_lines())
            save_string = self._get_save_string()
            self._add_lines_to_script(self._get_convert_to_matrix_lines(save_string))
        return self._script, error_message

    def _setup_sample_data(self, paths, file_selector):
        self._sample_data = DNSElasticDataset(data=file_selector["full_data"], path=paths["data_dir"], is_sample=True)
        self._plot_list = self._sample_data.create_subtract()

    def _set_loop(self):
        if len(self._sample_data.keys()) == 1:
            self._loop = f"for workspace in wss_sample['{list(self._sample_data.keys())[0]}']:"
            self._spacing = "\n" + INDENT_SPACE
        else:
            self._loop = f"for sample, workspacelist in wss_sample.items():\n{INDENT_SPACE}for workspace in workspacelist:"
            self._spacing = "\n" + 2 * INDENT_SPACE

    @staticmethod
    def _get_header_lines():
        lines = [
            "from mantidqtinterfaces.dns_single_crystal_elastic.scripts.md_single_crystal_elastic import load_all",
            "from mantid.simpleapi import ConvertMDHistoToMatrixWorkspace, mtd, SaveAscii, SaveNexus",
            "",
        ]
        return lines

    def _get_sample_data_lines(self):
        return [f"sample_data = {self._sample_data.format_dataset()}"]

    def _get_param_lines(self, options):
        return [
            "",
            f"params = {{'a': {options['a']}, "
            f"\n          'b': {options['b']},"
            f"\n          'c': {options['c']},"
            f"\n          'alpha': {options['alpha']},"
            f"\n          'beta': {options['beta']},"
            f"\n          'gamma': {options['gamma']},"
            f"\n          'hkl1': '{options['hkl1']}',"
            f"\n          'hkl2': '{options['hkl2']}',"
            f"\n          'omega_offset': {options['omega_offset']},"
            f"\n          'norm_to': '{self._norm}',"
            f"\n          'dx': '{options['dx']}',"
            f"\n          'dy': '{options['dy']}',"
            "}",
            "",
        ]

    def _get_binning_lines(self, options):
        two_theta_bin_edge_min = options["two_theta_min"] - options["two_theta_bin_size"] / 2
        two_theta_bin_edge_max = options["two_theta_max"] + options["two_theta_bin_size"] / 2
        two_theta_num_bins = int(round((two_theta_bin_edge_max - two_theta_bin_edge_min) / options["two_theta_bin_size"]))
        two_theta_binning = [two_theta_bin_edge_min, two_theta_bin_edge_max, two_theta_num_bins]
        omega_bin_edge_min = options["omega_min"] - options["omega_bin_size"] / 2
        omega_bin_edge_max = options["omega_max"] + options["omega_bin_size"] / 2
        omega_num_bins = int(round((omega_bin_edge_max - omega_bin_edge_min) / options["omega_bin_size"]))
        omega_binning = [omega_bin_edge_min, omega_bin_edge_max, omega_num_bins]
        lines = [
            f"binning = {{'two_theta_binning': {two_theta_binning},\n"
            f"           'omega_binning': {omega_binning}}} # min, max, number_of_bins"
        ]
        return lines

    def _get_load_data_lines(self):
        lines = ["wss_sample = load_all(sample_data, binning, params)"]
        if self._corrections:
            lines += ["wss_standard = load_all(standard_data, binning, params, standard=True)"]
        lines += [""]
        return lines

    def _get_ascii_save_string(self):
        if self._ascii:
            return (
                f"{self._spacing}SaveAscii('mat_{{}}'.format(workspace), "
                f"'{self._export_path}/{{}}.csv'.format(workspace), WriteSpectrumID=False)"
            )
        return ""

    def _get_nexus_save_string(self):
        if self._nexus:
            return f"{self._spacing}SaveNexus('mat_{{}}'.format(workspace), '{self._export_path}/{{}}.nxs'.format(workspace))"
        return ""

    def _get_save_string(self):
        ascii_string = self._get_ascii_save_string()
        nexus_string = self._get_nexus_save_string()
        save_strings = [x for x in [ascii_string, nexus_string] if x]
        return "".join(save_strings)

    def _get_convert_to_matrix_lines(self, save_string):
        lines = [
            "",
            "# convert the output sample MDHistoWorkspaces to MatrixWorkspaces",
            "# and save the data into the directory specified for Export",
            f"{self._loop}{self._spacing}ConvertMDHistoToMatrixWorkspace(workspace, Outputworkspace='mat_{{}}'.format(workspace), "
            f"Normalization='NoNormalization'){save_string}",
            "",
        ]
        if not save_string:
            lines.remove("# and save the data into the directory specified for Export")
        return lines

    def get_plot_list(self, options):
        for plot in self._plot_list:
            self._data_arrays[plot] = {
                "two_theta_array": self._get_sample_data_two_theta_binning_array(options),
                "omega_array": self._get_sample_data_omega_binning_array(options),
                "intensity": mtd[plot].getSignalArray(),
                "error": np.sqrt(mtd[plot].getErrorSquaredArray()),
            }
        return self._plot_list, self._data_arrays

    def _get_sample_data_omega_binning_array(self, options):
        min = round(options["omega_min"], 1)
        max = round(options["omega_max"], 1)
        bin_size = options["omega_bin_size"]
        binning_array = np.arange(min, max + bin_size, bin_size)
        return binning_array

    def _get_sample_data_two_theta_binning_array(self, options):
        min = round(options["two_theta_min"], 1)
        max = round(options["two_theta_max"], 1)
        bin_size = options["two_theta_bin_size"]
        binning_array = np.arange(min, max + bin_size, bin_size)
        return binning_array

    def _check_errors_in_selected_files(self, options):
        """
        If any inconsistencies are found in the files selected for
        data reduction, then this method will return a string with
        the corresponding error_message. If no inconsistencies are
        found then the empty string will be returned.
        """
        keys_to_check = ["wavelength", "dx", "dy", "hkl1", "hkl2", "omega_offset"]
        for key in keys_to_check:
            if options[key] == "" or options[key] == [] or options[key] is None:
                return f"Missing input for {key}."
        return ""
