import sys

from  Muon.GUI.Common import mock_widget
from  Muon.GUI.Common import load_utils
from  Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter
from  Muon.GUI.FrequencyDomainAnalysis.Transform import transform_widget
from  Muon.GUI.FrequencyDomainAnalysis.Transform import transform_view
from  Muon.GUI.FrequencyDomainAnalysis.TransformSelection import transform_selection_view
from  Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter


# need to update this
import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

class FFTTransformTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.load=  mock.create_autospec( load_utils.LoadUtils,spec_set=True)
        self.fft=   mock.create_autospec( fft_presenter.FFTPresenter,spec_Set=True)
        self.maxent=mock.create_autospec( maxent_presenter.MaxEntPresenter,spec_set=True)

        # create widget
        self.widget=transform_widget.TransformWidget(self.load)
        # create the view
        self.view=mock.create_autospec(transform_view.TransformView,spec_set=False)
        self.view.getView=mock.Mock()
        self.view.getMethods=mock.Mock(return_value=["FFT","MaxEnt"])
        self.view.hideAll=mock.Mock()
        self.view.show=mock.Mock()
        self.view.selection=mock.create_autospec(transform_selection_view.TransformSelectionView,spec_set=True)
        self.view.selection.changeMethodSignal=mock.Mock()
        # set the mocked view to the widget
        self.widget.mockWidget(self.view)  

    def test_changeDisplay(self):
        self.widget.updateDisplay(1)
        assert(self.view.hideAll.call_count==1)
        assert(self.view.show.call_count==1)


if __name__ == '__main__':
    unittest.main()
