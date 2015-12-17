#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *

import EnggUtils

class EnggFocus(PythonAlgorithm):
    INDICES_PROP_NAME = 'SpectrumNumbers'

    def category(self):
        return "Diffraction\\Engineering"

    def name(self):
        return "EnggFocus"

    def summary(self):
        return "Focuses a run by summing up all the spectra into a single one."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Workspace with the run to focus.")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "A workspace with focussed data")


        self.declareProperty(ITableWorkspaceProperty('DetectorPositions', '', Direction.Input,
                                                     PropertyMode.Optional),
                             "Calibrated detector positions. If not specified, default ones are used.")

        self.declareProperty(MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input,
                                                     PropertyMode.Optional),
                             doc = 'Workspace with the Vanadium (correction and calibration) run. '
                             'Alternatively, when the Vanadium run has been already processed, '
                             'the properties can be used')

        self.declareProperty(ITableWorkspaceProperty('VanIntegrationWorkspace', '',
                                                     Direction.Input, PropertyMode.Optional),
                             doc = 'Results of integrating the spectra of a Vanadium run, with one column '
                             '(integration result) and one row per spectrum. This can be used in '
                             'combination with OutVanadiumCurveFits from a previous execution and '
                             'VanadiumWorkspace to provide pre-calculated values for Vanadium correction.')

        self.declareProperty(MatrixWorkspaceProperty('VanCurvesWorkspace', '', Direction.Input,
                                                     PropertyMode.Optional),
                             doc = 'A workspace2D with the fitting workspaces corresponding to '
                             'the instrument banks. This workspace has three spectra per bank, as produced '
                             'by the algorithm Fit. This is meant to be used as an alternative input '
                             'VanadiumWorkspace for testing and performance reasons. If not given, no '
                             'workspace is generated.')

        vana_grp = 'Vanadium (open beam) properties'
        self.setPropertyGroup('VanadiumWorkspace', vana_grp)
        self.setPropertyGroup('VanIntegrationWorkspace', vana_grp)
        self.setPropertyGroup('VanCurvesWorkspace', vana_grp)

        self.declareProperty("Bank", '', StringListValidator(EnggUtils.ENGINX_BANKS),
                             direction=Direction.Input,
                             doc = "Which bank to focus: It can be specified as 1 or 2, or "
                             "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
                             "for a more flexible alternative to select specific detectors")

        self.declareProperty(self.INDICES_PROP_NAME, '', direction=Direction.Input,
                             doc = 'Sets the spectrum numbers for the detectors '
                             'that should be considered in the focussing operation (all others will be '
                             'ignored). This option cannot be used together with Bank, as they overlap. '
                             'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".')

        banks_grp = 'Banks / spectra'
        self.setPropertyGroup('Bank', banks_grp)
        self.setPropertyGroup(self.INDICES_PROP_NAME, banks_grp)

    def PyExec(self):
        # Get the run workspace
        wks = self.getProperty('InputWorkspace').value

        # Get spectra indices either from bank or direct list of indices, checking for errors
        bank = self.getProperty('Bank').value
        spectra = self.getProperty(self.INDICES_PROP_NAME).value
        indices = EnggUtils.getWsIndicesFromInProperties(wks, bank, spectra)

    	# Leave the data for the bank we are interested in only
        wks = EnggUtils.cropData(self, wks, indices)

        prog = Progress(self, start=0, end=1, nreports=3)

        prog.report('Preparing input workspace')
        # Leave data for the same bank in the vanadium workspace too
        vanWS = self.getProperty('VanadiumWorkspace').value
        vanIntegWS = self.getProperty('VanIntegrationWorkspace').value
        vanCurvesWS = self.getProperty('VanCurvesWorkspace').value
        EnggUtils.applyVanadiumCorrections(self, wks, indices, vanWS, vanIntegWS, vanCurvesWS)

    	# Apply calibration
        detPos = self.getProperty("DetectorPositions").value
        if detPos:
            self._applyCalibration(wks, detPos)

    	# Convert to dSpacing
        wks = EnggUtils.convertToDSpacing(self, wks)

        prog.report('Summing spectra')
    	# Sum the values
        wks = EnggUtils.sumSpectra(self, wks)

        prog.report('Preparing output workspace')
    	# Convert back to time of flight
        wks = EnggUtils.convertToToF(self, wks)

    	# OpenGenie displays distributions instead of pure counts (this is done implicitly when
    	# converting units), so I guess that's what users will expect
        self._convertToDistr(wks)

        self.setProperty("OutputWorkspace", wks)

    def _applyCalibration(self, wks, detPos):
        """
        Refines the detector positions using the result of calibration (if one is specified).

        @param wks :: workspace to apply the calibration (on its instrument)
        @param detPos :: detector positions (as a table of positions, one row per detector)
    	"""
        alg = self.createChildAlgorithm('ApplyCalibration')
        alg.setProperty('Workspace', wks)
        alg.setProperty('PositionTable', detPos)
        alg.execute()

    def _convertToDistr(self, wks):
        """
        Convert workspace to distribution

        @param wks :: workspace, which is modified/converted in place
    	"""
        alg = self.createChildAlgorithm('ConvertToDistribution')
        alg.setProperty('Workspace', wks)
        alg.execute()

AlgorithmFactory.subscribe(EnggFocus)
