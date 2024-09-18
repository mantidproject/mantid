# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,non-parent-init-called,too-few-public-methods
# non-parent-init-called is disabled to remove false positives from a bug in pyLint < 1.4
from abc import ABCMeta, abstractmethod
import systemtesting
import mantid.simpleapi as ms
from mantid import mtd


class ISISIndirectDiffractionReduction(metaclass=ABCMeta):
    """
    Base class for tests that use the ISISIndirectDiffractionReduction algorithm.
    """

    _output_workspace = None

    @abstractmethod
    def get_reference_file(self):
        """
        Gets reference result file for workspace comparison.
        """
        raise NotImplementedError()

    def runTest(self):
        """
        Runs an ISISIndirectDiffractionReduction with the configured parameters.
        """
        ms.ISISIndirectDiffractionReduction(
            InputFiles=self.raw_file,
            OutputWorkspace=self.output_workspace_group,
            Instrument=self.instrument,
            Mode=self.mode,
            SpectraRange=self.spectra_range,
            RebinParam=self.rebinning,
        )

        self._output_workspace = mtd[self.output_workspace_group].getNames()[0]

    def validate(self):
        """
        Validates the result workspace with the reference file.
        """
        self.disableChecking.append("Instrument")
        return self._output_workspace, self.get_reference_file()


# -------------------------------------------------------------------------------


class IRISDiffspecDiffractionTest(ISISIndirectDiffractionReduction):
    def __init__(self):
        ISISIndirectDiffractionReduction.__init__(self)

        self.instrument = "IRIS"
        self.mode = "diffspec"
        self.raw_file = "IRS21360.raw"
        self.spectra_range = [105, 112]
        self.rebinning = "3.0,0.001,4.0"
        self.output_workspace_group = "IRIS_Diffraction_DiffSpec_Test"

    def get_reference_file(self):
        return "IRISDiffspecDiffractionTest.nxs"


# -------------------------------------------------------------------------------


class TOSCADiffractionTest(ISISIndirectDiffractionReduction):
    def __init__(self):
        ISISIndirectDiffractionReduction.__init__(self)

        self.instrument = "TOSCA"
        self.mode = "diffspec"
        self.raw_file = "TSC11453.raw"
        self.spectra_range = [146, 149]
        self.rebinning = "0.5,0.001,2.1"
        self.output_workspace_group = "TOSCA_Diffraction_DiffSpec_Test"

    def get_reference_file(self):
        return "TOSCADiffractionTest.nxs"


# -------------------------------------------------------------------------------


class OSIRISDiffspecDiffractionTest(ISISIndirectDiffractionReduction):
    def __init__(self):
        ISISIndirectDiffractionReduction.__init__(self)

        self.instrument = "OSIRIS"
        self.mode = "diffspec"
        self.raw_file = "osiris00101300.raw"
        self.spectra_range = [3, 962]
        self.rebinning = "2.0,0.001,3.0"
        self.output_workspace_group = "OSIRIS_Diffraction_DiffSpec_Test"

    def get_reference_file(self):
        return "OsirisDiffspecDiffractionTest.nxs"


# -------------------------------------------------------------------------------


class OSIRISDiffonlyDiffractionTest(systemtesting.MantidSystemTest):
    def runTest(self):
        ms.OSIRISDiffractionReduction(
            OutputWorkspace="OsirisDiffractionTest",
            Sample="OSI89813.raw, OSI89814.raw, OSI89815.raw, OSI89816.raw, OSI89817.raw",
            CalFile="osiris_041_RES10.cal",
            Vanadium="OSI89757, OSI89758, OSI89759, OSI89760, OSI89761",
            GroupingMethod="File",
            GroupingFile="osiris_041_RES10.cal",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return "OsirisDiffractionTest", "OsirisDiffractionTest.nxs"
