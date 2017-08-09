from PyQt4 import QtGui
import mock
from Muon import FFTPresenter #test
from Muon import FFTView #test
import unittest
import sys

class FFTPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view=mock.create_autospec(FFTView.FFTView,spec_set=True) 
        #signals
        self.view.tableClickSignal=mock.Mock(return_value=[1,3])
        self.view.buttonSignal=mock.Mock()
        # functions  
        self.view.changed=mock.MagicMock()
        self.view.changedHideUnTick=mock.MagicMock()
        self.view.initFFTInput=mock.Mock(return_value={"InputWorkspace":"test","OutputWorkspace":"muon"})
        self.view.addFFTComplex=mock.Mock(return_value={"InputImWorkspace":"MuonFFT"})
        self.view.addFFTShift=mock.Mock()
        self.view.addRaw=mock.Mock()
 
        #get methods  
        self.view.getImBoxRow=mock.Mock(return_value=3)
        self.view.getShiftBoxRow=mock.Mock(return_value=5)
        self.view.isRaw=mock.Mock(return_value=True)
        self.view.isComplex=mock.Mock(return_value=True)
        self.view.isAutoShift=mock.Mock(return_value=True)
      

         #set presenter
        self.presenter=FFTPresenter.FFTPresenter(self.view)
    def sendSignal(self):
        row,col=self.view.tableClickSignal() 
        self.presenter.tableClicked(row,col)
         
    def test_ImBox(self):
        self.sendSignal() 
        self.view.changedHideUnTick.called_once()
        self.view.changed.assert_not_called()
        
    def test_shiftBox(self):
        self.view.tableClickSignal=mock.Mock(return_value=[1,5])
        self.sendSignal() 
        self.view.changed.called_once()
        self.view.changedHideUnTick.assert_not_called()

             
    def test_buttonNotRawAndNoIm(self):
       self.view.isRaw=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.not_called()       
       self.view.addFFTShift.assert_not_called() 
       self.view.addRaw.assert_not_called()      

    def test_buttonNotRawAndIm(self):
       self.view.isRaw=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.called_once()       
       self.view.addFFTShift.assert_not_called() 
       self.view.addRaw.assert_not_called()      

    def test_buttonRawAndIm(self):
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.called_once()       
       self.view.addFFTShift.assert_not_called() 
       self.view.addRaw.called_twice()      

    def test_buttonRawAndNoIm(self):
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.not_called()       
       self.view.addFFTShift.assert_not_called() 
       self.view.addRaw.called_twice()      


             
    def test_buttonNoShiftNotRawAndNoIm(self):
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.not_called()       
       self.view.addFFTShift.called_once() 
       self.view.addRaw.assert_not_called()      

    def test_buttonNoShiftNotRawAndIm(self):
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isRaw=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.called_once()       
       self.view.addFFTShift.called_once() 
       self.view.addRaw.assert_not_called()      

    def test_buttonNoShiftRawAndIm(self):
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.called_once()       
       self.view.addFFTShift.called_once() 
       self.view.addRaw.called_twice()      

    def test_buttonNoShiftRawAndNoIm(self):
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.buttonSignal()
       self.view.initFFTInput.called_once()       
       self.view.addFFTComplex.not_called()       
       self.view.addFFTShift.called_once() 
       self.view.addRaw.called_twice()      


if __name__ == '__main__':
    unittest.main()
