from mantid.kernel import *
from mantid.api import *

class EnginXFocus(PythonAlgorithm):
    def category(self):
        return "Diffraction\Engineering;PythonAlgorithms"

    def name(self):
        return "EnginXFocus"

    def summary(self):
        return "Focuses a run."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", FileAction.Load),
    		"Run to focus")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output),
    		"A workspace with focussed data")

        self.declareProperty(ITableWorkspaceProperty("DetectorPositions", "", Direction.Input, PropertyMode.Optional),
    		"Calibrated detector positions. If not specified, default ones are used.")

        self.declareProperty("Bank", 1, "Which bank to focus")



    def PyExec(self):
    	# Load the run file
        ws = self._loadRun()

    	# Leave the data for the bank we are interested in only
        ws = self._cropData(ws)

    	# Apply calibration
        self._applyCalibration(ws)

    	# Convert to dSpacing
        ws = self._convertToDSpacing(ws)

    	# Sum the values
        ws = self._sumSpectra(ws)

    	# Convert back to time of flight
        ws = self._convertToTOF(ws)

    	# OpenGenie displays distributions instead of pure counts (this is done implicitly when
    	# converting units), so I guess that's what users will expect
        self._convertToDistr(ws)

        self.setProperty("OutputWorkspace", ws)

    def _loadRun(self):
        """ Loads the specified run
    	"""
        alg = self.createChildAlgorithm('Load')
        alg.setProperty('Filename', self.getProperty("Filename").value)
        alg.execute()
        return alg.getProperty('OutputWorkspace').value

    def _applyCalibration(self, ws):
        """ Refines the detector positions using the result of calibration (if one is specified)
    	"""
        detPos = self.getProperty("DetectorPositions").value

        if detPos:
            alg = self.createChildAlgorithm('ApplyCalibration')
            alg.setProperty('Workspace', ws)
            alg.setProperty('PositionTable', detPos)
            alg.execute()

    def _convertToDSpacing(self, ws):
        """ Converts workspace to dSpacing
    	"""
        alg = self.createChildAlgorithm('ConvertUnits')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('Target', 'dSpacing')
        alg.setProperty('AlignBins', True)
        alg.execute()
        return alg.getProperty('OutputWorkspace').value

    def _convertToTOF(self, ws):
        """ Converts workspace to TOF
    	"""
        alg = self.createChildAlgorithm('ConvertUnits')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('Target', 'TOF')
        alg.execute()
        return alg.getProperty('OutputWorkspace').value

    def _convertToDistr(self, ws):
        """ Convert workspace to distribution
    	"""
        alg = self.createChildAlgorithm('ConvertToDistribution')
        alg.setProperty('Workspace', ws)
        alg.execute()

    def _cropData(self, ws):
        """ Crops the workspace so that only data for the specified bank is left.

    	    NB: This assumes spectra for a bank are consequent.
    	"""

        import EnginXUtils

        indices = EnginXUtils.getWsIndicesForBank(self.getProperty('Bank').value, ws)

    	# Leave only spectra between min and max
        alg = self.createChildAlgorithm('CropWorkspace')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('StartWorkspaceIndex', min(indices))
        alg.setProperty('EndWorkspaceIndex', max(indices))
        alg.execute()

        return alg.getProperty('OutputWorkspace').value

    def _sumSpectra(self, ws):
        """ Calls the SumSpectra algorithm
    	"""
        alg = self.createChildAlgorithm('SumSpectra')
        alg.setProperty('InputWorkspace', ws)
        alg.execute()
        return alg.getProperty('OutputWorkspace').value

AlgorithmFactory.subscribe(EnginXFocus)
