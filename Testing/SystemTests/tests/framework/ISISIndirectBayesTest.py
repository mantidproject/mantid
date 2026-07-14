# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init, too-few-public-methods
from abc import ABCMeta, abstractmethod

from mantid.api import AnalysisDataService
from mantid.simpleapi import ExtractSingleSpectrum, Fit, LoadNexusProcessed, Scale
import systemtesting


class JumpFitFunctionTestBase(systemtesting.MantidSystemTest, metaclass=ABCMeta):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)

        self._sample_name = "irs26176_graphite002_QLr_Workspace"
        self._q_range = [0.6, 1.705600]
        self._width_index = 2

        self._function = ""

    @abstractmethod
    def get_reference_files(self):
        """Returns the name of the reference files to compare against."""
        raise NotImplementedError("Implmenent get_reference_files to return the names of the files to compare against.")

    def runTest(self):
        # Load file
        filename = self._sample_name + ".nxs"
        LoadNexusProcessed(Filename=filename, OutputWorkspace=self._sample_name)

        # Extract the width spectrum
        ExtractSingleSpectrum(InputWorkspace=self._sample_name, OutputWorkspace=self._sample_name, WorkspaceIndex=self._width_index)

        # Data must be in HWHM
        Scale(InputWorkspace=self._sample_name, OutputWorkspace=self._sample_name, Factor=0.5)

        Fit(
            InputWorkspace=self._sample_name,
            Function=self._function,
            StartX=self._q_range[0],
            EndX=self._q_range[1],
            CreateOutput=True,
            Output=self._sample_name,
        )

    def validate(self):
        return self._sample_name + "_Workspace", self.get_reference_files()

    def cleanup(self):
        AnalysisDataService.clear()


# ==============================================================================


class JumpCETest(JumpFitFunctionTestBase):
    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = "name=ChudleyElliot,Tau=1.42,L=2.42"
        self.tolerance = 5e-3

    def get_reference_files(self):
        return "ISISIndirectBayes_JumpCETest.nxs"


# ==============================================================================


class JumpHallRossTest(JumpFitFunctionTestBase):
    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = "name=HallRoss,Tau=2.7,L=1.75"
        self.tolerance = 5e-3

    def get_reference_files(self):
        return "ISISIndirectBayes_JumpHallRossTest.nxs"


# ==============================================================================


class JumpFickTest(JumpFitFunctionTestBase):
    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = "name=FickDiffusion,D=0.07"
        self.tolerance = 5e-4

    def get_reference_files(self):
        return "ISISIndirectBayes_JumpFickTest.nxs"


# ==============================================================================


class JumpTeixeiraTest(JumpFitFunctionTestBase):
    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = "name=TeixeiraWater,Tau=1.6,L=0.9"
        self.tolerance = 1e-3

    def get_reference_files(self):
        return "ISISIndirectBayes_JumpTeixeiraTest.nxs"


# ==============================================================================
