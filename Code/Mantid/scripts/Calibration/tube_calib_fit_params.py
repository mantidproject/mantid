from mantid.simpleapi import *
from mantid.kernel import *

class TubeCalibFitParams:

# This class is to take the fitting method and parameters for fitting the peaks crated by the calibration slits etc
# and to deliver them to TubeCalib, so it can fit the peaks appropriately

# Author: Karl Palmen ISIS

    def __init__(self, peaks, Height=1000.0, Width=30.0, ThreePointMethod=False, outEdge=30.0,  inEdge=50.0, EdgeGrad=6.0, Margin=0.4):

        """     
        Creates an instance
        
        @param peaks: expected positions of the peaks in pixels
        @param height: expect height of peaks
        @param width: expected width (sigma for Gaussian fitting) of peaks
        @param ThreePointMethod: True if three point method is used (first and last peaks are the extreme ends of very wide peaks).
        """	
        # Peaks
        self.height = Height*1.0
        self.width = Width*1.0
        self.peaks = peaks
        # Margin
        self.margin = Margin
        # Three pointMethod parameter (to be phased out)
        self.threePointMethod = ThreePointMethod
        # Edges
        self.outEdge = outEdge
        self.inEdge = inEdge
        self.edgeGrad = EdgeGrad
        
    def getPeaks(self):
        return self.peaks
        
    def getHeightAndWidth(self):
        return self.height, self.width
        
    def isThreePointMethod(self):
        return self.threePointMethod
        
    def getEdgeParameters(self):
        return self.outEdge, self.inEdge, self.edgeGrad
        
    def getMargin(self):
        return self.margin
        
        