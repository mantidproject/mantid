# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILLReduction_common as common

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        MatrixWorkspaceProperty, PropertyMode, WorkspaceProperty,
                        WorkspaceUnitValidator)
from mantid.kernel import CompositeValidator, Direction, FloatArrayProperty, StringListValidator

class DirectILLSofQW(DataProcessorAlgorithm):
    """An algorithm to transform S(2theta,E) to S(q,E)."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLSofQW'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        subalgLogging = False
        if self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON:
            subalgLogging = True
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        mainWS = self._sOfQW(mainWS, wsNames, wsCleanup, subalgLogging)
        mainWS = self._transpose(mainWS, wsNames, wsCleanup, subalgLogging)

        self._finalize(mainWS, wsCleanup)

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('DeltaE'))

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output of the algorithm.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=common.CLEANUP_ON,
                             validator=StringListValidator([
                                 common.CLEANUP_ON,
                                 common.CLEANUP_OFF]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=common.PROP_SUBALG_LOGGING,
                             defaultValue=common.SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 common.SUBALG_LOGGING_OFF,
                                 common.SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to ' +
                                 'print in the logs.')
        self.declareProperty(name=common.PROP_BINNING_MODE_Q,
                             defaultValue=common.REBIN_AUTO_Q,
                             validator=StringListValidator([
                                 common.REBIN_AUTO_Q,
                                 common.REBIN_MANUAL_Q]),
                             direction=Direction.Input,
                             doc='q rebinning mode.')
        self.setPropertyGroup(common.PROP_BINNING_MODE_Q, common.PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=common.PROP_BINNING_PARAMS_Q),
                             doc='Manual q rebinning parameters.')
        self.setPropertyGroup(common.PROP_BINNING_PARAMS_Q, common.PROPGROUP_REBINNING)
        self.declareProperty(name=common.PROP_TRANSPOSE_SAMPLE_OUTPUT,
                             defaultValue=common.TRANSPOSING_ON,
                             validator=StringListValidator([
                                 common.TRANSPOSING_ON,
                                 common.TRANSPOSING_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable ' + common.PROP_OUTPUT_WS + ' transposing.')

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.finalCleanup()

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _sOfQW(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Run the SofQWNormalisedPolygon algorithm."""
        sOfQWWSName = wsNames.withSuffix('sofqw')
        qRebinningMode = self.getProperty(common.PROP_BINNING_MODE_Q).value
        if qRebinningMode == common.REBIN_AUTO_Q:
            qMin, qMax = _minMaxQ(mainWS)
            dq = _deltaQ(mainWS)
            qBinning = '{0}, {1}, {2}'.format(qMin, dq, qMax)
        else:
            qBinning = self.getProperty(common.PROP_BINNING_PARAMS_Q).value
        Ei = mainWS.run().getLogData('Ei').value
        sOfQWWS = SofQWNormalisedPolygon(InputWorkspace=mainWS,
                                         OutputWorkspace=sOfQWWSName,
                                         QAxisBinning=qBinning,
                                         EMode='Direct',
                                         EFixed=Ei,
                                         ReplaceNaNs=False,
                                         EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return sOfQWWS

    def _transpose(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Transpose the final output workspace."""
        transposing = self.getProperty(common.PROP_TRANSPOSE_SAMPLE_OUTPUT).value
        if transposing == common.TRANSPOSING_OFF:
            return mainWS
        pointDataWSName = wsNames.withSuffix('point_data_converted')
        pointDataWS = ConvertToPointData(InputWorkspace=mainWS,
                                         OutputWorkspace=pointDataWSName,
                                         EnableLogging=subalgLogging)
        transposedWSName = wsNames.withSuffix('transposed')
        transposedWS = Transpose(InputWorkspace=pointDataWS,
                                 OutputWorkspace=transposedWSName,
                                 EnableLogging=subalgLogging)
        wsCleanup.cleanup(pointDataWS)
        wsCleanup.cleanup(mainWS)
        return transposedWS


AlgorithmFactory.subscribe(DirectILLSofQW)
