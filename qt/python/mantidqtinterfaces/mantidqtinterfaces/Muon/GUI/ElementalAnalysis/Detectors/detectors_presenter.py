# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class DetectorsPresenter(object):
    def __init__(self, view):
        self.view = view
        self.detectors = []
        for name in self.view.widgets.keys():
            self.detectors.append(self.view.widgets[name])

    def setStateQuietly(self, name, state):
        self.view.setStateQuietly(name, state)

    def enableDetector(self, name):
        self.view.enableDetector(name)

    def disableDetector(self, name):
        self.view.disableDetector(name)

    def getNames(self):
        return self.view.widgets.keys()
