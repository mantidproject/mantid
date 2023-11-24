# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator model for elastic single crystal data.
"""

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset import \
    DNSElasticDataset
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import \
    get_normalisation
from mantidqtinterfaces.dns_powder_tof.script_generator. \
    common_script_generator_model import \
    DNSScriptGeneratorModel

import numpy as np
from mantid.simpleapi import mtd


class DNSElasticSCScriptGeneratorModel(DNSScriptGeneratorModel):
    # pylint: disable=too-many-instance-attributes
    # having the options as instance attributes, is much better readable
    # none of them are public

    def __init__(self, parent):
        super().__init__(parent)
        self._data_arrays = {}
        self._script = []
        self._plot_list = []
        self._sample_data = None
        self._standard_data = None
        self._loop = None
        self._spacing = None
        self._vana_correction = None
        self._nicr_correction = None
        self._sample_background_correction = None
        self._background_factor = None
        self._ignore_vana = None
        self._sum_sf_nsf = None
        self._non_magnetic = None
        self._xyz = None
        self._corrections = None
        self._export_path = None
        self._ascii = None
        self._nexus = None
        self._norm = None

    def script_maker(self, options, paths, file_selector=None):
        self._script = []

        # shortcuts for options
        self._vana_correction = options['corrections'] and options['det_efficiency']
        self._nicr_correction = options['corrections'] and options['flipping_ratio']
        self._sample_background_correction = (options['corrections']
                                              and options['subtract_background_from_sample'])
        self._background_factor = options['background_factor']
        self._ignore_vana = str(options['ignore_vana_fields'])
        self._sum_sf_nsf = str(options['sum_vana_sf_nsf'])
        self._non_magnetic = options['separation'] and options['separation_coh_inc']
        self._xyz = options['separation'] and options['separation_xyz']
        self._corrections = (self._sample_background_correction or self._vana_correction
                             or self._nicr_correction)
        self._export_path = paths['export_dir']
        self._ascii = (paths['ascii'] and paths['export']
                       and bool(self._export_path))
        self._nexus = (paths['nexus'] and paths['export']
                       and bool(self._export_path))
        self._norm = get_normalisation(options)

        self._setup_sample_data(paths, file_selector)
        self._setup_standard_data(paths, file_selector)
        self._set_loop()

        # starting writing script
        self._add_lines_to_script(self._get_header_lines())
        self._add_lines_to_script(self._get_sample_data_lines())
        self._add_lines_to_script(self._get_standard_data_lines())
        self._add_lines_to_script(self._get_param_lines(options))
        self._add_lines_to_script(self._get_binning_lines(options))
        self._add_lines_to_script(self._get_load_data_lines())
        self._add_lines_to_script(self._get_bg_corr_lines())
        self._add_lines_to_script(self._get_vanac_lines())
        self._add_lines_to_script(self._get_nicrc_lines())
        return self._script, ''

    def _setup_sample_data(self, paths, file_selector):
        self._sample_data = DNSElasticDataset(data=file_selector['full_data'],
                                              path=paths['data_dir'],
                                              is_sample=True)
        self._plot_list = self._sample_data.create_subtract()

    def _setup_standard_data(self, paths, file_selector):
        if self._corrections:
            self._standard_data = DNSElasticDataset(data=file_selector['standard_data_tree_model'],
                                                    path=paths['standards_dir'],
                                                    is_sample=False,
                                                    banks=self._sample_data['banks'])

    def _set_loop(self):
        if len(self._sample_data.keys()) == 1:
            self._loop = "for workspace in wss_sample" \
                         f"['{list(self._sample_data.keys())[0]}']:"
            self._spacing = "\n" + " " * 4
        else:
            self._loop = "for sample, workspacelist in wss_sample.items(): " \
                         "\n    for workspace in workspacelist:"
            self._spacing = "\n" + " " * 8

    @staticmethod
    def _get_header_lines():
        lines = [
            'from mantidqtinterfaces.dns_single_crystal_elastic.scripts.'
            'md_single_crystal_elastic import load_all',
            'from mantidqtinterfaces.dns_single_crystal_elastic.scripts.'
            'md_single_crystal_elastic import '
            'vanadium_correction, flipping_ratio_correction',
            'from mantidqtinterfaces.dns_single_crystal_elastic.scripts.'
            'md_single_crystal_elastic import background_subtraction',
            'from mantid.simpleapi import ConvertMDHistoToMatrixWorkspace,'
            ' mtd',
            'from mantid.simpleapi import SaveAscii, SaveNexus', ''
        ]
        return lines

    def _get_sample_data_lines(self):
        return [f'sample_data = {self._sample_data.format_dataset()}']

    def _get_standard_data_lines(self):
        if self._corrections:
            return [
                f'standard_data = {self._standard_data.format_dataset()}'
            ]
        return ['']

    def _get_param_lines(self, options):
        return ["", f"params = {{'a': {options['a']}, "
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
                    f"\n          'dy': '{options['dx']}',"
                    "}", ""]

    def _get_binning_lines(self, options):
        bin_edge_min = options['two_theta_min'] - options['two_theta_bin_size'] / 2
        bin_edge_max = options['two_theta_max'] + options['two_theta_bin_size'] / 2
        num_bins = int(round((bin_edge_max - bin_edge_min) / options['two_theta_bin_size']))
        binning = [bin_edge_min, bin_edge_max, num_bins]
        lines = [
            f"binning = {{'twoTheta': {binning},\n"
            f"           'Omega':  ["
            f"{self._sample_data.omega.bin_edge_min:.3f},"
            f" {self._sample_data.omega.bin_edge_max:.3f},"
            f" {self._sample_data.omega.nbins:d}]}} # min, max,"
            " number_of_bins"]
        return lines

    def _get_load_data_lines(self):
        lines = [
            "wss_sample = load_all(sample_data, binning, params)"
        ]
        if self._corrections:
            lines += ["wss_standard = load_all(standard_data, "
                      "binning, params, standard=True)"
                      ]
        lines += ['']
        return lines

    def _get_bg_corr_lines(self):
        lines = []
        if self._vana_correction or self._nicr_correction:
            lines = [
                "# subtract background from vanadium and nicr",
                "for sample, workspacelist in wss_standard.items(): "
                "\n    for workspace in workspacelist:"
                "\n        background_subtraction(workspace)", ""
            ]
        return lines

    def _return_sample_bg_string(self):
        return f"{self._spacing}background_subtraction(workspace, " \
               f"factor={self._background_factor})"

    def _return_sample_vanac_strinf(self):
        return f"{self._spacing}vanadium_correction(workspace, " \
               " vana_set=standard_data['vana'], " \
               f"ignore_vana_fields={self._ignore_vana}, " \
               f"sum_vana_sf_nsf={self._sum_sf_nsf})"

    def _get_vanac_lines(self):
        backgroundstring = self._return_sample_bg_string()
        vanacstring = self._return_sample_vanac_strinf()
        lines = []
        if self._sample_background_correction or self._vana_correction:
            lines = ["# correct sample data",
                     f"{self._loop}{backgroundstring}{vanacstring}"]
        return lines

    def _get_nicrc_lines(self):
        lines = []
        if self._nicr_correction:
            lines = [f"{self._loop}{self._spacing}"
                     "flipping_ratio_correction(workspace)"]
        return lines

    def get_plotlist(self):
        for plot in self._plot_list:
            self._data_arrays[plot] = {
                'ttheta': self._sample_data.ttheta.range,
                'omega': self._sample_data.omega.range,
                'intensity': mtd[plot].getSignalArray(),
                'error': np.sqrt(mtd[plot].getErrorSquaredArray())
            }
        return self._plot_list, self._data_arrays
