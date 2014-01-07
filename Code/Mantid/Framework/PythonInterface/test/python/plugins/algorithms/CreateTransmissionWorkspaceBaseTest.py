import unittest
import mantid.api
import abc
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, Load
from testhelpers.algorithm_decorator import make_decorator

import inspect
import re

class CreateTransmissionWorkspaceBaseTest(object):
    __metaclass__  = abc.ABCMeta
    
    @abc.abstractmethod
    def algorithm_type(self):
        raise RuntimeError("Not implemented")
    
    def setUp(self):
        tof = CreateWorkspace(UnitX="TOF", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        self.__tof = tof
        self.__not_tof = not_tof
        
    def tearDown(self):
        DeleteWorkspace(self.__tof)
        DeleteWorkspace(self.__not_tof)
    
    def construct_standard_algorithm(self):
        alg = make_decorator(self.algorithm_type())
        alg.set_FirstTransmissionRun(self.__tof)
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(1.0)
        alg.set_I0MonitorIndex(0)
        alg.set_ProcessingInstructions("0, 1")
        alg.set_MonitorBackgroundWavelengthMin(0.0)
        alg.set_MonitorBackgroundWavelengthMax(1.0)
        alg.set_MonitorIntegrationWavelengthMin(0.0)
        alg.set_MonitorIntegrationWavelengthMax(1.0)
        return alg
    
    def test_input_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__not_tof)
        self.assertRaises(Exception, alg.execute)
      
    def test_second_transmission_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__not_tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_second_transmission_run_without_params_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__tof)
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_second_transmission_run_without_start_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_EndOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_provide_end_transmission_run_without_end_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_end_overlap_q_must_be_greater_than_start_overlap_q_or_throw(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlap( 0.6 )
        alg.set_EndOverlap( 0.4 )
        self.assertRaises(Exception, alg.execute)
        
    def test_must_provide_wavelengths(self):
        self.assertRaises(RuntimeError, self.algorithm_type(), FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMin=1.0)
        self.assertRaises(RuntimeError, self.algorithm_type(), FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMax=1.0)
        
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
        
    def test_spectrum_map_mismatch_throws(self):
        alg = self.construct_standard_algorithm()
        trans_run1 = Load('INTER00013463.nxs')
        trans_run2 = self.__tof
        
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(trans_run1) 
        alg.set_SecondTransmissionRun(trans_run2)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlap(1)
        alg.set_EndOverlap(2)
        self.assertRaises(Exception, alg.execute)
        
        DeleteWorkspace(trans_run1)
        
    def test_execute_one_tranmission(self):
        alg = make_decorator(self.algorithm_type())
        
        trans_run1 = Load('INTER00013463.nxs')
        
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(trans_run1) 
        alg.set_I0MonitorIndex(0)
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(17.9)
        alg.set_WavelengthStep(0.5)
        alg.set_MonitorBackgroundWavelengthMin(15.0)
        alg.set_MonitorBackgroundWavelengthMax(17.0)
        alg.set_MonitorIntegrationWavelengthMin(4.0)
        alg.set_MonitorIntegrationWavelengthMax(10.0)
        
        transmission_ws = alg.execute()
        
        self.assertTrue(isinstance(transmission_ws, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", transmission_ws.getAxis(0).getUnit().unitID())
        
        # Because we have one transmission workspaces, binning should come from the WavelengthStep.
        x = transmission_ws.readX(0)
        actual_binning = x[1] - x[0]
        step = alg.get_WavelengthStep()
        self.assertAlmostEqual( actual_binning, step, 6)
        
        DeleteWorkspace(trans_run1)
        DeleteWorkspace(transmission_ws)
        
    def test_execute_two_tranmissions(self):
        alg = make_decorator(self.algorithm_type())
        
        trans_run1 = Load('INTER00013463.nxs')
        trans_run2 = Load('INTER00013464.nxs')
        
        alg.set_ProcessingInstructions("3,4")
        alg.set_FirstTransmissionRun(trans_run1) 
        alg.set_SecondTransmissionRun(trans_run2)
        alg.set_I0MonitorIndex(0)
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(17.9)
        alg.set_WavelengthStep(0.5)
        alg.set_MonitorBackgroundWavelengthMin(15.0)
        alg.set_MonitorBackgroundWavelengthMax(17.0)
        alg.set_MonitorIntegrationWavelengthMin(4.0)
        alg.set_MonitorIntegrationWavelengthMax(10.0)
        alg.set_Params([1.5, 0.02, 17])
        alg.set_StartOverlap( 10.0 )
        alg.set_EndOverlap( 12.0 )
        
        transmission_ws = alg.execute()
        
        self.assertTrue(isinstance(transmission_ws, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Wavelength", transmission_ws.getAxis(0).getUnit().unitID())
        
        # Because we have two transmission workspaces, binning should come from the Params for stitching.
        x = transmission_ws.readX(0)
        actual_binning = x[1] - x[0]
        params = alg.get_Params()
        self.assertAlmostEqual( actual_binning, params[1], 6)
        self.assertAlmostEqual( 1.5, params[0], 6)
        self.assertAlmostEqual( 17, params[2], 6)
        
        DeleteWorkspace(trans_run1)
        DeleteWorkspace(trans_run2)
        DeleteWorkspace(transmission_ws)
            