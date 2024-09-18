# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, Progress
from mantid.kernel import Direction


class HFIRSANS2Wavelength(PythonAlgorithm):
    def category(self):
        return "SANS\\Wavelength"

    def seeAlso(self):
        return ["LoadEventAsWorkspace2D"]

    def summary(self):
        return "Convert the fake time of flight event workspace into a Workspace2D with units of wavelength"

    def PyInit(self):
        # Workspace which is to be masked
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The workspace which is to be converted to wavelength",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="The output workspace.",
        )

    def PyExec(self):
        inWS = self.getProperty("InputWorkspace").value
        runObj = inWS.getRun()
        try:
            wavelength = runObj["wavelength"].getStatistics().mean
            wavelength_spread = runObj["wavelength_spread"].getStatistics().mean
            wavelength_spread *= wavelength
        except:
            raise ValueError("Could not read wavelength and wavelength_spread logs from the workspace")
        progress = Progress(self, 0.0, 1.0, 4)

        # rebin into a single bin
        progress.report("Rebin")
        rebin_alg = self.createChildAlgorithm("Rebin", enableLogging=False)
        rebin_alg.setProperty("InputWorkspace", inWS)
        rebin_alg.setProperty("Params", "-20000,40000,20000")
        rebin_alg.setProperty("PreserveEvents", False)
        rebin_alg.execute()
        outWS = rebin_alg.getProperty("OutputWorkspace").value

        # scale to wavelength spread
        progress.report("Scale to spread")
        scale_alg = self.createChildAlgorithm("ScaleX", enableLogging=False)
        scale_alg.setProperty("InputWorkspace", outWS)
        scale_alg.setProperty("OutputWorkspace", outWS)
        scale_alg.setProperty("Factor", wavelength_spread / 40000.0)
        scale_alg.execute()
        outWS = scale_alg.getProperty("OutputWorkspace").value

        # shift to wavelength center
        progress.report("Shift to wavelength center")
        scale_alg = self.createChildAlgorithm("ScaleX", enableLogging=False)
        scale_alg.setProperty("InputWorkspace", outWS)
        scale_alg.setProperty("OutputWorkspace", outWS)
        scale_alg.setProperty("Factor", wavelength)
        scale_alg.setProperty("Operation", "Add")
        scale_alg.execute()
        outWS = scale_alg.getProperty("OutputWorkspace").value

        # change units
        progress.report("Set units")
        outWS.getAxis(0).setUnit("Wavelength")
        self.setProperty("OutputWorkspace", outWS)


AlgorithmFactory.subscribe(HFIRSANS2Wavelength)
