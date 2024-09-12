# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import math
from mantid.api import AlgorithmFactory, PythonAlgorithm, IPeaksWorkspaceProperty
from mantid.kernel import Direction, Property, V3D, FloatBoundedValidator, VisibleWhenProperty, PropertyCriterion
from mantid.geometry import Goniometer


class HFIRCalculateGoniometer(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Reduction"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "Calculate the goniometer for peak making an assumption for constant wavelength and goniometer rotation axes"

    def name(self):
        return "HFIRCalculateGoniometer"

    def seeAlso(self):
        return ["FindPeaksMD", "PredictPeaks", "SetGoniometer"]

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty("Workspace", "", direction=Direction.InOut), doc="Peaks Workspace to be modified")
        self.declareProperty(
            "Wavelength",
            Property.EMPTY_DBL,
            validator=FloatBoundedValidator(0.0),
            doc="Wavelength to set the workspace, will be the value set on workspace if not provided.",
        )
        self.declareProperty(
            "OverrideProperty",
            False,
            "If False then the value for InnerGoniometer and FlipX will be determiend from the workspace, "
            "it True then the properties will be used",
        )
        condition = VisibleWhenProperty("OverrideProperty", PropertyCriterion.IsNotDefault)

        self.declareProperty(
            "InnerGoniometer", False, "Whether the goniometer to be calculated is the most inner (phi) or most outer (omega)"
        )
        self.setPropertySettings("InnerGoniometer", condition)
        self.declareProperty(
            "FlipX",
            False,
            "Used when calculating goniometer angle if the q_lab x value should be negative, "
            "hence the detector of the other side (right) of the beam",
        )
        self.setPropertySettings("FlipX", condition)

    def PyExec(self):
        peaks = self.getProperty("Workspace").value

        wavelength = self.getProperty("Wavelength").value
        if wavelength == Property.EMPTY_DBL:
            wavelength = float(peaks.run()["wavelength"].value)

        if self.getProperty("OverrideProperty").value:
            flip_x = self.getProperty("FlipX").value
            inner = self.getProperty("InnerGoniometer").value
        else:
            flip_x = peaks.getInstrument().getName() == "HB3A"

            if peaks.getInstrument().getName() == "HB3A":
                inner = math.isclose(peaks.run().getTimeAveragedStd("omega"), 0.0)
            else:
                inner = False

        starting_goniometer = peaks.run().getGoniometer().getR()

        for n in range(peaks.getNumberPeaks()):
            p = peaks.getPeak(n)
            g = Goniometer()
            g.setR(starting_goniometer)
            g.calcFromQSampleAndWavelength(V3D(*p.getQSampleFrame()), wavelength, flip_x, inner)
            self.log().information(
                "Found goniometer omega={:.2f} chi={:.2f} phi={:.2f} for peak {} with Q_sample {}".format(
                    *g.getEulerAngles("YZY"), n, p.getQSampleFrame()
                )
            )
            p.setWavelength(wavelength)
            p.setGoniometerMatrix(g.getR())


AlgorithmFactory.subscribe(HFIRCalculateGoniometer)
