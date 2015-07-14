#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *

class EnggFocus(PythonAlgorithm):
    INDICES_PROP_NAME = 'SpectrumNumbers'

    def category(self):
        return "Diffraction\\Engineering;PythonAlgorithms"

    def name(self):
        return "EnggFocus"

    def summary(self):
        return "Focuses a run."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Workspace with the run to focus.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output),\
                             "A workspace with focussed data")

        import EnggUtils
        self.declareProperty("Bank", '', StringListValidator(EnggUtils.ENGINX_BANKS),
                             direction=Direction.Input,
                             doc = "Which bank to focus: It can be specified as 1 or 2, or "
                             "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
                             "for a more flexible alternative to select specific detectors")

        self.declareProperty(self.INDICES_PROP_NAME, '', direction=Direction.Input,
                             doc = 'Sets the spectrum numbers for the detectors '
                             'that should be considered in the focussing operation (all others will be '
                             'ignored). This options cannot be used together with Bank, as they overlap. '
                             'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".')

        self.declareProperty(ITableWorkspaceProperty('DetectorPositions', '', Direction.Input,\
                                                     PropertyMode.Optional),\
                             "Calibrated detector positions. If not specified, default ones are used.")


    def PyExec(self):
        import EnggUtils

        # Get the run workspace
        ws = self.getProperty('InputWorkspace').value

        indices = EnggUtils.getWsIndicesFromInProperties(ws, self.getProperty('Bank').value,
                                                           self.getProperty(self.INDICES_PROP_NAME).value)

    	# Leave the data for the bank we are interested in only
        ws = self._cropData(ws, indices)

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

    def _cropData(self, ws, indices):
        """
        Produces a cropped workspace from the input workspace so that only
        data for the specified bank is left.

        NB: This assumes spectra for a bank are consequent.

        @param ws :: workspace to crop (not modified in-place)
        @param bank :: workspace indices to keep in the workpace returned

        @returns cropped workspace, with only the spectra corresponding to the indices requested
        """
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


AlgorithmFactory.subscribe(EnggFocus)
