# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
class LineSelectorPresenter(object):
    def __init__(self, view):
        self.view = view
        self.total = self.view.total
        self.prompt = self.view.prompt
        self.delayed = self.view.delayed
        self.line_checkboxes = self.view.line_checkboxes
