# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


from Muon.GUI.Common.dummy.dummy_view import DummyView
from Muon.GUI.Common.dummy.dummy_presenter import DummyPresenter


class DummyWidget(object):
    """
    """
    def __init__(self,name,parent=None):
        view=DummyView(name,parent)
        model=None
        self._presenter = DummyPresenter(view,model)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def setButtonConnection(self,slot):
        view=self._presenter.widget
        view.buttonSignal.connect(slot)
