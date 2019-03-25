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
    def __init__(self):
        super(MuonGuiContext, self).__init__()
        self.notifier = GuiVariablesNotifier(self)

    def __setitem__(self, key, value):
        super(MuonGuiContext, self).__setitem__(key, value)
        self.notifier.notify_subscribers(**{key: value})

    def add_subscriber(self, observer):
        self.notifier.add_subscriber(observer)
