"""
    Implementation of reduction steps for ISIS SANS instruments
"""
from Reducer import ReductionStep
from SANSReductionSteps import BaseTransmission
from mantidsimple import *

class Transmission(ReductionStep):
    """
        Transmission calculation for ISIS SANS instruments
    """
    
    # Map input values to Mantid options
    TRANS_FIT_OPTIONS = {
        'YLOG' : 'Log',
        'STRAIGHT' : 'Linear',
        'CLEAR' : 'Off',
        # Add Mantid ones as well
        'LOG' : 'Log',
        'LINEAR' : 'Linear',
        'LIN' : 'Linear',
        'OFF' : 'Off'
    }    
    
    def __init__(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        """
            @param lambda_min: MinWavelength parameter for CalculateTransmission
            @param lambda_max: MaxWavelength parameter for CalculateTransmission
            @param fit_method: FitMethod parameter for CalculateTransmission (Linear or Log)
        """
        super(Transmission, self).__init__()
        self._trans = 1.0
        self._error = 0.0
        self._lambda_min = lambda_min
        self._lambda_max = lambda_max
        self._fit_method = fit_method
            
        fit_method = fit_method.upper()
        if fit_method in self.TRANS_FIT_OPTIONS.keys():
            self._fit_method = self.TRANS_FIT_OPTIONS[fit_method]
        else:
            self._fit_method = 'Log'      
            mantid.sendLogMessage("ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default LOG method")
            
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        """
            Calls the CalculateTransmission algorithm
        """ 
        if self._lambda_max == None:
            self._lambda_max = reducer.instrument.TRANS_WAV1_FULL
        if self._lambda_min == None:
            self._lambda_min = reducer.instrument.TRANS_WAV2_FULL
        
        raise NotImplemented
    
    