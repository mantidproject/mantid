from __future__ import (absolute_import, division, print_function)

from Muon import fft_presenter
from Muon import load_utils
from Muon import maxent_presenter
from Muon import transform_selection_presenter


class TransformPresenter(object):
    """
    The widget for controlling which method to display
    in the transformation tab
    """
    def __init__(self,view,model):
        self.view=view
        self.load=load_utils.LoadUtils()
        if not self.load.MuonAnalysisExists():
            return

        # create presenters for the views
        self.FFTPresenter=fft_presenter.FFTPresenter(self.view.getView("FFT"),model.getModel("FFT"),self.load)
        self.MaxEntPresenter=maxent_presenter.MaxEntPresenter(self.view.getView("MaxEnt"),model.getModel("MaxEnt"),self.load)
        # get the transform selection view
        self.selectionPresenter=transform_selection_presenter.TransformSelectionPresenter(self.view.selection)
        # gets a list of the views/methods
        self.methodsList=self.view.getMethods()
        # sets the transform selection view to have the correct options
        self.selectionPresenter.setMethodsCombo(self.methodsList)
        # connect
        self.view.selection.changeMethodSignal.connect(self.updateDisplay)

    def close(self):
        self.MaxEntPresenter.cancel()

    #switch the view
    def updateDisplay(self,index):
        self.view.hideAll()
        self.view.show(self.methodsList[index])
