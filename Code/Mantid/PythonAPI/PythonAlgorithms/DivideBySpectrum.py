from MantidFramework import *

class DivideBySpectrum(PythonAlgorithm):

	def category(self):
		return "MSG"
	
	def PyInit(self):
		self.declareWorkspaceProperty("LHS", "", Direction.Input)
		self.declareWorkspaceProperty("RHS", "", Direction.Input)
		self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)
	
	def PyExec(self):
		lhs_ws = self.getProperty("LHS")
		rhs_ws = self.getProperty("RHS")
		
		if ( lhs_ws.getNumberBins() != rhs_ws.getNumberBins() ):
			sys.exit("Number of bins in input workspaces do not match.")
		
		outWS = WorkspaceFactory.createMatrixWorkspaceFromCopy(lhs_ws)
		
		nhist = lhs_ws.getNumberHistograms()
		nbins = lhs_ws.getNumberBins()
		
		for i in range(0, nhist):
			
			for j in range(0, nbins):
				outWS.dataX(i)[j] = lhs_ws.readX(i)[j]
				
				if ( lhs_ws.readY(i)[j] == 0 ) or ( rhs_ws.readY(0)[j] == 0 ):
					outWS.dataY(i)[j] = 0
				else:
					outWS.dataY(i)[j] = ( lhs_ws.readY(i)[j] / rhs_ws.readY(0)[j] )
				if ( lhs_ws.readE(i)[j] == 0 ) or ( rhs_ws.readE(0)[j] == 0 ):
					outWS.dataE(i)[j] = 0
				else:
					outWS.dataE(i)[j] = ( lhs_ws.readE(i)[j] / rhs_ws.readE(0)[j] )
			outWS.dataX(i)[nbins] =  lhs_ws.readX(i)[nbins]
		
		self.setProperty("OutputWorkspace", outWS)

mantid.registerPyAlgorithm(DivideBySpectrum())
