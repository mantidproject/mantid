# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from mantidqtinterfaces.Muon.GUI.Common.muon_base import MuonBase


class MuonBasePair(MuonBase):
    def __init__(self, pair_name, periods=[1]):
        super(MuonBasePair, self).__init__(pair_name, periods)

    def get_asymmetry_workspace_for_run(self, run, rebin):
        # return the first element as it will always be a single value list
        try:
            if rebin:
                return self.get_asymmetry_workspace_names_rebinned([run])[0]
            else:
                return self.get_asymmetry_workspace_names([run])[0]
        except IndexError:
            raise KeyError("Workspace does not exist")
