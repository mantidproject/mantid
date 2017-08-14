import mantid.simpleapi as mantid
import sys
from  Muon import FFT_presenter 
from  Muon import FFT_view 
import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class FFTPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view=mock.create_autospec(FFT_view.FFTView,spec_set=True) 
        #signals
        self.view.tableClickSignal=mock.Mock(return_value=[3,1])
        #needed for connect in presenter
        self.view.buttonSignal=mock.Mock()
        # functions  
        self.view.changed=mock.MagicMock()
        self.view.changedHideUnTick=mock.MagicMock()
        self.view.initFFTInput=mock.Mock(return_value={"InputWorkspace":"testWS","OutputWorkspace":"muon"})
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
        self.presenter=FFT_presenter.FFTPresenter(self.view)
    def sendSignal(self):
        row,col=self.view.tableClickSignal() 
        self.presenter.tableClicked(row,col)
         
    def test_ImBox(self):
        self.sendSignal() 
        self.view.tableClickSignal=mock.Mock(return_value=[3,1])
        assert(self.view.changedHideUnTick.call_count==1)
        assert(self.view.changed.call_count == 0)
        
    def test_shiftBox(self):
        self.view.tableClickSignal=mock.Mock(return_value=[5,1])
        self.sendSignal() 
        assert(self.view.changed.call_count==1)
        assert(self.view.changedHideUnTick.call_count==0)

             
    def test_buttonNotRawAndNoIm(self):
       self.view.isAutoShift=mock.Mock(return_value=True)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.isRaw=mock.Mock(return_value=False)
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==0)
       assert(self.view.addFFTShift.call_count==0)
       assert(self.view.addRaw.call_count==0)

    def test_buttonNotRawAndIm(self):
       self.view.isAutoShift=mock.Mock(return_value=True)
       self.view.isComplex=mock.Mock(return_value=True)
       self.view.isRaw=mock.Mock(return_value=False)
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==1)
       assert(self.view.addFFTShift.call_count==0)
       assert(self.view.addRaw.call_count==0)

    def test_buttonRawAndIm(self):
       self.view.isAutoShift=mock.Mock(return_value=True)
       self.view.isComplex=mock.Mock(return_value=True)
       self.view.isRaw=mock.Mock(return_value=True)
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==1)
       assert(self.view.addFFTShift.call_count==0)
       assert(self.view.addRaw.call_count==2)

    def test_buttonRawAndNoIm(self):
       self.view.isAutoShift=mock.Mock(return_value=True)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.isRaw=mock.Mock(return_value=True)
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==0)
       assert(self.view.addFFTShift.call_count==0)
       assert(self.view.addRaw.call_count==1)


             
    def test_buttonNoShiftNotRawAndNoIm(self):
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.isRaw=mock.Mock(return_value=False)
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==0)
       assert(self.view.addFFTShift.call_count==1)
       assert(self.view.addRaw.call_count==0)

    def test_buttonNoShiftNotRawAndIm(self):
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=True)
       self.view.isRaw=mock.Mock(return_value=False)
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==1)
       assert(self.view.addFFTShift.call_count==1)
       assert(self.view.addRaw.call_count==0)

    def test_buttonNoShiftRawAndIm(self):
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=True)
       self.view.isRaw=mock.Mock(return_value=True)
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==1)
       assert(self.view.addFFTShift.call_count==1)
       assert(self.view.addRaw.call_count==2)

    def test_buttonNoShiftRawAndNoIm(self):
       self.view.isAutoShift=mock.Mock(return_value=False)
       self.view.isComplex=mock.Mock(return_value=False)
       self.view.isRaw=mock.Mock(return_value=True)
       testWS=mantid.CreateWorkspace([0,1],[2,3])
       self.presenter.handleButton()
       assert(self.view.initFFTInput.call_count==1)
       assert(self.view.addFFTComplex.call_count==0)
       assert(self.view.addFFTShift.call_count==1)
       assert(self.view.addRaw.call_count==1)


if __name__ == '__main__':
    unittest.main()
