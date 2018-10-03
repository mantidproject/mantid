# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
class DetectorsPresenter(object):
    def __init__(self, view):
        self.view = view
        self.detectors = [
            self.view.GE1,
            self.view.GE2,
            self.view.GE3,
            self.view.GE4]
