# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic options tab model of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_elastic.helpers.converters import convert_hkl_string_to_float
from mantidqtinterfaces.dns_single_crystal_elastic.helpers.converters import d_spacing_from_lattice
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import DNSCommonOptionsModel


class DNSElasticSCOptionsModel(DNSCommonOptionsModel):
    @staticmethod
    def get_dx_dy(options):
        hkl1 = convert_hkl_string_to_float(options["hkl1"])
        hkl2 = convert_hkl_string_to_float(options["hkl2"])
        dx = d_spacing_from_lattice(options["a"], options["b"], options["c"], options["alpha"], options["beta"], options["gamma"], hkl1)
        dy = d_spacing_from_lattice(options["a"], options["b"], options["c"], options["alpha"], options["beta"], options["gamma"], hkl2)
        return [dx, dy]
