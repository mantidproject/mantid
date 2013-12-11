import unittest
from mantid.simpleapi import CreateTransmissionWorkspace, CreateWorkspace, DeleteWorkspace

import inspect
import re

def make_decorator(algorithm_to_decorate):
    """
    Dynamically create a builder pattern style decorator around a Mantid algorithm.
    This allows you to separate out setting algorithm parameters from the actual method execution. Parameters may be reset multiple times.
    
    Usage:
     rebin = make_decorator(Rebin)
     rebin.set_Params([0, 0.1, 1])
     ....
     rebin.execute()
    
    Arguments:
     algorithm_to_decorate: The mantid.simpleapi algorithm to decorate.
     
     
    
    """
    
    class Decorator(object):
        
        def __init__(self, alg_subject):
            self.__alg_subject = alg_subject
            self.__parameters__ = dict()
        
        def execute(self, additional=None, verbose=False):
            if verbose:
                print "Algorithm Parameters:"
                print self.__parameters__
                print 
            out = self.__alg_subject(**self.__parameters__)
            return out
        
        def set_additional(self, additional):
            self.__parameters__.update(**additional)

    def add_getter_setter(type, name):
        
        def setter(self, x):
            self.__parameters__[name] = x
            
        def getter(self):
            return self.__parameters__[name]
            
        setattr(type, "set_" + name, setter)
        setattr(type, "get_" + name, getter)


    argspec = inspect.getargspec(algorithm_to_decorate)
    for parameter in argspec.varargs.split(','):
        m = re.search('(^\w+)', parameter) # Take the parameter key part from the defaults given as 'key=value'
        if m:
            parameter = m.group(0).strip()
        m = re.search('\w+$', parameter) # strip off any leading numerical values produced by argspec
        if m:
            parameter = m.group(0).strip()
        add_getter_setter(Decorator, m.group(0).strip())

    return Decorator(algorithm_to_decorate) 


class CreateTransmissionWorkspaceTest(unittest.TestCase):
    
    def setUp(self):
        tof = CreateWorkspace(UnitX="TOF", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0,0], DataY=[0,0,0], NSpec=1)
        self.__tof = tof
        self.__not_tof = not_tof
        
    def tearDown(self):
        DeleteWorkspace(self.__tof)
        DeleteWorkspace(self.__not_tof)
    
    def construct_standard_algorithm(self):
        alg = make_decorator(CreateTransmissionWorkspace)
        alg.set_InputWorkspace(self.__tof)
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(1.0)
        alg.set_I0MonitorIndex(0)
        alg.set_WorkspaceIndexList([0, 1])
        alg.set_MonitorBackgroundWavelengthMin(0.0)
        alg.set_MonitorBackgroundWavelengthMax(1.0)
        alg.set_MonitorIntegrationWavelengthMin(0.0)
        alg.set_MonitorIntegrationWavelengthMax(1.0)
        return alg
    
    def test_input_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_InputWorkspace(self.__not_tof)
        self.assertRaises(ValueError, alg.execute)
      
    def test_second_transmission_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionWorkspace(self.__not_tof)
        self.assertRaises(ValueError, alg.execute)
        
    def test_provide_second_transmission_run_without_params_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionWorkspace(self.__tof)
        self.assertRaises(ValueError, alg.execute)
        
    def test_provide_second_transmission_run_without_start_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionWorkspace(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_EndOverlapQ( 0.4 )
        self.assertRaises(ValueError, alg.execute)
        
    def test_provide_end_transmission_run_without_end_overlap_q_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionWorkspace(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlapQ( 0.4 )
        self.assertRaises(ValueError, alg.execute)
        
    def test_end_overlap_q_must_be_greater_than_start_overlap_q_or_throw(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionWorkspace(self.__tof)
        alg.set_Params([0, 0.1, 1])
        alg.set_StartOverlapQ( 0.6 )
        alg.set_EndOverlapQ( 0.4 )
        self.assertRaises(ValueError, alg.execute)
        
    def test_must_provide_wavelengths(self):
        self.assertRaises(RuntimeError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMin=1.0)
        self.assertRaises(RuntimeError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMax=1.0)
        
        
    def test_execute(self):
        alg = self.construct_standard_algorithm()
        alg.execute()
            
 
if __name__ == '__main__':
    unittest.main()