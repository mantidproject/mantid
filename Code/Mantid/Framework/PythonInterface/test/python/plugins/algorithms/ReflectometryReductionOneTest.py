import unittest
from mantid.simpleapi import *

import inspect
import re

def make_decorator(algorithm_to_decorate):
    """
    Dynamically create a builder pattern style decorator around a Mantid algorithm.
    This allows you to separate out setting algorithm parameters from the actual method execution. Parameters may be reset multiple times.
    """
    
    class Decorator(object):
        
        def __init__(self, alg_subject):
            self.__alg_subject = alg_subject
            self.__parameters__ = dict()
        
        def execute(self, verbose=False):
            if verbose:
                print "Algorithm Parameters:"
                print self.__parameters__
                print 
            out = self.__alg_subject(**self.__parameters__)
            return out

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


class ReflectometryReductionOneTest(unittest.TestCase):
    
    def construct_standard_algorithm(self):
        alg = make_decorator(ReflectometryReductionOne)
        alg.set_InputWorkspace(self.__tof.getName())
        alg.set_WavelengthMin(0.0)
        alg.set_WavelengthMax(1.0)
        alg.set_I0MonitorIndex(0)
        alg.set_MonitorBackgroundWavelengthMin(0.0)
        alg.set_MonitorBackgroundWavelengthMax(1.0)
        alg.set_MonitorIntegrationWavelengthMin(0.0)
        alg.set_MonitorIntegrationWavelengthMax(1.0)
        return alg
    
    def setUp(self):
        tof = CreateWorkspace(UnitX="TOF", DataX=[0,0,0], DataY=[0,0,0], NSpec=1)
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0], DataY=[0,0,0], NSpec=1)
        self.__tof = tof
        self.__not_tof = not_tof
        
    def tearDown(self):
        DeleteWorkspace(self.__tof)
        DeleteWorkspace(self.__not_tof)
    
    def test_check_input_workpace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_InputWorkspace(self.__not_tof)
        self.assertRaises(ValueError, alg.execute)
    
    def test_check_first_transmission_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_FirstTransmissionRun(self.__not_tof)
        self.assertRaises(ValueError, alg.execute)
    
    def test_check_second_transmission_workspace_not_tof_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_SecondTransmissionRun(self.__not_tof)
        self.assertRaises(ValueError, alg.execute)
        
    def test_must_provide_wavelengths(self):
        self.assertRaises(RuntimeError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMin=1.0)
        self.assertRaises(RuntimeError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMax=1.0)
        #ReflectometryReductionOne( InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof, WavelengthMin=0, WavelengthMax=1.0, I0MonitorIndex=0)
    
    def test_wavelength_min_greater_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_WavelengthMin(1.0)
        alg.set_WavelengthMax(0.0)
        self.assertRaises(ValueError, alg.execute)
        
    def test_monitor_background_wavelength_min_greater_monitor_background_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_MonitorBackgroundWavelengthMin(1.0)
        alg.set_MonitorBackgroundWavelengthMax(0.0)
        self.assertRaises(ValueError, alg.execute)
    
    def test_monitor_integration_wavelength_min_greater_monitor_integration_wavelength_max_throws(self):
        alg = self.construct_standard_algorithm()
        alg.set_MonitorIntegrationWavelengthMin(1.0)
        alg.set_MonitorIntegrationWavelengthMax(0.0)
        self.assertRaises(ValueError, alg.execute)
        
        
if __name__ == '__main__':
    unittest.main()
