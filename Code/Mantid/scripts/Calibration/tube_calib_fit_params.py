from mantid.simpleapi import *
from mantid.kernel import *

class TubeCalibFitParams:

# This class is to take the fitting method and parameters for fitting the peaks crated by the calibration slits etc
# and to deliver them to TubeCalib, so it can fit the peaks appropriately

# Author: Karl Palmen ISIS

    def __init__(self, peaks, Height=1000.0, Width=30.0, ThreePointMethod=False):
        """     
        Creates an instance
        
        @param peaks: expected positions of the peaks in pixels
        @param height: expect height of peaks
        @param width: expected width (sigma for Gaussian fitting) of peaks
        @param ThreePointMethod: True if three point method is used (first and last peaks are the extreme ends of very wide peaks).
        """	
        self.height = Height*1.0
        self.width = Width*1.0
        self.peaks = peaks
        self.threePointMethod = ThreePointMethod
        
    def getPeaks(self):
        return self.peaks
        
    def getHeightAndWidth(self):
        return self.height, self.width
        
    def isThreePointMethod(self):
        return self.threePointMethod
        