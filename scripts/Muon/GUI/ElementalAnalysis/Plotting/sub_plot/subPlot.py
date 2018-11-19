from __future__ import (absolute_import, division, print_function)
from six import iteritems
from Muon.GUI.ElementalAnalysis.Plotting.sub_plot.subPlot_context import subPlotContext

# use this to interact with plot

class subPlot(object):

    def __init__(self, name):
        self.context = subPlotContext(name)

    def test(self):
        