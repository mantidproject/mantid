import unittest
from mantid.simpleapi import *
from testhelpers.algorithm_decorator import make_decorator
import mantid.api
import math
import abc

class ReflectometryReductionOneBaseTest(object):
    
    __metaclass__  = abc.ABCMeta
    
    @abc.abstractmethod
    def algorithm_type(self):
        raise RuntimeError("Not implemented")
    
    def construct_standard_algorithm(self):
        alg = make_decorator(self.algorithm_type())
        alg.set_InputWorkspace(self.__tof)
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
    
    def setUp(self):
        tof = CreateWorkspace(UnitX="TOF", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        self.__tof = tof
        self.__not_tof = not_tof
        
    def tearDown(self):
        DeleteWorkspace(self.__tof)
        DeleteWorkspace(self.__not_tof)
        
    def test_check_input_workpace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_InputWorkspace(self.__not_tof)
        self.assertRaises(Exception, alg.execute)
    
    def test_check_first_transmission_workspace_not_tof_or_wavelength_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__not_tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_check_second_transmission_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__not_tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_proivde_second_transmission_run_without_first_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_second_transmission_run_without_params_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__tof)
        alg.set_SecondTransmissionRun(self.__tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_second_transmission_run_without_start_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__tof)
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_EndOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_end_transmission_run_without_end_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__tof)
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_end_overlap_q_must_be_greater_than_start_overlap_q_or_throw(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__tof)
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlap( 0.6 )
        alg.set_EndOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_must_provide_wavelengths(self):
        self.assertRaises(RuntimeError, self.algorithm_type(), InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMin=1.0)
        self.assertRaises(RuntimeError, self.algorithm_type(), InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMax=1.0)
        
    def test_wavelength_min_greater_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_WavelengthMin(1.0)
        alg.set_WavelengthMax(0.0)
        self.assertRaises(Exception, alg.execute)
        
    def test_monitor_background_wavelength_min_greater_monitor_background_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_MonitorBackgroundWavelengthMin(1.0)
        alg.set_MonitorBackgroundWavelengthMax(0.0)
        self.assertRaises(Exception, alg.execute)
    
    def test_monitor_integration_wavelength_min_greater_monitor_integration_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_MonitorIntegrationWavelengthMin(1.0)
        alg.set_MonitorIntegrationWavelengthMax(0.0)
        self.assertRaises(Exception, alg.execute)
        
    def test_monitor_index_positive(self):
        alg = self.construct_standard_algorithm()
        alg.set_I0MonitorIndex(-1)
        self.assertRaises(Exception, alg.execute)
        
    def test_workspace_index_list_throw_if_not_pairs(self):
        alg = self.construct_standard_algorithm()
        alg.set_ProcessingInstructions("0")
        self.assertRaises(Exception, alg.execute)
        
    def test_workspace_index_list_values_not_positive_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_ProcessingInstructions("-1, 0") # -1 is not acceptable.
        self.assertRaises(Exception, alg.execute)
    
    def test_workspace_index_list_min_max_pairs_throw_if_min_greater_than_max(self):
        alg = self.construct_standard_algorithm()
        alg.set_ProcessingInstructions("1, 0") # 1 > 0
        self.assertRaises(Exception, alg.execute)
        
    def test_region_of_interest_throws_if_i0monitor_index_negative(self):
        alg = self.construct_standard_algorithm()
        alg.set_I0MonitorIndex(-1)
        self.assertRaises(Exception, alg.execute)
        
    def test_cannot_set_direct_beam_region_of_interest_without_multidetector_run(self):
        alg = self.construct_standard_algorithm()
        alg.set_AnalysisMode("PointDetectorAnalysis")
        alg.set_RegionOfDirectBeam([1, 2])
        self.assertRaises(Exception, alg.execute)
        
    def test_region_of_direct_beam_indexes_cannot_be_negative_or_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_RegionOfDirectBeam([0, -1]);
        self.assertRaises(Exception, alg.execute)
        
    def test_region_of_direct_beam_indexes_must_be_provided_as_min_max_order_or_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_RegionOfDirectBeam([1, 0]);
        self.assertRaises(Exception, alg.execute)
        
    def test_bad_detector_component_name_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_DetectorComponentName("made-up")
        self.assertRaises(Exception, alg.execute)
    
    def test_bad_sample_component_name_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SampleComponentName("made-up")
        self.assertRaises(Exception, alg.execute)
        
    def test_point_detector_run_with_single_transmission_workspace(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(real_run) # Currently a requirement that one transmisson correction is provided.
        alg.set_ThetaIn(0.2)
        
        out_ws_q, out_ws_lam, theta = alg.execute()
        self.assertEqual(0.2, theta, "Theta in and out should be the same")
        
        self.assertTrue(isinstance(out_ws_lam, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", out_ws_lam.getAxis(0).getUnit().unitID())
        
        self.assertTrue(isinstance(out_ws_q, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("MomentumTransfer", out_ws_q.getAxis(0).getUnit().unitID())
        
        self.assertEqual(2, out_ws_lam.getNumberHistograms())
        DeleteWorkspace(real_run)
    
    def test_point_detector_run_with_two_transmission_workspaces(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        trans_run1 = Load('INTER00013463.nxs')
        trans_run2 = Load('INTER00013464.nxs')
        
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(trans_run1) 
        alg.set_SecondTransmissionRun(trans_run2)
        
        alg.set_Params([0.0, 0.02, 5])
        alg.set_StartOverlap( 10.0 )
        alg.set_EndOverlap( 12.0 )
        alg.set_ThetaIn(0.2)
        
        out_ws_q, out_ws_lam, theta = alg.execute()
        
        DeleteWorkspace(real_run)
        DeleteWorkspace(trans_run1)
        DeleteWorkspace(trans_run2)
    
    def test_spectrum_map_mismatch_throws_when_strict(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        trans_run1_tof = Load('INTER00013463.nxs')
        '''
        Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
        and strict checking is turned on, so this will throw upon execution.
        '''
        trans_run1_lam = ConvertUnits(trans_run1_tof, Target='Wavelength')
        trans_run1_lam = CropWorkspace(trans_run1_lam, EndWorkspaceIndex=1)
        
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4") # This will make spectrum numbers in input workspace different from denominator
        alg.set_FirstTransmissionRun(trans_run1_lam) 
        alg.set_StrictSpectrumChecking(True)
        
        self.assertRaises(Exception, alg.execute) # Should throw due to spectrum missmatch.
        
        DeleteWorkspace(real_run)
        DeleteWorkspace(trans_run1_tof)
        DeleteWorkspace(trans_run1_lam)
        
    def test_spectrum_map_mismatch_doesnt_throw_when_not_strict(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        trans_run1_tof = Load('INTER00013463.nxs')
        '''
        Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
        and strict checking is turned off, so this will NOT throw upon execution.
        '''
        trans_run1_lam = ConvertUnits(trans_run1_tof, Target='Wavelength')
        trans_run1_lam = CropWorkspace(trans_run1_lam, EndWorkspaceIndex=1)
        
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4") # This will make spectrum numbers in input workspace different from denominator
        alg.set_FirstTransmissionRun(trans_run1_lam) 
        alg.set_StrictSpectrumChecking(False) # Will not crash-out on spectrum checking.
        
        alg.execute()# Should not throw
        
        DeleteWorkspace(real_run)
        DeleteWorkspace(trans_run1_tof)
        DeleteWorkspace(trans_run1_lam)
        
        
    def test_calculate_theta(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(real_run) # Currently a requirement that one transmisson correction is provided.
        
        out_ws_q, out_ws_lam, theta = alg.execute()
        self.assertAlmostEqual(0.70969419, theta, 4)
        
        DeleteWorkspace(real_run)
           
    def test_correct_positions_point_detector(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('INTER00013460.nxs')
        alg.set_InputWorkspace(real_run)
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(real_run) # Currently a requirement that one transmisson correction is provided.
        alg.set_ThetaIn(0.4) # Low angle
        alg.set_CorrectDetectorPositions(True)
        out_ws_q1, out_ws_lam1, theta1 = alg.execute()
        pos1 = out_ws_lam1.getInstrument().getComponentByName('point-detector').getPos()
        
        alg.set_ThetaIn(0.8) # Repeat with greater incident angle
        out_ws_q2, out_ws_lam2, theta2 = alg.execute()
        pos2 = out_ws_lam2.getInstrument().getComponentByName('point-detector').getPos()
        
        self.assertTrue(pos2.Y() > pos1.Y(), "Greater incident angle so greater height.")
        self.assertEqual(pos2.X(), pos1.X())
        self.assertEqual(pos2.Z(), pos1.Z())
        DeleteWorkspace(real_run)
        
    def test_multidetector_run(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('POLREF00004699.nxs')
        alg.set_InputWorkspace(real_run[0])
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_CorrectDetectorPositions(False)
        alg.set_ProcessingInstructions("3, 10") # Fictional values
        alg.set_RegionOfDirectBeam("20, 30") # Fictional values
        alg.set_ThetaIn(0.1) # Fictional values
        
        out_ws_q, out_ws_lam, theta =  alg.execute()
        
        self.assertTrue(isinstance(out_ws_lam, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", out_ws_lam.getAxis(0).getUnit().unitID())
        
        self.assertTrue(isinstance(out_ws_q, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("MomentumTransfer", out_ws_q.getAxis(0).getUnit().unitID())
        
    def test_correct_positions_multi_detector(self):
        alg = self.construct_standard_algorithm()
        real_run = Load('POLREF00004699.nxs')
        alg.set_InputWorkspace(real_run[0])
        alg.set_AnalysisMode("MultiDetectorAnalysis")
        alg.set_CorrectDetectorPositions(True)
        alg.set_ProcessingInstructions("73") 
        alg.set_RegionOfDirectBeam("28,29") 
        alg.set_ThetaIn(0.49/2) 
        out_ws_q, out_ws_lam, theta =  alg.execute()
        
        pos = out_ws_lam.getInstrument().getComponentByName('lineardetector').getPos()
        self.assertAlmostEqual(-0.05714, pos.Z(), 3, "Vertical correction is wrong. Recorded as: " + str(pos.Z()))
        
        
        
        
