from MantidFramework import *
from mantidsimple import *
from math import sin
from math import cos
from math import sqrt

class Elwin(PythonAlgorithm):
	def category(self):
		return "MSG"
	def name(self):
		return "Elwin"
	def PyInit(self):
		# declare properties
		self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input)
		self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)
		self.declareListProperty("EnergyRange", [-0.1, 0.1])
		self.declareProperty("EFixed", 1.0)
		# set constants (non-property dependent)
		self.hbar = 6.582118e-16
	
	def PyExec(self):
		self.inWS = self.getProperty("InputWorkspace")
		self.eRange = self.getProperty("EnergyRange")
		self.eFixed = self.getProperty("EFixed")
		nhist = self.inWS.getNumberHistograms()
		samplePos = self.inWS.getInstrument().getSample().getPos()
		beamPos = samplePos - self.inWS.getInstrument().getSource().getPos()
		self.kf = self.getK(self.eFixed)
		if not (self.eRange[0] < 0.0) and (self.eRange[1] > 0.0):
			self.omega = ( eRange[0] + eRange[1] ) / 2
			self.ei = self.hbar * self.omega + self.eFixed
			self.ki = self.getK(self.ei)

		# Perform the intergration steps here
		if ( len(self.eRange) == 2 ):
			Integration(self.inWS, '_elwin_tmp_ws', RangeLower=self.eRange[0], RangeUpper=self.eRange[1])
		elif ( len(self.eRange) == 4 ):
			FlatBackground(self.inWS, '_elwin_tmp_ws', StartX=self.eRange[2], EndX=self.eRange[3], Mode='Mean')
			Integration('_elwin_tmp_ws', '_elwin_tmp_ws', RangeLower=self.eRange[0], RangeUpper=self.eRange[1])
		else:
			sys.exit(1)

		temp = mantid.getMatrixWorkspace('_elwin_tmp_ws')
		outWS = WorkspaceFactory.createMatrixWorkspace(1, nhist+1, nhist)
		for i in range(0, nhist):
			outWS.dataY(0)[i] = temp.readY(i)[0]
			outWS.dataE(0)[i] = temp.readE(i)[0]
			twoTheta = temp.getDetector(i).getTwoTheta(samplePos, beamPos)
			q = self.getQ(twoTheta)
			outWS.dataX(0)[i] = q * q
		outWS.dataX(0)[nhist] = 2*outWS.dataX(0)[nhist-1]-outWS.dataX(0)[nhist-2]
		mantid.deleteWorkspace('_elwin_tmp_ws')
		self.setProperty("OutputWorkspace", outWS)
		
	def getK(self, e):
		k = sqrt(e/2.072)
		return k
		
	def getQ(self, twoTheta):
		if (self.eRange[0] < 0.0) and (self.eRange[1] > 0.0):
			q =  2 * self.kf * sin( twoTheta /2)
		else:
			q = sqrt( ( self.ki^2 + self.kf^2 ) - ( 2 * self.ki * self.kf * cos(twoTheta) ) )
		return q
# Register algorithm with Mantid
mantid.registerPyAlgorithm(Elwin())
