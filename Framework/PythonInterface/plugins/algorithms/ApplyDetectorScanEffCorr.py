# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace, Transpose, Multiply, MaskDetectors
from mantid.api import AlgorithmFactory, PropertyMode, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction
import numpy as np


class ApplyDetectorScanEffCorr(PythonAlgorithm):
    def category(self):
        return "ILL\\Diffraction;Diffraction\\Utility"

    def seeAlso(self):
        return ["PowderILLEfficiency", "LoadILLDiffraction"]

    def name(self):
        return "ApplyDetectorScanEffCorr"

    def summary(self):
        return "Applies the calibration workspace generated by PowderILLEfficiency to data loaded with LoadILLDiffraction."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            "The workspace for the detector efficiency correction to be applied to.",
        )
        self.declareProperty(
            WorkspaceProperty("DetectorEfficiencyWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            "The workspace containing the detector efficiency correction factors generated by PowderDiffILLDetEffCorr.",
        )
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            "The output workspace with the calibrated data. Optionally can be the same as the input workspace.",
        )

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        eff_ws = self.getProperty("DetectorEfficiencyWorkspace").value
        transposed = Transpose(InputWorkspace=eff_ws, StoreInADS=False)
        efficiencies = transposed.extractY().flatten()
        errors = transposed.extractE().flatten()
        n_hist = input_ws.getNumberHistograms()
        if n_hist % efficiencies.size != 0:
            raise ValueError(
                "Number of histograms in input workspace is not a multiple of number of entries in detector efficiency workspace."
            )
        n_time_indexes = n_hist / efficiencies.size
        to_multiply = CreateWorkspace(
            DataY=np.repeat(efficiencies, n_time_indexes),
            DataE=np.repeat(errors, n_time_indexes),
            DataX=np.zeros(n_hist),
            NSpec=n_hist,
            StoreInADS=False,
        )
        output = Multiply(LHSWorkspace=input_ws, RHSWorkspace=to_multiply, OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        # In the output we should mask the detectors where calibration constant is masked
        det_IDs = ""
        n_pixels_per_tube = eff_ws.getNumberHistograms()
        for spectrum in range(n_pixels_per_tube):
            if eff_ws.hasMaskedBins(spectrum):
                masked = eff_ws.maskedBinsIndices(spectrum)
                for bin in masked:
                    det_IDs += str(bin * n_pixels_per_tube + spectrum + 1) + ","
        if det_IDs:
            MaskDetectors(Workspace=output, DetectorList=det_IDs[:-1])
        self.setProperty("OutputWorkspace", output)


AlgorithmFactory.subscribe(ApplyDetectorScanEffCorr)
