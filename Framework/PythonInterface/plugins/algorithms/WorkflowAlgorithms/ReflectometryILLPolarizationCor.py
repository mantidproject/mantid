# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import ILL_utilities as utils
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    MatrixWorkspace,
    mtd,
    WorkspaceGroup,
    WorkspaceGroupProperty,
)
from mantid.kernel import (
    CompositeValidator,
    Direction,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    StringListValidator,
)
from mantid.simpleapi import LoadILLPolarizationFactors, PolarizationEfficiencyCor, RebinToWorkspace
from typing import List


class Prop:
    CLEANUP = "Cleanup"
    EFFICIENCY_FILE = "EfficiencyFile"
    INPUT_WS = "InputWorkspaces"
    OUTPUT_WS = "OutputWorkspace"
    SUBALG_LOGGING = "SubalgorithmLogging"


class SubalgLogging:
    OFF = "Logging OFF"
    ON = "Logging ON"


class ReflectometryILLPolarizationCor(DataProcessorAlgorithm):
    def category(self):
        """Returns algorithm's categories."""
        return "ILL\\Reflectometry;Workflow\\Reflectometry"

    def name(self):
        """Returns the name of the algorithm."""
        return "ReflectometryILLPolarizationCor"

    def summary(self):
        """Returns a summary of the algorithm."""
        return "Performs polarization efficiency corrections for reflectometry instruments at ILL."

    def seeAlso(self):
        """Returns a list of related algorithm names."""
        return ["ReflectometryILLConvertToQ", "ReflectometryILLPreprocess", "ReflectometryILLSumForeground", "ReflectometryILLAutoProcess"]

    def version(self):
        """Returns the version of the algorithm."""
        return 1

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        mandatory_input_workspaces = CompositeValidator()
        mandatory_input_workspaces.add(StringArrayMandatoryValidator())
        mandatory_input_workspaces.add(StringArrayLengthValidator(1, 4))

        self.declareProperty(
            StringArrayProperty(Prop.INPUT_WS, values=[], validator=mandatory_input_workspaces),
            doc="A set of polarized workspaces, in wavelength.",
        )

        self.declareProperty(
            WorkspaceGroupProperty(Prop.OUTPUT_WS, defaultValue="", direction=Direction.Output),
            doc="A group of polarization efficiency corrected workspaces.",
        )

        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc="Enable or disable child algorithm logging.",
        )

        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=utils.Cleanup.ON,
            validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
            doc="Enable or disable intermediate workspace cleanup.",
        )

        self.declareProperty(
            FileProperty(Prop.EFFICIENCY_FILE, defaultValue="", action=FileAction.Load),
            doc="A file containing the polarization efficiency factors.",
        )

    def PyExec(self):
        """Execute the algorithm."""
        self._subalg_logging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanup_mode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanup_mode, self._subalg_logging)
        ws_prefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(ws_prefix, cleanup_mode)

        ws = self._input_ws()

        eff_ws = self._efficiencies(ws[0])

        ws = self._common_binning(ws)

        ws = self._correct(ws, eff_ws)

        self._finalize(ws)

    def _common_binning(self, wss: List[str]) -> List[str]:
        """Rebins all workspaces in wss list to the first one.

        Keyword arguments:
        wss -- string array with workspace names to be rebinned
        """
        for i in range(1, len(wss)):
            RebinToWorkspace(WorkspaceToRebin=wss[i], OutputWorkspace=wss[i], WorkspaceToMatch=wss[0], EnableLogging=self._subalg_logging)
        return wss

    def _correct(self, wss: List[str], eff_ws: MatrixWorkspace) -> WorkspaceGroup:
        """Returns a workspace group containing the polarization efficiency corrected workspaces.

        Keyword arguments:
        wss -- string array with workspace names to be corrected
        eff_ws -- workspace containing the efficiency factors
        """
        flippers = self._flipper_configuration(wss)
        corr_wss_name = self.getPropertyValue(Prop.OUTPUT_WS)
        corr_wss = PolarizationEfficiencyCor(
            InputWorkspaces=wss, OutputWorkspace=corr_wss_name, Flippers=flippers, Efficiencies=eff_ws, EnableLogging=self._subalg_logging
        )
        for ws in wss:
            self._cleanup.cleanup(ws)
        return corr_wss

    def _efficiencies(self, ref_ws: MatrixWorkspace) -> MatrixWorkspace:
        """Loads the polarization efficiencies, return efficiency factor workspace.

        Keyword arguments:
        ref_ws -- workspace used as wavelength reference
        """
        filename = self.getProperty(Prop.EFFICIENCY_FILE).value
        eff_ws_name = self._names.withSuffix("efficiencies")
        eff_ws = LoadILLPolarizationFactors(
            Filename=filename, OutputWorkspace=eff_ws_name, WavelengthReference=ref_ws, EnableLogging=self._subalg_logging
        )
        self._cleanup.cleanupLater(eff_ws)
        return eff_ws

    def _finalize(self, ws: WorkspaceGroup) -> None:
        """Sets ws workspace to the OutputWorkspace and clean up.

        Keyword arguments:
        ws -- workspace group containing the output
        """
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.finalCleanup()

    def _flipper_configuration(self, wss: List[str]) -> str:
        """Returns flipper configuration string and reorder wss for PolarizationEfficiencyCor compatibility.

        Keyword arguments:
        wss -- string array with workspace names to be reordered
        """
        if len(wss) == 1:
            # It's direct beam.
            self.log().notice("Performing direct beam polarization corrections.")
            return "0"
        if len(wss) == 2:
            # Analyzer or no analyzer?
            is_analyzer = list()
            flippers = list()
            for ws in wss:
                run = mtd[ws].run()
                analyzer_translation = run.getProperty("tra.value").value
                if analyzer_translation < 0.0 or analyzer_translation >= 200.0:
                    is_analyzer.append(True)
                else:
                    is_analyzer.append(False)
                flippers.append(int(run.getProperty("fl1.value").value))
            if is_analyzer[0] != is_analyzer[1]:
                raise RuntimeError("Analyzer config mismatch: one of the input workspaces has analyzer on, the other off.")
            is_analyzer = is_analyzer[0]
            if flippers[0] == 1:
                # Reorder workspaces as expected by PolarizationEfficiencyCor.
                wss[1], wss[0] = wss[0], wss[1]
            if is_analyzer:
                self.log().notice("Performing analyzerless polarization corrections.")
                return "0, 1"
            else:
                self.log().notice("Performing polarization corrections with missing 01 and 10 intensities.")
                return "00, 11"
        if len(wss) == 3:
            # Missing 01 or 10 flipper configuration?
            flippers = dict()
            for ws in wss:
                run = mtd[ws].run()
                flipper1 = int(run.getProperty("fl1.value").value)
                flipper2 = int(run.getProperty("fl2.value").value)
                flippers[(flipper1, flipper2)] = ws
            # Reorder workspaces as expected by PolarizationEfficiencyCor.
            present_flipper = (0, 1) if (0, 1) in flippers.keys() else (1, 0)
            missing_flipper = (0, 1) if (0, 1) not in flippers.keys() else (1, 0)
            wss[0] = flippers[0, 0]
            wss[1] = flippers[present_flipper]
            wss[2] = flippers[1, 1]
            if missing_flipper == (0, 1):
                self.log().notice("Performing polarization corrections with missing 01 intensity.")
                return "00, 10, 11"
            else:
                self.log().notice("Performing polarization corrections with missing 10 intensity.")
                return "00, 01, 11"
        # Full corrections.
        self.log().notice("Performing full polarization corrections.")
        flippers = dict()
        for ws in wss:
            run = mtd[ws].run()
            flipper1 = int(run.getProperty("fl1.value").value)
            flipper2 = int(run.getProperty("fl2.value").value)
            flippers[(flipper1, flipper2)] = ws
            self.log().information(ws + " flipper state found: {0}, {1}".format(str(flipper1), str(flipper2)))
        wss[0] = flippers[0, 0]
        wss[1] = flippers[0, 1]
        wss[2] = flippers[1, 0]
        wss[3] = flippers[1, 1]
        return "00, 01, 10, 11"

    def _input_ws(self) -> List[str]:
        """Returns an array containing the input workspaces."""
        wss = self.getProperty(Prop.INPUT_WS).value
        for ws in wss:
            self._cleanup.protect(ws)
        return wss


AlgorithmFactory.subscribe(ReflectometryILLPolarizationCor)
