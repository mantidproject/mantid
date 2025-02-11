# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator model for powder TOF data.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_tof_powder_dataset import DNSTofDataset
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_model import DNSScriptGeneratorModel


class DNSTofPowderScriptGeneratorModel(DNSScriptGeneratorModel):
    # pylint: disable=too-many-instance-attributes
    # having the options as instance attributes, is much better readable
    # none of them are public

    def __init__(self, parent):
        super().__init__(parent)
        self._script = None
        self._nb_banks = 0
        self._nb_vana_banks = 0
        self._nb_empty_banks = 0
        self._tof_opt = None
        self._vana_cor = None
        self._bg_cor = None
        self._standard_data = None
        self._sample_data = None

    def _validate_tof_options(self):
        return not (
            self._tof_opt["dE_step"] == 0
            or self._tof_opt["q_step"] == 0
            or self._tof_opt["q_max"] <= self._tof_opt["q_min"]
            or self._tof_opt["dE_max"] <= self._tof_opt["dE_min"]
        )

    def _validate_nb_empty_banks(self):
        return not (
            self._nb_empty_banks == 0
            and self._tof_opt["corrections"]
            and (self._tof_opt["subtract_vana_back"] or self._tof_opt["subtract_sample_back"])
        )

    def _validate_nb_vana_banks(self):
        return not (self._nb_vana_banks == 0 and self._tof_opt["corrections"] and self._tof_opt["det_efficiency"])

    def _check_vana_cor(self):
        return self._tof_opt["corrections"] and self._tof_opt["det_efficiency"] and self._nb_vana_banks > 0

    def _check_bg_cor(self):
        return self._tof_opt["corrections"] and self._nb_empty_banks > 0

    def _error_in_input(self):
        if not self._validate_tof_options():
            return (
                "Binning makes no sense. Make sure that dE and q steps are greater than zero, as well as q_max > q_min and dE_max > dE_min."
            )
        if not self._validate_nb_vana_banks():
            return "Vanadium correction option is chosen, but the number of selected vanadium scans is either 0 or greater than 1."
        if not self._validate_nb_empty_banks():
            return "Background subtraction option is chosen, but the number of selected background scans is either 0 or greater than 1."
        return ""

    def _get_vana_string(self):
        if self._vana_cor:
            return f"          'vana_temperature': {self._tof_opt['vanadium_temperature']},\n"
        return ""

    def _get_back_string(self):
        if self._bg_cor:
            return f"          'ecVanaFactor': {self._tof_opt['vana_back_factor']},\n"
        return ""

    def _get_back_tof_string(self):
        if self._bg_cor:
            return f"          'ecSampleFactor': {self._tof_opt['sample_back_factor']},"
        return ""

    def _get_parameter_lines(self):
        back_string = self._get_back_string()
        vana_string = self._get_vana_string()
        back_tof_string = self._get_back_tof_string()
        return [
            f"params = {{'e_channel': {self._tof_opt['epp_channel']},\n"
            f"          'wavelength': {self._tof_opt['wavelength']},\n"
            f"          'delete_raw': {self._tof_opt['delete_raw']},\n"
            f"{vana_string}{back_string}{back_tof_string}}}",
            "",
        ]

    def _get_binning_lines(self):
        return [
            f"bins = {{'q_min': {self._tof_opt['q_min']:4.3f},\n"
            f"        'q_max': {self._tof_opt['q_max']:4.3f},\n"
            f"        'q_step': {self._tof_opt['q_step']:4.3f},\n"
            f"        'dE_min': {self._tof_opt['dE_min']:4.3f},\n"
            f"        'dE_max': {self._tof_opt['dE_max']:4.3f},\n"
            f"        'dE_step': {self._tof_opt['dE_step']:4.3f}}}",
            "",
        ]

    @staticmethod
    def _get_header_lines():
        lines = [
            "from mantid.simpleapi import MonitorEfficiencyCorUser, FindEPP, mtd",
            "from mantid.simpleapi import ComputeCalibrationCoefVan, Divide, CorrectTOF",
            "from mantid.simpleapi import SaveAscii, SaveNexus, MaskDetectors",
            "from mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof import convert_to_de, get_sqw, load_data",
            "import numpy as np",
        ]
        return lines

    def _get_sample_data_lines(self):
        return [f"sample_data = {self._sample_data.format_dataset()}"]

    def _get_standard_data_lines(self):
        if self._tof_opt["corrections"]:
            return [f"standard_data = {self._standard_data.format_dataset()}"]
        return [""]

    def _get_load_data_lines(self):
        lines = [f'load_data(sample_data["{self._sample_data.get_sample_filename()}"], "raw_data1", params)']
        if self._bg_cor:
            lines += [f'load_data(standard_data["{self._standard_data.get_empty_scan_name()}"], "raw_ec", params)']
        if self._vana_cor:
            lines += [f'load_data(standard_data["{self._standard_data.get_vana_scan_name()}"], "raw_vanadium", params)']
        lines += [""]
        return lines

    def _get_normalisation_lines(self):
        if self._tof_opt["norm_monitor"]:
            return ["# normalize", 'data1 = MonitorEfficiencyCorUser("raw_data1")']
        return ['data1 = mtd["raw_data1"]']

    def _get_subtract_empty_lines(self):
        lines = []
        if self._bg_cor:
            lines = ["", 'ec =  MonitorEfficiencyCorUser("raw_ec")']
            if self._nb_empty_banks != self._nb_banks:
                lines += ["# only one empty can bank", "ec = ec[0]"]
            if self._tof_opt["subtract_sample_back"]:
                lines += ["# subtract empty can", "data1 = data1 - ec * params['ecSampleFactor']", ""]
        return lines

    def _get_vana_ec_subst_lines(self):
        if self._tof_opt["subtract_vana_back"] and self._bg_cor:
            if self._tof_opt["vana_back_factor"] != 1:
                return ["vanadium = vanadium - ec * params['ecVanaFactor']"]
            return ["vanadium = vanadium - ec"]
        return [""]

    def _get_only_one_vana_lines(self):
        if self._nb_vana_banks != self._nb_banks:
            return ["# only one vanadium bank position", "vanadium = vanadium[0]", ""]
        return []

    @staticmethod
    def _get_epp_and_coef_lines():
        return [
            "# detector efficiency correction: compute coefficients",
            "EPP_table = FindEPP(vanadium)",
            "coefs = ComputeCalibrationCoefVan(vanadium, EPP_table, Temperature=params['vana_temperature'])",
        ]

    def _get_corr_epp_lines(self):
        if self._tof_opt["correct_elastic_peak_position"]:
            return ["", "# correct TOF to get EPP at 0 meV", "data1 = CorrectTOF(data1, EPP_table)", ""]
        return []

    def _get_bad_det_lines(self):
        if self._nb_vana_banks > 1 or self._nb_vana_banks == self._nb_banks:
            return ["badDetectors = np.where(np.array(coefs[0].extractY()).flatten() <= 0)[0]"]
        return ["badDetectors = np.where(np.array(coefs.extractY()).flatten() <= 0)[0]"]

    def _get_mask_det_lines(self):
        if self._tof_opt["mask_bad_detectors"]:
            lines = ["# get list of bad detectors"]
            lines += self._get_bad_det_lines()
            lines += [
                'print("Following detectors will be masked: ",badDetectors)',
                "MaskDetectors(data1, DetectorList=badDetectors)",
                "",
            ]
            return lines
        return [""]

    @staticmethod
    def _get_det_eff_cor_lines():
        return ["# apply detector efficiency correction", "data1 = Divide(data1, coefs)"]

    def _get_vana_lines(self):
        if self._vana_cor:
            lines = ['vanadium =  MonitorEfficiencyCorUser("raw_vanadium")']
            lines += self._get_vana_ec_subst_lines()
            lines += self._get_only_one_vana_lines()
            lines += self._get_epp_and_coef_lines()
            lines += self._get_mask_det_lines()
            lines += self._get_det_eff_cor_lines()
            lines += self._get_corr_epp_lines()
            return lines
        return [""]

    @staticmethod
    def _get_energy_print_lines():
        return ["# get Ei", "Ei = data1[0].getRun().getLogData('Ei').value", "print ('Incident Energy is {} meV'.format(Ei))", ""]

    @staticmethod
    def _get_sqw_lines():
        return [
            "# get S(q,w)",
            "convert_to_de('data1', Ei)",
            "",
            "# merge al detector positions together",
            "get_sqw('data1_dE_S', 'data1', bins)",
        ]

    def _get_save_lines(self, paths):
        lines = []
        ascii, nexus = self._check_if_to_save(paths)
        if ascii:
            lines += [f"SaveAscii('data1_dE_S', '{paths['export_dir']}/data1_dE_S.csv', WriteSpectrumID=False)"]
        if nexus:
            lines += [f"SaveNexus('data1_dE_S', '{paths['export_dir']}/data1_dE_S.nxs')"]
        return lines

    @staticmethod
    def _check_if_to_save(paths):
        ascii = paths["ascii"] and paths["export"] and bool(paths["export_dir"])
        nexus = paths["nexus"] and paths["export"] and bool(paths["export_dir"])
        return [ascii, nexus]

    def _setup_sample_data(self, paths, file_selector):
        self._sample_data = DNSTofDataset(data=file_selector["full_data"], path=paths["data_dir"], is_sample=True)
        self._nb_banks = self._sample_data.get_nb_sample_banks()

    def _setup_standard_data(self, paths, file_selector):
        if self._tof_opt["corrections"]:
            self._standard_data = DNSTofDataset(
                data=file_selector["standard_data_tree_model"], path=paths["standards_dir"], is_sample=False
            )
            self._nb_vana_banks = self._standard_data.get_nb_vana_banks()
            self._nb_empty_banks = self._standard_data.get_nb_empty_banks()
        else:
            self._nb_vana_banks = 0
            self._nb_empty_banks = 0

    def script_maker(self, options, paths, file_selector=None):
        self._tof_opt = options
        self._script = []

        self._setup_sample_data(paths, file_selector)
        self._setup_standard_data(paths, file_selector)
        self._check_if_to_save(paths)
        # if to do correction
        self._vana_cor = self._check_vana_cor()
        self._bg_cor = self._check_bg_cor()
        # validate if input binning makes sense, otherwise return
        error = self._error_in_input()
        if error:
            return [""], error

        # start writing script
        self._add_lines_to_script(self._get_header_lines())
        self._add_lines_to_script(self._get_sample_data_lines())
        self._add_lines_to_script(self._get_standard_data_lines())
        self._add_lines_to_script(self._get_parameter_lines())
        self._add_lines_to_script(self._get_binning_lines())
        self._add_lines_to_script(self._get_load_data_lines())
        self._add_lines_to_script(self._get_normalisation_lines())
        self._add_lines_to_script(self._get_subtract_empty_lines())
        self._add_lines_to_script(self._get_vana_lines())
        self._add_lines_to_script(self._get_energy_print_lines())
        self._add_lines_to_script(self._get_sqw_lines())
        self._add_lines_to_script(self._get_save_lines(paths))
        return self._script, error
