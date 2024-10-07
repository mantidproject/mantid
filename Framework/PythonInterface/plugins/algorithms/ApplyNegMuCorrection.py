# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import *  # PythonAlgorithm, registerAlgorithm, WorkspaceProperty
from mantid.simpleapi import *
from mantid.kernel import *

# pylint: disable=no-init, too-many-arguments


class ApplyNegMuCorrection(PythonAlgorithm):
    # Combining work spaces and normalising the correction.
    # _OILE and _OIHE are unused parameters for now to make PyLint happy
    def combine(self, dataDir, runno, A2000, B2000, A3000, B3000, _OILE, _OIHE, spec):
        if spec < 10:
            specNo = "0" + str(spec)
        else:
            specNo = str(spec)
        print(dataDir + "ral0" + runno + ".rooth30" + specNo + ".dat")
        # loading data
        rooth30_filename = dataDir + r"\ral0" + runno + ".rooth30" + specNo + ".dat"
        rooth20_filename = dataDir + r"\ral0" + runno + ".rooth20" + specNo + ".dat"
        try:
            ws3000 = Load(Filename=rooth30_filename, OutputWorkspace="ws3000")
        except RuntimeError:
            raise RuntimeError("could not find file: " + rooth30_filename)
        try:
            ws2000 = Load(Filename=rooth20_filename, OutputWorkspace="ws2000")
        except RuntimeError:
            raise RuntimeError("could not find file: " + rooth20_filename)

        # Correcting for Gain and offset of the detectors
        ws2000_corr = CreateWorkspace(A2000 * ws2000.readX(0)[:] + B2000, ws2000.readY(0)[:])
        ws3000_corr = CreateWorkspace(A3000 * ws3000.readX(0)[:] + B3000, ws3000.readY(0)[:])

        # Summing total counts for normalisation
        ws2000_total = 0
        ws3000_total = 0
        for count in range(0, 8000):
            ws2000_total = ws2000_corr.readY(0)[count] + ws2000_total
            ws3000_total = ws3000_corr.readY(0)[count] + ws3000_total
        print(ws2000_total)
        print(ws3000_total)
        # normalising
        ws2000_corr = ws2000_corr / ws2000_total
        ws3000_corr = ws3000_corr / ws3000_total

        # rebinning to adataDir detectors together
        data = [100, ws2000.readX(0)[2] - ws2000.readX(0)[1], 8000]

        ws2000_corr_rebin = Rebin(ws2000_corr, data)
        ws3000_corr_rebin = Rebin(ws3000_corr, data)

        ws_ral = Plus(ws2000_corr_rebin, ws3000_corr_rebin)

        suf = "_" + str(spec) + "_" + runno

        RenameWorkspaces(ws_ral, Suffix=suf)
        RenameWorkspaces(ws2000_corr, Suffix=suf)
        RenameWorkspaces(ws3000_corr, Suffix=suf)

        DeleteWorkspace(ws2000)
        DeleteWorkspace(ws3000)
        DeleteWorkspace(ws2000_corr_rebin)
        DeleteWorkspace(ws3000_corr_rebin)

    def PyInit(self):
        self.declareProperty(
            FileProperty(name="DataDirectory", defaultValue=r"", action=FileAction.OptionalDirectory), doc="Data directory"
        )
        self.declareProperty(name="FirstRunNumber", defaultValue=1718, doc="First Run Number")
        self.declareProperty(name="LastRunNumber", defaultValue=1719, doc="Last Run Number")
        self.declareProperty(name="GainRIKENHighE", defaultValue=1.077, doc="Gain RIKEN High E")
        self.declareProperty(name="OffsetRIKENHighE", defaultValue=-1, doc="OffSet RIKEN High E")
        self.declareProperty(name="GainISISHighE", defaultValue=1.278, doc="Gain ISIS High E")
        self.declareProperty(name="OffsetISISHighE", defaultValue=-12, doc="OffSet ISIS High E")
        self.declareProperty(name="GainISISLowE", defaultValue=1.2, doc="Gain ISIS Low E")
        self.declareProperty(name="OffsetISISLowE", defaultValue=0.0, doc="OffSet ISIS Low E")

    def category(self):
        return "CorrectionFunctions\\SpecialCorrections;Muon"

    def PyExec(self):
        spec = 1
        dataDir = self.getProperty("DataDirectory").value
        first = self.getProperty("FirstRunNumber").value
        last = self.getProperty("LastRunNumber").value + 1
        GRHE = self.getProperty("GainRIKENHighE").value
        ORHE = self.getProperty("OffsetRIKENHighE").value
        GIHE = self.getProperty("GainISISHighE").value
        OIHE = self.getProperty("OffsetISISHighE").value
        GILE = self.getProperty("GainISISLowE").value
        OILE = self.getProperty("OffsetISISLowE").value

        for run in range(first, last):
            for spec in range(0, 3):
                runno = str(run)
                self.combine(dataDir, runno, GRHE, ORHE, GIHE, OIHE, GILE, OILE, spec)
            self.combine(dataDir, runno, GRHE, ORHE, GIHE, OIHE, GILE, OILE, 10)


AlgorithmFactory.subscribe(ApplyNegMuCorrection)
