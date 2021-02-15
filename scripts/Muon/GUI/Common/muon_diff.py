# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111

from Muon.GUI.Common.muon_base import MuonBase


class MuonDiff(MuonBase):
    def __init__(self,diff_name,positive,negative, group_or_pair="group", periods =[1]):
        super(MuonDiff, self).__init__(diff_name, periods)
        self._positive = positive
        self._negative = negative
        self._group_or_pair = group_or_pair

    @property
    def forward_group(self):
        return self._positive

    @property
    def backward_group(self):
        return self._negative

    @property
    def group_or_pair(self):
        return self._group_or_pair