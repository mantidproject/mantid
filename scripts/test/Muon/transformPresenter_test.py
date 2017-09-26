import sys

from  Muon import FFT_presenter
from  Muon import load_utils
from  Muon import transform_presenter
from  Muon import transform_view
from  Muon import transform_selection_view
from  Muon import MaxEnt_presenter
from  Muon import model_constructor

import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class FFTTransformTest(unittest.TestCase):
    def setUp(self):
        load_utils.LoadUtils=mock.Mock()
        FFT_presenter.FFTPresenter=mock.Mock()
        MaxEnt_presenter.MaxEntPresenter=mock.Mock()
        self.view=mock.create_autospec(transform_view.TransformView,spec_set=False)
        self.view.getView=mock.Mock()
        self.view.getMethods=mock.Mock(return_value=["FFT","MaxEnt"])
        self.view.hideAll=mock.Mock()
        self.view.show=mock.Mock()
        self.view.selection=mock.create_autospec(transform_selection_view.TransformSelectionView,spec_set=True)
        self.view.selection.changeMethodSignal=mock.Mock()
        self.model=mock.create_autospec(model_constructor.ModelConstructor)
        self.model.getModel=mock.Mock()
 
        #set presenter
        self.presenter=transform_presenter.TransformPresenter(self.view,self.model)

    def test_changeDisplay(self):
        self.presenter.updateDisplay(1)
        assert(self.view.hideAll.call_count==1)
        assert(self.view.show.call_count==1)


if __name__ == '__main__':
    unittest.main()
