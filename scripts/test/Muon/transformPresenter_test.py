import mantid.simpleapi as mantid
import sys

from  Muon import transform_presenter 
from  Muon import transform_view 
from  Muon import transform_selection_presenter 
from  Muon import transform_selection_view 
import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class FFTTransformTest(unittest.TestCase):
    def setUp(self):
        self.view=mock.create_autospec(transform_view.transformView,spec_set=False) 
        self.view.getView=mock.Mock()
        self.view.getMethods=mock.Mock(return_value=["FFT","MaxEnt"]) 
        self.view.hideAll=mock.Mock() 
        self.view.show=mock.Mock() 
          
        self.view.selection=mock.create_autospec(transform_selection_view.TransformSelectionView,spec_set=True)
        self.view.selection.changeMethodSignal=mock.Mock()
        #set presenter
        self.presenter=transform_presenter.transformPresenter(self.view)
 
    def test_changeDisplay(self):
       self.presenter.updateDisplay(1)
       assert(self.view.hideAll.call_count==1)
       assert(self.view.show.call_count==1)


if __name__ == '__main__':
    unittest.main()
