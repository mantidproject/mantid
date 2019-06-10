# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)



class PeaksPresenter(object):
    def __init__(self, view):
        self.view = view
        self.major = self.view.major
        self.minor = self.view.minor
        self.gamma = self.view.gamma
        self.electron = self.view.electron
        self.peak_checkboxes = self.view.peak_checkboxes
