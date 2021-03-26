# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from Muon.GUI.Common.muon_base_pair import MuonBasePair


class MuonPhasequad(object):
    """
    Simple structure to store information on a phasequad.

    - The name is set at initialization and after that cannot be changed.
    - The phasquad has two pairs associated to it and are stored as MuonBasePairs.
    - The workspaces associated with the pairs can be set, but must be of type MuonWorkspaceWrapper.
    """
    def __init__(self, phasequad_name, phase_table):
        self._phasequad_name = phasequad_name
        self._phase_table = phase_table
        self._Re = MuonBasePair(phasequad_name + "_Re_")
        self._Im = MuonBasePair(phasequad_name + "_Im_")

    @property
    def Re(self):
        return self._Re

    @property
    def Im(self):
        return self._Im

    @property
    def name(self):
        return self._phasequad_name

    @property
    def phase_table(self):
        return self._phase_table

    @phase_table.setter
    def phase_table(self, new_table):
        self._phase_table = new_table

    def update_asymmetry_workspaces(self, ws_list, run, rebin=False):
        self._Re.update_asymmetry_workspace(ws_list[0], run, rebin=rebin)
        self._Im.update_asymmetry_workspace(ws_list[1], run, rebin=rebin)
