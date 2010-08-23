from MantidFramework import *
from mantidsimple import *

class ExtractFFTSpectrum(PythonAlgorithm):

	def category(self):
		return "MSG"
	def name(self):
		return "ExtractFFTSpectrum"

	def PyInit(self):
		self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input)
		self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)
		self.declareProperty("FFTPart", 2)
		
	def PyExec(self):
		inWS = self.getProperty("InputWorkspace")
		fftPart = self.getProperty("FFTPart")
		nhist = inWS.getNumberHistograms()
		nbins = inWS.getNumberBins()
		
		for i in range(0, nhist):
			tmpWS = '_fury_algorithm_temp'
			FFT(inWS, tmpWS, i)
			
			tempWS = mantid.getMatrixWorkspace(tmpWS)
			if ( i == 0 ):
				outWS = WorkspaceFactory.createMatrixWorkspaceFromCopy(tempWS, nhist, nbins+1, nbins)
			for j in range(0, outWS.getNumberBins()):
				outWS.dataX(i)[j] = tempWS.readX(fftPart)[j]
				outWS.dataE(i)[j] = tempWS.readE(fftPart)[j]
				outWS.dataY(i)[j] = tempWS.readY(fftPart)[j]
			outWS.dataX(i)[nbins] = tempWS.readX(fftPart)[nbins]
		
		mantid.deleteWorkspace(tmpWS)
		self.setProperty("OutputWorkspace", outWS)

mantid.registerPyAlgorithm(ExtractFFTSpectrum())
