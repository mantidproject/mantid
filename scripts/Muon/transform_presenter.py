from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid

from Muon import FFT_presenter
from Muon import MaxEnt_presenter
from Muon import transform_selection_presenter


class transformPresenter(object):

    def __init__(self,view):
        self.view=view
        self.FFTPresenter=FFT_presenter.FFTPresenter(self.view.getView("FFT"))
        self.MaxEntPresenter=MaxEnt_presenter.MaxEntPresenter(self.view.getView("MaxEnt"))
        self.selectionPresenter=transform_selection_presenter.transformSelectionPresenter(self.view.selection)
        self.methodsList=self.view.getMethods()
        self.selectionPresenter.setMethodsCombo(self.methodsList)

        self.view.selection.changeMethodSignal.connect(self.updateDisplay)

    def updateDisplay(self,index):
        self.view.hideAll()
        self.view.show(self.methodsList[index])
             
 
