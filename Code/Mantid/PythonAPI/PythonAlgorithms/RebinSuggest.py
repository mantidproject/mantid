from MantidFramework import *
from mantidsimple import *

class RebinSuggest(PythonAlgorithm):
	def PyInit(self):
		self.declareWorkspaceProperty('InputWorkspace', '', Direction.Input)
		self.declareWorkspaceProperty('OutputWorkspace', '', Direction.Output)
			
	def PyExec(self):
		inputWS = self.getProperty('InputWorkspace')
		nBins = inputWS.getNumberBins() - 1
		nHist = inputWS.getNumberHistograms()
		
		for i in range(0, nHist):
			if (i == 0):
				xMin = inputWS.readX(0)[0]
				xMax = inputWS.readX(0)[nBins]
			else:
				if (inputWS.readX(i)[0] < xMin):
					xMin = inputWS.readX(i)[0]
				if (inputWS.readX(i)[nBins] > xMax):
					xMax = inputWS.readX(i)[nBins]
		
		xStep = ( xMax - xMin ) / (nBins + 1)
		
		param = str(xMin) + "," + str(xStep) + "," + str(xMax)
		
		Rebin(inputWS, '__tmp__', param)
		
		outputWS = mantid.getMatrixWorkspace('__tmp__')
		
		self.setProperty('OutputWorkspace', outputWS)

		mantid.deleteWorkspace('__tmp__')
	
	def category(self):
		return "MSG"

mantid.registerPyAlgorithm(RebinSuggest())