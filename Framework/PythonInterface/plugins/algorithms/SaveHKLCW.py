# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from mantid.api import AlgorithmFactory, PythonAlgorithm, FileAction, FileProperty, IPeaksWorkspaceProperty
from mantid.kernel import Direction, logger


class SaveHKLCW(PythonAlgorithm):
    def category(self):
        return "Diffraction\\DataHandling"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "Save a PeaksWorkspace to SHELX76 format required by WINGX"

    def name(self):
        return "SaveHKLCW"

    def seeAlso(self):
        return ["SaveHKL"]

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty("Workspace", "", direction=Direction.Input), doc="PeaksWorkspace to be saved")
        self.declareProperty(
            FileProperty("OutputFile", "", action=FileAction.Save, extensions=["hkl", "int"], direction=Direction.Input), doc="Output File."
        )
        self.declareProperty("Header", True, direction=Direction.Input, doc="If to include the header in the output")
        self.declareProperty("DirectionCosines", False, direction=Direction.Input, doc="If to include the direction cosines output")

    def PyExec(self):
        peak_ws = self.getProperty("Workspace").value
        outFile = self.getPropertyValue("OutputFile")
        header = self.getProperty("Header").value
        directionCosines = self.getProperty("DirectionCosines").value

        with open(outFile, "w") as f:
            if header:
                f.write("Single crystal data\n")
                if directionCosines:
                    f.write("(3i4,2f8.2,i4,6f8.5)\n")
                else:
                    f.write("(3i4,2f8.2,i4)\n")
                wavelengths = [p.getWavelength() for p in peak_ws]
                if np.std(wavelengths) > 0.01:
                    logger.warning("Large variation of wavelengths, this doesn't look like constant wavelength")
                wavelength = np.mean([p.getWavelength() for p in peak_ws])
                f.write(f"{wavelength:.5f}  0   0\n")

            if directionCosines:
                for p in peak_ws:
                    kr_n = p.getSourceDirectionSampleFrame()
                    kf_n = p.getDetectorDirectionSampleFrame()
                    reverse_incident_cos = peak_ws.sample().getOrientedLattice().cosFromDir(kr_n)
                    scattered_cos = peak_ws.sample().getOrientedLattice().cosFromDir(kf_n)
                    f.write(
                        "{:4.0f}{:4.0f}{:4.0f}{:8.2f}{:8.2f}{:4d}{:8.5f}{:8.5f}{:8.5f}{:8.5f}{:8.5f}{:8.5f}\n".format(
                            p.getH(),
                            p.getK(),
                            p.getL(),
                            p.getIntensity(),
                            p.getSigmaIntensity(),
                            1,
                            reverse_incident_cos[0],
                            scattered_cos[0],
                            reverse_incident_cos[1],
                            scattered_cos[1],
                            reverse_incident_cos[2],
                            scattered_cos[2],
                        )
                    )
            else:
                for p in peak_ws:
                    f.write(
                        "{:4.0f}{:4.0f}{:4.0f}{:8.2f}{:8.2f}{:4d}\n".format(
                            p.getH(), p.getK(), p.getL(), p.getIntensity(), p.getSigmaIntensity(), 1
                        )
                    )


AlgorithmFactory.subscribe(SaveHKLCW)
