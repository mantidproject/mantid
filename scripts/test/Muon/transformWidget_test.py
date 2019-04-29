# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter
from Muon.GUI.FrequencyDomainAnalysis.Transform import transform_widget
from Muon.GUI.FrequencyDomainAnalysis.Transform import transform_view
from Muon.GUI.FrequencyDomainAnalysis.TransformSelection import transform_selection_view
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget

class TransformTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.load=  mock.create_autospec( load_utils.LoadUtils,spec_set=True)
        self.fft=   mock.create_autospec( fft_presenter.FFTPresenter,spec_Set=True)
        self.maxent=mock.create_autospec( maxent_presenter.MaxEntPresenter,spec_set=True)

        # create widget
        self.widget=transform_widget.TransformWidget(self.load, FFTWidget, MaxEntWidget)
        # create the view
        self.view=mock.create_autospec(transform_view.TransformView,spec_set=False)
        self.view.getView=mock.Mock()
        self.view.getMethods=mock.Mock(return_value=["FFT","MaxEnt"])
        self.view.hideAll=mock.Mock()
        self.view.showMethod=mock.Mock()
        self.view.selection=mock.create_autospec(transform_selection_view.TransformSelectionView,spec_set=True)
        self.view.selection.changeMethodSignal=mock.Mock()
        # set the mocked view to the widget
        self.widget.mockWidget(self.view)  

    def test_changeDisplay(self):
        self.widget.updateDisplay(1)
        assert(self.view.hideAll.call_count==1)
        self.assertEquals(self.view.showMethod.call_count,1)


if __name__ == '__main__':
    unittest.main()
