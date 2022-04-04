# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS tof powder plot model
"""
from mantid.simpleapi import mtd
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel


class DNSTofPowderPlotModel(DNSObsModel):

    @staticmethod
    def get_plot_workspace():
        try:
            if mtd['data1_sqw'].id() == 'WorkspaceGroup':
                return mtd['data1_sqw'].getItem(0)
            return mtd['data1_sqw']
        except KeyError:
            return False
