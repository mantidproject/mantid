# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import os
import re
from collections import defaultdict
from string import Formatter

from mantid.api import *
from mantid.kernel import *
import isis_powder.gem_routines

_MAUD_TEMPLATE_PATH = None


def _maud_template_path():
    global _MAUD_TEMPLATE_PATH
    if _MAUD_TEMPLATE_PATH is None:
        _MAUD_TEMPLATE_PATH = os.path.join(os.path.dirname(isis_powder.gem_routines.__file__), "maud_param_template.maud")
    return _MAUD_TEMPLATE_PATH


class SaveGEMMAUDParamFile(PythonAlgorithm):
    PROP_INPUT_WS = "InputWorkspace"
    PROP_TEMPLATE_FILE = "TemplateFilename"
    PROP_GROUPING_SCHEME = "GroupingScheme"
    PROP_GSAS_PARAM_FILE = "GSASParamFile"
    PROP_OUTPUT_FILE = "OutputFilename"

    BANK_PARAMS_LINE = "^INS  [0-9]BNKPAR"
    DIFFRACTOMETER_CONSTANTS_LINE = "^INS  [0-9] ICONS"
    PROFILE_COEFFS_LINE_1 = "^INS  [0-9]PRCF 1"
    PROFILE_COEFFS_LINE_2 = "^INS  [0-9]PRCF 2"

    def category(self):
        return "DataHandling\\Text;Diffraction\\DataHandling"

    def name(self):
        return "SaveGEMMAUDParamFile"

    def summary(self):
        return (
            "Read calibration information from focused workspace and GSAS parameter file, and save to " "MAUD-readable calibration format"
        )

    def PyInit(self):
        self.declareProperty(
            WorkspaceGroupProperty(name=self.PROP_INPUT_WS, defaultValue="", direction=Direction.Input),
            doc="WorkspaceGroup of focused banks",
        )

        self.declareProperty(
            FileProperty(name=self.PROP_GSAS_PARAM_FILE, action=FileAction.Load, defaultValue=""),
            doc="GSAS parameter file to read diffractometer constants and profile coefficients from",
        )

        self.declareProperty(
            FileProperty(name=self.PROP_TEMPLATE_FILE, action=FileAction.Load, defaultValue=_maud_template_path()),
            doc="Template for the .maud file",
        )

        self.declareProperty(
            IntArrayProperty(name=self.PROP_GROUPING_SCHEME),
            doc="An array of bank IDs, where the value at element i is the ID of the bank in "
            + self.PROP_GSAS_PARAM_FILE
            + " to associate spectrum i with",
        )

        self.declareProperty(
            FileProperty(name=self.PROP_OUTPUT_FILE, action=FileAction.Save, defaultValue=""), doc="Name of the file to save to"
        )

    def PyExec(self):
        input_ws = mtd[self.getPropertyValue(self.PROP_INPUT_WS)]
        gsas_filename = self.getProperty(self.PROP_GSAS_PARAM_FILE).value

        num_banks = input_ws.getNumberOfEntries()
        grouping_scheme = self.getProperty(self.PROP_GROUPING_SCHEME).value

        # closure around self._expand_to_texture_bank, capturing num_banks and grouping_scheme
        def expand_to_texture_bank(bank_param_list):
            return self._expand_to_texture_bank(
                bank_param_list=bank_param_list, spectrum_numbers=range(num_banks), grouping_scheme=grouping_scheme
            )

        output_params = {}

        gsas_file_params = self._parse_gsas_param_file(gsas_filename)
        gsas_file_params_to_write = {
            key: self._format_param_list(expand_to_texture_bank(gsas_file_params[key])) for key in gsas_file_params
        }
        output_params.update(gsas_file_params_to_write)

        two_thetas, phis = zip(*[self._get_two_theta_and_phi(bank) for bank in input_ws])
        output_params["thetas"] = self._format_param_list(two_thetas)
        output_params["etas"] = self._format_param_list(phis)

        def create_empty_param_list(default_value="0"):
            return "\n".join(default_value for _ in range(num_banks))

        template_file_path = self.getProperty(self.PROP_TEMPLATE_FILE).value
        if len(template_file_path) == 0:
            logger.error("Could not find default diffraction directory for .maud template file: " "you'll have to find it yourself")

        with open(template_file_path) as template_file:
            template = template_file.read()

        output_params["function_types"] = create_empty_param_list("1")
        output_params["gsas_prm_file"] = gsas_filename
        output_params["inst_counter_bank"] = "Bank1"
        output_params["bank_ids"] = "\n".join("Bank{}".format(i + 1) for i in range(num_banks))

        with open(self.getProperty(self.PROP_OUTPUT_FILE).value, "w") as output_file:
            # Note, once we've got rid of Python 2 support this can be simplified to
            # template.format_map(**defaultdict(create_empty_param_list, output_params))
            output_file.write(Formatter().vformat(template, (), defaultdict(create_empty_param_list, output_params)))

    def validateInputs(self):
        issues = {}

        input_ws = mtd[self.getPropertyValue(self.PROP_INPUT_WS)]
        grouping_scheme = self.getProperty(self.PROP_GROUPING_SCHEME).value
        if len(grouping_scheme) != input_ws.getNumberOfEntries():
            issues[self.PROP_GROUPING_SCHEME] = (
                "Number of entries in {} does not match number of spectra in {}. "
                "You must assign a bank to every focused spectrum in the input workspace".format(
                    self.PROP_GROUPING_SCHEME, self.PROP_INPUT_WS
                )
            )

        return issues

    def _expand_to_texture_bank(self, bank_param_list, spectrum_numbers, grouping_scheme):
        """
        :param bank_param_list: a list of n values for some parameter, such as DIFC
        :param spectrum_numbers: a list of m bank IDs, one for each focused spectrum
        :param grouping_scheme: a list of m indexes, to index bank_param_list, where
        the element at item i is the bank number in bank_param_list to associate
        focused spectrum i with
        :return: a list of m values, where each value is the assigned parameter for the
        corresponding bank
        """
        return (bank_param_list[grouping_scheme[spec_num] - 1] for spec_num in spectrum_numbers)

    def _format_param_list(self, param_list):
        return "\n".join(str(param) for param in param_list)

    def _get_two_theta_and_phi(self, bank):
        instrument = bank.getInstrument()
        detector = bank.getDetector(0)

        sample_pos = instrument.getSample().getPos()
        source_pos = instrument.getSource().getPos()
        det_pos = detector.getPos()

        beam_dir = sample_pos - source_pos
        detector_dir = det_pos - sample_pos

        return math.degrees(beam_dir.angle(detector_dir)), math.degrees(detector.getPhi())

    def _parse_gsas_param_file(self, gsas_filename):
        with open(gsas_filename) as prm_file:
            gsas_params_lines = prm_file.read().split("\n")

        distances = []
        difcs = []
        difas = []
        tzeros = []
        alpha_zeros = []
        alpha_ones = []
        beta_zeros = []
        beta_ones = []
        sigma_zeros = []
        sigma_ones = []
        sigma_twos = []
        for line in gsas_params_lines:
            line_items = line.split()
            if re.match(self.BANK_PARAMS_LINE, line):
                distances.append(float(line_items[2]))
            elif re.match(self.DIFFRACTOMETER_CONSTANTS_LINE, line):
                difcs.append(float(line_items[3]))
                difas.append(float(line_items[4]))
                tzeros.append(float(line_items[5]))
            elif re.match(self.PROFILE_COEFFS_LINE_1, line):
                alpha_zeros.append(float(line_items[3]))
                alpha_ones.append(float(line_items[4]))
                beta_zeros.append(float(line_items[5]))
                beta_ones.append(float(line_items[6]))
            elif re.match(self.PROFILE_COEFFS_LINE_2, line):
                sigma_zeros.append(float(line_items[3]))
                sigma_ones.append(float(line_items[4]))
                sigma_twos.append(float(line_items[5]))

        return {
            "dists": distances,
            "difas": difas,
            "difcs": difcs,
            "tzeros": tzeros,
            "func_1_alpha_zeros": alpha_zeros,
            "func_1_alpha_ones": alpha_ones,
            "func_1_beta_zeros": beta_zeros,
            "func_1_beta_ones": beta_ones,
            "func_1_sigma_zeros": sigma_zeros,
            "func_1_sigma_ones": sigma_ones,
            "func_1_sigma_twos": sigma_twos,
        }


AlgorithmFactory.subscribe(SaveGEMMAUDParamFile)
