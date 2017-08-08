from PyQt4 import QtGui
import mock
from Muon import FFTPresenter #test
from Muon import FFTView #test
import unittest
import sys

class FFTPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view=mock.create_autospec(FFTView.FFTView,spec_set=True) 
        self.view.table_click_signal=mock.Mock(return_value=[1,3])
        self.view.button_signal=mock.Mock()
        self.view.changed=mock.MagicMock()
        self.view.changedHideUnTick=mock.MagicMock()
        self.view.getImBoxRow=mock.Mock(return_value=3)
        self.view.getShiftBoxRow=mock.Mock(return_value=5)
        self.view.init_FFT_input=mock.Mock(return_value={InputWorkspace="test";OutputWorkspace="muon"}
        self.view.init_FFT_input=mock.Mock(return_value={InputWorkspace="test";OutputWorkspace="muon"}
        self.presenter=FFTPresenter.FFTPresenter(self.view)
          
    def test_ImBox(self):
        def sendSignal():
              row,col=self.view.table_click_signal() 
              self.presenter.Clicked(row,col)
        sendSignal() 
        self.view.changedHideUnTick.called_once()
        self.view.changed.assert_not_called()
        
    def test_shiftBox(self):
        def sendSignal():
              row,col=self.view.table_click_signal() 
              self.presenter.Clicked(row,col)
        self.view.table_click_signal=mock.Mock(return_value=[1,5])
        sendSignal() 
        self.view.changed.called_once()
        self.view.changedHideUnTick.assert_not_called()

             
    def test_buttonNotRaw(self):

if __name__ == '__main__':
    unittest.main()
