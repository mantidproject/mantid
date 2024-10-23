# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, MatrixWorkspaceProperty
from mantid.kernel import Direction
from mantid.simpleapi import LoadAscii
import math


class PearlMCAbsorption(PythonAlgorithm):
    def category(self):
        return "CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Loads pre-calculated or measured absorption correction files for Pearl."

    def seeAlso(self):
        return ["MonteCarloAbsorption", "MayersSampleCorrection", "CarpenterSampleCorrection", "VesuvioCalculateMS"]

    def PyInit(self):
        # Input file
        self.declareProperty(FileProperty("Filename", "", FileAction.Load, [".out", ".dat"]), doc="The name of the input file.")
        # Output workspace
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The name of the input file.")

    def PyExec(self):
        filename = self.getProperty("Filename").value
        thickness = self._parseHeader(filename)
        # If we have a thickness value then this is a file containing measured mu(t) values
        # and the units are wavelength. If not then they are calculated and the units are dspacing
        if thickness is not None:
            x_unit = "Wavelength"
        else:
            x_unit = "dSpacing"

        wkspace_name = self.getPropertyValue("OutputWorkspace")
        # Load the file
        ascii_wkspace = LoadAscii(Filename=filename, OutputWorkspace=wkspace_name, Separator="Space", Unit=x_unit)
        if thickness is None:
            coeffs = ascii_wkspace
        else:
            coeffs = self._calculateAbsorption(ascii_wkspace, float(thickness))

        coeffs.setYUnitLabel("Attenuation Factor (I/I0)")
        coeffs.setYUnit("")
        coeffs.setDistribution(True)
        self.setProperty("OutputWorkspace", coeffs)

    def _parseHeader(self, filename):
        """Parses the header in the file.
        If the first line contains t= then this is assumed to be measured file else
        calculated is assumed.
        """
        # Parse some header information to test whether this is a measured or calculated file
        input_file = open(filename, "r")
        tvalue = None
        for line in input_file:
            sep = "t="
            if sep not in line:
                sep = "t ="
                if sep not in line:
                    sep = None
            if sep is not None:
                tvalue = line.split(sep)[1]
                tvalue = tvalue.split("mm")[0]
                break
        return tvalue

    def _calculateAbsorption(self, input_ws, thickness):
        """
        Calculate the I/I_0 for the given set of mu_values, i.e.
        (I/I_0) = exp(-mu*t)
        """
        # c = math.exp(-1.0*mu*thickness)
        num_hist = input_ws.getNumberHistograms()
        num_vals = input_ws.blocksize()
        for i in range(num_hist):
            mu_values = input_ws.readY(i)
            for j in range(num_vals):
                input_ws.dataY(i)[j] = math.exp(-1.0 * mu_values[j] * thickness)

        return input_ws


#############################################################################################
AlgorithmFactory.subscribe(PearlMCAbsorption)
