# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.observer_pattern import Observable


class GuiVariablesNotifier(Observable):
    def __init__(self, outer):
        Observable.__init__(self)
        self.outer = outer  # handle to containing class

    def notify_subscribers(self, *args, **kwargs):
        Observable.notify_subscribers(self, *args, **kwargs)

class MuonGuiContext(dict):
    def __init__(self, *args, **kwargs):
        super(MuonGuiContext, self).__init__(*args, **kwargs)
        self.gui_variables_notifier = GuiVariablesNotifier(self)

    def update_and_send_signal(self, *args, **kwargs):
        super(MuonGuiContext, self).update(*args, **kwargs)
        self.gui_variables_notifier.notify_subscribers()

    def add_subscriber(self, observer):
        self.gui_variables_notifier.add_subscriber(observer)

    def period_string(self, run=None):
        summed_periods = self["SummedPeriods"] if 'SummedPeriods' in self else [1]
        subtracted_periods = self["SubtractedPeriods"] if 'SubtractedPeriods' in self else []
        if subtracted_periods:
            return '+'.join([str(period) for period in summed_periods]) + '-' + '-'.join([str(period) for period in subtracted_periods])
        else:
            return '+'.join([str(period) for period in summed_periods])
