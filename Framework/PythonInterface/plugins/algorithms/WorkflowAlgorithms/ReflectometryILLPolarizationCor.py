# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import ILL_utilities as utils
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, mtd, WorkspaceGroupProperty)
from mantid.kernel import (CompositeValidator, Direction, StringArrayLengthValidator, StringArrayMandatoryValidator, StringArrayProperty,
                           StringListValidator)
from mantid.simpleapi import (LoadILLPolarizationFactors, PolarizationEfficiencyCor, RebinToWorkspace)


class Prop:
    CLEANUP = 'Cleanup'
    EFFICIENCY_FILE = 'EfficiencyFile'
    INPUT_WS = 'InputWorkspaces'
    OUTPUT_WS = 'OutputWorkspace'
    SUBALG_LOGGING = 'SubalgorithmLogging'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class ReflectometryILLPolarizationCor(DataProcessorAlgorithm):

    def category(self):
        """Return algorithm's categories."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLPolarizationCor'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Performs polarization efficiency corrections for reflectometry instruments at ILL.'

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryILLConvertToQ', 'ReflectometryILLPreprocess', 'ReflectometryILLSumForeground']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(wsPrefix, cleanupMode)

        wss = self._inputWS()

        effWS = self._efficiencies(wss[0])

        wss = self._commonBinning(wss)

        wss = self._correct(wss, effWS)

        self._finalize(wss)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        mandatoryInputWorkspaces = CompositeValidator()
        mandatoryInputWorkspaces.add(StringArrayMandatoryValidator())
        mandatoryInputWorkspaces.add(StringArrayLengthValidator(1, 4))
        self.declareProperty(StringArrayProperty(Prop.INPUT_WS,
                                                 values=[],
                                                 validator=mandatoryInputWorkspaces),
                             doc='A set of polarized workspaces, in wavelength.')
        self.declareProperty(WorkspaceGroupProperty(Prop.OUTPUT_WS,
                                                    defaultValue='',
                                                    direction=Direction.Output),
                             doc='A group of polarization efficiency corrected workspaces.')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disable child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=utils.Cleanup.ON,
                             validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(FileProperty(Prop.EFFICIENCY_FILE,
                                          defaultValue='',
                                          action=FileAction.Load),
                             doc='A file containing the polarization efficiency factors.')

    def _commonBinning(self, wss):
        """Rebin all workspaces in wss to the first one."""
        rebinnedWSs = [wss[0]]
        for i in range(1, len(wss)):
            rebinnedWSName = self._names.withSuffix('rebinned_to_' + str(wss[0]))
            RebinToWorkspace(
                WorkspaceToRebin=wss[i],
                OutputWorkspace=rebinnedWSName,
                WorkspaceToMatch=wss[0],
                EnableLogging=self._subalgLogging
            )
            self._cleanup.cleanup(wss[i])
            rebinnedWSs.append(rebinnedWSName)
        return rebinnedWSs

    def _correct(self, wss, effWS):
        """Return a workspace group containing the polarization efficiency corrected workspaces."""
        flippers = self._flipperConfiguration(wss)
        corrWSsName = self.getPropertyValue(Prop.OUTPUT_WS)
        corrWSs = PolarizationEfficiencyCor(
            InputWorkspaces=wss,
            OutputWorkspace=corrWSsName,
            Flippers=flippers,
            Efficiencies=effWS,
            EnableLogging=self._subalgLogging
        )
        for ws in wss:
            self._cleanup.cleanup(ws)
        return corrWSs

    def _efficiencies(self, refWS):
        """Load the polarization efficiencies, return efficiency factor workspace."""
        filename = self.getProperty(Prop.EFFICIENCY_FILE).value
        effWSName = self._names.withSuffix('efficiencies')
        effWS = LoadILLPolarizationFactors(
            Filename=filename,
            OutputWorkspace=effWSName,
            WavelengthReference=refWS,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanupLater(effWS)
        return effWS

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.finalCleanup()

    def _flipperConfiguration(self, wss):
        """Return flipper configuration string and reorder wss for PolarizationEfficiencyCor compatibility."""
        if len(wss) == 1:
            # It's direct beam.
            self.log().notice('Performing direct beam polarization corrections.')
            return '0'
        if len(wss) == 2:
            # Analyzer or no analyzer?
            isAnalyzer = list()
            flippers = list()
            for ws in wss:
                run = mtd[ws].run()
                analyzerTranslation = run.getProperty('tra.value').value
                if analyzerTranslation < 0. or analyzerTranslation >= 200.:
                    isAnalyzer.append(True)
                else:
                    isAnalyzer.append(False)
                flippers.append(run.getProperty('Flipper1.stateint').value)
            if isAnalyzer[0] != isAnalyzer[1]:
                raise RuntimeError('Analyzer config mismatch: one of the input workspaces has analyzer on, the other off.')
            isAnalyzer = isAnalyzer[0]
            if flippers[0] == 1:
                # Reorder workspaces as expected by PolarizationEfficiencyCor.
                wss[1], wss[0] = wss[0], wss[1]
            if isAnalyzer:
                self.log().notice('Performing analyzerless polarization corrections.')
                return '0, 1'
            else:
                self.log().notice('Performing polarization corrections with missing 01 and 10 intensities.')
                return '00, 11'
        if len(wss) == 3:
            # Missing 01 or 10 flipper configuration?
            flippers = dict()
            for ws in wss:
                run = mtd[ws].run()
                flipper1 = run.getProperty('Flipper1.stateint').value
                flipper2 = run.getProperty('Flipper2.stateint').value
                flippers[(flipper1, flipper2)] = ws
            # Reorder workspaces as expected by PolarizationEfficiencyCor.
            presentFlipper = (0, 1) if (0, 1) in flippers.keys() else (1, 0)
            missingFlipper = (0, 1) if (0, 1) not in flippers.keys() else (1, 0)
            wss[0] = flippers[0, 0]
            wss[1] = flippers[presentFlipper]
            wss[2] = flippers[1, 1]
            if missingFlipper == (0, 1):
                self.log().notice('Performing polarization corrections with missin 01 intensity.')
                return '00, 10, 11'
            else:
                self.log().notice('Performing polarization corrections with missin 10 intensity.')
                return '00, 01, 11'
        # Full corrections.
        self.log().notice('Performing full polarization corrections.')
        flippers = dict()
        for ws in wss:
            run = mtd[ws].run()
            flipper1 = run.getProperty('Flipper1.stateint').value
            flipper2 = run.getProperty('Flipper2.stateint').value
            flippers[(flipper1, flipper2)] = ws
        wss[0] = flippers[0, 0]
        wss[1] = flippers[0, 1]
        wss[2] = flippers[1, 0]
        wss[3] = flippers[1, 1]
        return '00, 01, 10, 11'

    def _inputWS(self):
        """Return the input workspace."""
        wss = self.getProperty(Prop.INPUT_WS).value
        for ws in wss:
            self._cleanup.protect(ws)
        return wss


AlgorithmFactory.subscribe(ReflectometryILLPolarizationCor)
