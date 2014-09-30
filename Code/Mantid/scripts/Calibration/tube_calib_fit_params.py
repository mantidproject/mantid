from mantid.simpleapi import *
from mantid.kernel import *

class TubeCalibFitParams:

# This class is to take the fitting method and parameters for fitting the peaks crated by the calibration slits etc
# and to deliver them to TubeCalib, so it can fit the peaks appropriately

# Author: Karl Palmen ISIS

    def __init__(self, peaks, height=1000.0, width=30.0, threePointMethod=False, outEdge=30.0,  inEdge=50.0, edgeGrad=6.0, margin=15):

        """
        Holds the parameters needed for fitting the positions of the peaks formed by the slits or edges.

        The constructor has the following arguments:

        :param peaks: expected positions of the peaks in pixels
        :param height: expect height of peaks
        :param width: expected width (sigma for Gaussian fitting) of peaks
        :param threePointMethod: True if three point method is used (first and last peaks are the extreme ends of very wide peaks).
        :param margin: defines the region around the peak that will be considered for fitting

        This class has also an attribute, called automatic, accessed through
        :meth:`~tube_calib_fit_params.TubeCalibFitParams.getAutomatic`, that
        defines it a dynamic evaluation of the fit parameters can be used or not
        (see :func:`~tube.calibrate` to check how the automatic flag will be used).

        The function :func:`~tube_calib.getCalibration` of :mod:`tube_calib` needs such an object.
        """
        # Peaks
        self.height = height*1.0
        self.width = width*1.0
        self.peaks = peaks
        # Margin
        self.margin = margin
        # Three pointMethod parameter (to be phased out)
        self.threePointMethod = threePointMethod
        # Edges
        self.outEdge = outEdge
        self.inEdge = inEdge
        self.edgeGrad = edgeGrad
        self.automatic = False

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

    def setMargin(self, mar):
        self.margin = mar

    def setAutomatic(self, boolOption):
        self.automatic = boolOption

    def getAutomatic(self):
        return self.automatic

    def __str__(self):
        return 'peaks'+str(self.peaks)+'height'+str(self.height)+'width'+str(self.width)+'margin'+str(self.margin)+'outedge'+str(self.outEdge)+'inedge'+str(self.inEdge)+'edgegrad'+str(self.edgeGrad       )

