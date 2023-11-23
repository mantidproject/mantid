# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Common Presenter for DNS Script generators
"""

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_dataset import \
    DNSDataset
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import \
    get_normation
from mantidqtinterfaces.dns_powder_tof.script_generator. \
    common_script_generator_model import \
    DNSScriptGeneratorModel

import numpy as np
from mantid.simpleapi import mtd


class DNSElasticSCScriptGeneratorModel(DNSScriptGeneratorModel):
    # pylint: disable=too-many-instance-attributes
    # having the options as instance attribues, is much better readable
    # none of them are public
    """
    Common Model for DNS Script generators
    """

    def __init__(self, parent):
        super().__init__(parent)
        self._data_arrays = {}
        self._script = []
        self._plotlist = []
        self._sample_data = None
        self._standard_data = None
        self._loop = None
        self._spac = None
        self._vanac = None
        self._nicrc = None
        self._sampb = None
        self._backfac = None
        self._ign_vana = None
        self._sum_sfnsf = None
        self._nonmag = None
        self._xyz = None
        self._corrections = None
        self._export_path = None
        self._ascii = None
        self._nexus = None
        self._norm = None

    def script_maker(self, options, paths, file_selector=None):
        self._script = []
        # shortcuts for options
        self._vanac = options['corrections'] and options['det_efficency']
        self._nicrc = options['corrections'] and options['flipping_ratio']
        self._sampb = (options['corrections']
                       and options['substract_background_from_sample'])
        self._backfac = options['background_factor']
        self._ign_vana = str(options['ignore_vana_fields'])
        self._sum_sfnsf = str(options['sum_vana_sf_nsf'])
        self._xyz = options["separation"] and options['separation_xyz']
        self._nonmag = options["separation"] and options['separation_coh_inc']

        self._corrections = (self._sampb or self._vanac or self._nicrc)
        self._export_path = paths["export_dir"]
        self._ascii = (paths["ascii"] and paths["export"]
                       and bool(self._export_path))
        self._nexus = (paths["nexus"] and paths["export"]
                       and bool(self._export_path))
        self._norm = get_normation(options)

        self._setup_sample_data(paths, file_selector)
        self._setup_standard_data(paths, file_selector)
        self._interpolate_standard()
        self._set_loop()

        # startin wrting script
        self._add_lines_to_script(self._get_header_lines())
        self._add_lines_to_script(self._get_sample_data_lines())
        self._add_lines_to_script(self._get_standard_data_lines())
        self._add_lines_to_script(self._get_param_lines(options))
        self._add_lines_to_script(self._get_binning_lines())
        self._add_lines_to_script(self._get_load_data_lines())
        self._add_lines_to_script(self._get_bg_corr_lines())
        self._add_lines_to_script(self._get_vanac_lines())
        self._add_lines_to_script(self._get_nicrc_lines())
        return self._script, ''

    def _setup_sample_data(self, paths, fselector):
        self._sample_data = DNSDataset(data=fselector['full_data'],
                                       path=paths['data_dir'],
                                       issample=True)
        self._plotlist = self._sample_data.create_plotlist()

    def _setup_standard_data(self, paths, fselector):
        if self._corrections:
            self._standard_data = DNSDataset(data=fselector['standard_data'],
                                             path=paths['standards_dir'],
                                             issample=False,
                                             fields=self._sample_data.fields)

    def _interpolate_standard(self):
        self._standard_data.interpolate_standard(
            banks=self._sample_data.banks,
            scriptname=self._sample_data.scriptname,
            parent=self)

    def _set_loop(self):
        if len(self._sample_data.keys()) == 1:
            self._loop = "for workspace in wss_sample" \
                         f"['{list(self._sample_data.keys())[0]}']:"
            self._spac = "\n" + " " * 4
        else:
            self._loop = "for sample, workspacelist in wss_sample.items(): " \
                         "\n    for workspace in workspacelist:"
            self._spac = "\n" + " " * 8

    @staticmethod
    def _get_header_lines():
        lines = [
            'from mantidqtinterfaces.dns_sc_elastic.scripts.'
            'md_sc_elastic import load_all',
            'from mantidqtinterfaces.dns_sc_elastic.scripts.'
            'md_sc_elastic import '
            'vanadium_correction, fliping_ratio_correction',
            'from mantidqtinterfaces.dns_sc_elastic.scripts.'
            'md_sc_elastic import background_substraction',
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

        return ["", f"params = {{'a' : {options['a']}, "
                    f"\n          'b' : {options['b']},"
                    f"\n          'c' : {options['c']},"
                    f"\n          'alpha' : {options['alpha']},"
                    f"\n          'beta'  : {options['beta']},"
                    f"\n          'gamma' : {options['gamma']},"
                    f"\n          'hkl1'  : '{options['hkl1']}',"
                    f"\n          'hkl2'  : '{options['hkl2']}',"
                    f"\n          'omega_offset' : {options['omega_offset']},"
                    f"\n          'norm_to' : '{self._norm}',"
                    f"\n          'dx' : '{options['dx']:7.4f}',"
                    f"\n          'dy' : '{options['dx']:7.4f}',"
                    "}", ""]

    def _get_binning_lines(self):
        lines = [
            f"binning = {{'twoTheta' : ["
            f"{self._sample_data.ttheta.bin_edge_min:.3f},"
            f" {self._sample_data.ttheta.bin_edge_max:.3f},"
            f" {self._sample_data.ttheta.nbins:d}],\n"
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
        if self._vanac or self._nicrc:
            lines = [
                "# substract background from vanadium and nicr",
                "for sample, workspacelist in wss_standard.items(): "
                "\n    for workspace in workspacelist:"
                "\n        background_substraction(workspace)", ""
            ]
        return lines

    def _return_sample_bg_string(self):
        return f"{self._spac}background_substraction(workspace, " \
               f"factor={self._backfac})"

    def _return_sample_vanac_strinf(self):
        return f"{self._spac}vanadium_correction(workspace, " \
               " vanaset=standard_data['vana'], " \
               f"ignore_vana_fields={self._ign_vana}, " \
               f"sum_vana_sf_nsf={self._sum_sfnsf})"

    def _get_vanac_lines(self):
        backgroundstring = self._return_sample_bg_string()
        vanacstring = self._return_sample_vanac_strinf()
        lines = []
        if self._sampb or self._vanac:
            lines = ["# correct sample data",
                     f"{self._loop}{backgroundstring}{vanacstring}"]
        return lines

    def _get_nicrc_lines(self):
        lines = []
        if self._nicrc:
            lines = [f"{self._loop}{self._spac}"
                     "fliping_ratio_correction(workspace)"]
        return lines

    def get_plotlist(self):
        for plot in self._plotlist:
            self._data_arrays[plot] = {
                'ttheta': self._sample_data.ttheta.range,
                'omega': self._sample_data.omega.range,
                'intensity': mtd[plot].getSignalArray(),
                'error': np.sqrt(mtd[plot].getErrorSquaredArray())
            }
        return self._plotlist, self._data_arrays
