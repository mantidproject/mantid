#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
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

        self.declareProperty('NormaliseByCurrent', True, direction=Direction.Input,
                             doc='Normalize the input data by applying the NormaliseByCurrent algorithm '
                             'which use the log entry gd_proton_charge')

        self.declareProperty(FloatArrayProperty('MaskBinsXMins', EnggUtils.ENGINX_MASK_BIN_MINS,
                                                direction=Direction.Input),
                             doc="List of minimum bin values to mask, separated by commas.")

        self.declareProperty(FloatArrayProperty('MaskBinsXMaxs', EnggUtils.ENGINX_MASK_BIN_MAXS,
                                                direction=Direction.Input),
                             doc="List of maximum bin values to mask, separated by commas.")

        prep_grp = 'Data preparation/pre-processing'
        self.setPropertyGroup('NormaliseByCurrent', prep_grp)
        self.setPropertyGroup('MaskBinsXMins', prep_grp)
        self.setPropertyGroup('MaskBinsXMaxs', prep_grp)

    def validateInputs(self):
        issues = dict()

        if not self.getPropertyValue('MaskBinsXMins') and self.getPropertyValue('MaskBinsXMaxs') or\
           self.getPropertyValue('MaskBinsXMins') and not self.getPropertyValue('MaskBinsXMaxs'):
            issues['MaskBinsXMins'] = "Both minimum and maximum values need to be given, or none"

        min_list = self.getProperty('MaskBinsXMins').value
        max_list = self.getProperty('MaskBinsXMaxs').value
        if len(min_list) > 0 and len(max_list) > 0:
            len_min = len(min_list)
            len_max = len(max_list)
            if len_min != len_max:
                issues['MaskBinsXMins'] = ("The number of minimum and maximum values must match. Got "
                                           "{0} and {1} for the minimum and maximum, respectively"
                                           .format(len_min, len_max))

        return issues

    def PyExec(self):
        # Get the run workspace
        wks = self.getProperty('InputWorkspace').value

        # Get spectra indices either from bank or direct list of indices, checking for errors
        bank = self.getProperty('Bank').value
        spectra = self.getProperty(self.INDICES_PROP_NAME).value
        indices = EnggUtils.getWsIndicesFromInProperties(wks, bank, spectra)

        detPos = self.getProperty("DetectorPositions").value
        nreports = 5
        if detPos:
            nreports += 1
        prog = Progress(self, start=0, end=1, nreports=nreports)

        # Leave only the data for the bank/spectra list requested
        prog.report('Selecting spectra from input workspace')
        wks = EnggUtils.cropData(self, wks, indices)

        prog.report('Masking some bins if requested')
        self._mask_bins(wks, self.getProperty('MaskBinsXMins').value, self.getProperty('MaskBinsXMaxs').value)

        prog.report('Preparing input workspace with vanadium corrections')
        # Leave data for the same bank in the vanadium workspace too
        vanWS = self.getProperty('VanadiumWorkspace').value
        vanIntegWS = self.getProperty('VanIntegrationWorkspace').value
        vanCurvesWS = self.getProperty('VanCurvesWorkspace').value
        EnggUtils.applyVanadiumCorrections(self, wks, indices, vanWS, vanIntegWS, vanCurvesWS)

        # Apply calibration
        if detPos:
            self._applyCalibration(wks, detPos)

        # Convert to dSpacing
        wks = EnggUtils.convertToDSpacing(self, wks)

        prog.report('Summing spectra')
        # Sum the values across spectra
        wks = EnggUtils.sumSpectra(self, wks)

        prog.report('Preparing output workspace')
        # Convert back to time of flight
        wks = EnggUtils.convertToToF(self, wks)

        prog.report('Normalizing input workspace if needed')
        if self.getProperty('NormaliseByCurrent').value:
            self._normalize_by_current(wks)

        # OpenGenie displays distributions instead of pure counts (this is done implicitly when
        # converting units), so I guess that's what users will expect
        self._convertToDistr(wks)

        self.setProperty("OutputWorkspace", wks)

    def _mask_bins(self, wks, min_bins, max_bins):
        """
        Mask multiple ranges of bins, given multiple pairs min-max

        @param wks :: workspace that will be masked (in/out, masked in place)
        @param min_bins :: list of minimum values for every range to mask
        @param max_bins :: list of maxima
        """
        for min_x, max_x in zip(min_bins, max_bins):
            alg = self.createChildAlgorithm('MaskBins')
            alg.setProperty('InputWorkspace', wks)
            alg.setProperty('OutputWorkspace', wks)
            alg.setProperty('XMin', min_x)
            alg.setProperty('XMax', max_x)
            alg.execute()

    def _normalize_by_current(self, wks):
        """
        Apply the normalize by current algorithm on a workspace

        @param wks :: workspace (in/out, modified in place)
        """
        p_charge = wks.getRun().getProtonCharge()
        if p_charge <= 0:
            self.log().warning("Cannot normalize by current because the proton charge log value "
                               "is not positive!")

        self.log().notice("Normalizing by current with proton charge: {0} uamp".
                          format(p_charge))

        alg = self.createChildAlgorithm('NormaliseByCurrent')
        alg.setProperty('InputWorkspace', wks)
        alg.setProperty('OutputWorkspace', wks)
        alg.execute()

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
