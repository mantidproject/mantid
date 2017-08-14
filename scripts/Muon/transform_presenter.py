from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid

from Muon import FFT_presenter
from Muon import MaxEnt_presenter
from Muon import transform_selection_view


class transformPresenter(object):

    def __init__(self,view):
        self.view=view
        self.FFTPresenter=FFT_presenter.FFTPresenter(self.view.FFTView)
        
 
