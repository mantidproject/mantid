import stresstesting
from algorithm_decorator import make_decorator
from mantid.simpleapi import *
import mantid.api

class RROAutoFunctionalityTest(stresstesting.MantidStressTest):
    """
    This test is to check the functionality of ReflectometryReductionOneAuto. Data testing is done separately
    """
    

    def __init__(self):
        super(RROAutoFunctionalityTest, self).__init__()
        data_ws = Load('INTER00013460.nxs')
        self.__data_ws = data_ws
        trans_ws_1 = Load('INTER00013463.nxs')
        self.__trans_ws_1 = trans_ws_1
        trans_ws_2 = Load('INTER00013464.nxs')
        self.__trans_ws_2 = trans_ws_2
        line_detector_ws = Load('POLREF00004699.nxs')
        self.__line_detector_ws = line_detector_ws
    
    def __del__(self):
        DeleteWorkspace(self.__data_ws)
        DeleteWorkspace(self.__trans_ws_1)
        DeleteWorkspace(self.__trans_ws_2)  
        DeleteWorkspace(self.__self.__line_detector_ws)
    
      
    def construct_standard_algorithm(self):
        alg = make_decorator(ReflectometryReductionOneAuto)
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(1.0)
        alg.set_I0MonitorIndex(0)
        alg.set_ProcessingInstructions("0, 1")
        alg.set_MonitorBackgroundWavelengthMin(0.0)
        alg.set_MonitorBackgroundWavelengthMax(1.0)
        alg.set_MonitorIntegrationWavelengthMin(0.0)
        alg.set_MonitorIntegrationWavelengthMax(1.0)
        alg.set_additional({'OutputWorkspaceWavelength': 'out_ws_wav'})
        return alg
           
    def test_point_detector_run_with_single_transmission_workspace(self):
        alg = self.construct_standard_algorithm()
        alg.set_InputWorkspace(self.__data_ws)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(self.__trans_ws_1) 
        alg.set_ThetaIn(0.2)
        
        out_ws_q, out_ws_lam, theta = alg.execute()
        self.assertEqual(0.2, theta, "Theta in and out should be the same")
        
        self.assertTrue(isinstance(out_ws_lam, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", out_ws_lam.getAxis(0).getUnit().unitID())
        
        self.assertTrue(isinstance(out_ws_q, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("MomentumTransfer", out_ws_q.getAxis(0).getUnit().unitID())
        
        self.assertEqual(2, out_ws_lam.getNumberHistograms())
    
    def test_point_detector_run_with_two_transmission_workspaces(self):
        alg = self.construct_standard_algorithm()
        
        alg.set_InputWorkspace(self.__data_ws)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(self.__trans_ws_1) 
        alg.set_SecondTransmissionRun(self.__trans_ws_2)
        alg.set_ThetaIn(0.2)
        
        out_ws_q, out_ws_lam, theta = alg.execute() 
        
    
    def test_spectrum_map_mismatch_throws_when_strict(self):
        alg = self.construct_standard_algorithm()
        '''
        Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
        and strict checking is turned on, so this will throw upon execution.
        '''
        trans_run1_lam = ConvertUnits(self.__trans_ws_1, Target='Wavelength')
        trans_run1_lam = CropWorkspace(trans_run1_lam, EndWorkspaceIndex=1)
        
        alg.set_InputWorkspace(self.__data_ws)
        alg.set_ProcessingInstructions("3,4") # This will make spectrum numbers in input workspace different from denominator
        alg.set_FirstTransmissionRun(trans_run1_lam) 
        alg.set_StrictSpectrumChecking(True)
        
        self.assertRaises(Exception, alg.execute) # Should throw due to spectrum missmatch.
        
        
    def test_spectrum_map_mismatch_doesnt_throw_when_not_strict(self):
        alg = self.construct_standard_algorithm()

        '''
        Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
        and strict checking is turned off, so this will NOT throw upon execution.
        '''
        trans_run1_lam = ConvertUnits(self.__trans_ws_1, Target='Wavelength')
        trans_run1_lam = CropWorkspace(trans_run1_lam, EndWorkspaceIndex=1)
        
        alg.set_InputWorkspace(self.__data_ws)
        alg.set_ProcessingInstructions("3,4") # This will make spectrum numbers in input workspace different from denominator
        alg.set_FirstTransmissionRun(trans_run1_lam) 
        alg.set_StrictSpectrumChecking(False) # Will not crash-out on spectrum checking.
        
        alg.execute()# Should not throw
       
        
    def test_multidetector_run(self):
        alg = self.construct_standard_algorithm()
        
        alg.set_InputWorkspace(self.__line_detector_ws[0])
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_DetectorComponentName('lineardetector')
        alg.set_ProcessingInstructions("10") # Fictional values
        alg.set_CorrectDetectorPositions(False)
        alg.set_RegionOfDirectBeam("20, 30") # Fictional values
        alg.set_ThetaIn(0.1) # Fictional values
        
        out_ws_q, out_ws_lam, theta =  alg.execute()
        
        self.assertTrue(isinstance(out_ws_lam, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", out_ws_lam.getAxis(0).getUnit().unitID())
        
        self.assertTrue(isinstance(out_ws_q, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("MomentumTransfer", out_ws_q.getAxis(0).getUnit().unitID())
        
    def test_multidetector_run_correct_positions(self):
        alg = self.construct_standard_algorithm()
        
        alg.set_InputWorkspace(self.__line_detector_ws[0])
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_DetectorComponentName('lineardetector')
        alg.set_ProcessingInstructions("73") # Fictional values
        alg.set_CorrectDetectorPositions(True)
        alg.set_RegionOfDirectBeam("28, 29") # Fictional values
        alg.set_ThetaIn(0.49 / 2) # Fictional values
        
        out_ws_q, out_ws_lam, theta =  alg.execute()
        
        self.assertTrue(isinstance(out_ws_lam, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", out_ws_lam.getAxis(0).getUnit().unitID())
        
        self.assertTrue(isinstance(out_ws_q, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("MomentumTransfer", out_ws_q.getAxis(0).getUnit().unitID())
        
        instrument = out_ws_lam.getInstrument()
        detector_pos = instrument.getComponentByName("lineardetector").getPos()
        
        self.assertDelta(-0.05714, detector_pos.Z(), 0.0001)
    
    
    def runTest(self):
    
        self.test_point_detector_run_with_single_transmission_workspace()
        
        self.test_point_detector_run_with_two_transmission_workspaces()
        
        self.test_spectrum_map_mismatch_throws_when_strict()
        
        self.test_spectrum_map_mismatch_doesnt_throw_when_not_strict()
        
        self.test_multidetector_run()
        
        self.test_multidetector_run_correct_positions()

        
            
    def validate(self):
        return True
