# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from Muon.GUI.Common.muon_base_pair import MuonBasePair


class MuonPair(MuonBasePair):
    """
    Simple structure to store information on a detector group pair.

    - The name is set at initialization and after that cannot be changed.
    - The pair has two groups associated to it, and we store only their names.
    - The balance parameter is stored and modifiable.
    - The workspace associated to the pair can be set, but must be of type MuonWorkspaceWrapper.
    """
    def __init__(self, pair_name, forward_group_name="", backward_group_name="", alpha=1.0, periods=[1]):
        super().__init__(pair_name, periods)
        self._forward_group_name = forward_group_name
        self._backward_group_name = backward_group_name
        self._alpha = float(alpha)

    @property
    def forward_group(self):
        return self._forward_group_name

    @property
    def backward_group(self):
        return self._backward_group_name

    @forward_group.setter
    def forward_group(self, new_name):
        self._forward_group_name = new_name

    @backward_group.setter
    def backward_group(self, new_name):
        self._backward_group_name = new_name

    @property
    def alpha(self):
        return float("{}".format(self._alpha))

    @alpha.setter
    def alpha(self, new_alpha):
        if float(new_alpha) >= 0.0:
            self._alpha = float(new_alpha)
        else:
            raise AttributeError("Alpha must be > 0.0.")
