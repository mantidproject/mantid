# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.Common.contexts.plotting_context import PlottingContext


class PlotPanesContext(object):
    def __init__(self):
        self._plotting_context = {}

    def add_pane(self, name):
        self._plotting_context[name] = PlottingContext()

    def __getitem__(self, name):
        return self._plotting_context[name]
