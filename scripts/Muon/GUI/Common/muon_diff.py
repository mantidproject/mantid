# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from Muon.GUI.Common.muon_base import MuonBase


class MuonDiff(MuonBase):
    """
    Simple structure to store information on a difference.
    - The name is set at initialization and after that cannot be changed.
    - The difference has two properies positive and negative used in the calculation
    - The difference can be between two groups or two pairs only
    - The workspace associated to the difference can be set, but must be of type MuonWorkspaceWrapper.
    """
    def __init__(self, diff_name, positive, negative, group_or_pair="group", periods=[1]):
        super(MuonDiff, self).__init__(diff_name, periods)
        self._positive = positive
        self._negative = negative
        self._group_or_pair = group_or_pair

    @property
    def positive(self):
        return self._positive

    @property
    def negative(self):
        return self._negative

    @property
    def group_or_pair(self):
        return self._group_or_pair
