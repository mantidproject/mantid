from __future__ import (absolute_import, division, print_function)

from Muon import FFT_presenter
from Muon import load_utils
from Muon import MaxEnt_presenter
from Muon import transform_selection_presenter


class TransformPresenter(object):

    def __init__(self,view,model):
        self.view=view
        self.load=load_utils.LoadUtils()
        if self.load.MuonAnalysisExists() == False:
            return
        self.FFTPresenter=FFT_presenter.FFTPresenter(self.view.getView("FFT"))
        self.MaxEntPresenter=MaxEnt_presenter.MaxEntPresenter(self.view.getView("MaxEnt"),model.getModel("MaxEnt"),self.load)
        self.selectionPresenter=transform_selection_presenter.TransformSelectionPresenter(self.view.selection)
        self.methodsList=self.view.getMethods()
        self.selectionPresenter.setMethodsCombo(self.methodsList)

        self.view.selection.changeMethodSignal.connect(self.updateDisplay)

    def updateDisplay(self,index):
        self.view.hideAll()
        self.view.show(self.methodsList[index])
