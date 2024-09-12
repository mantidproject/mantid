# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name, line-too-long
"""
These system tests are to verify the behaviour of the
ISIS reflectometry instruments signed theta value
"""

import os
from mantid.simpleapi import *
import systemtesting


class ReflectometryInstrumentSignedThetaTest(systemtesting.MantidSystemTest):
    def signed_theta_test(self, idf_name, detector_vertical_position, detector_name="point-detector"):
        idf_dir = config["instrumentDefinition.directory"]
        # Load instrument definition
        I = LoadEmptyInstrument(os.path.join(idf_dir, idf_name))
        # Get the reference frame from loaded IDF
        ref_frame = I.getInstrument().getReferenceFrame()
        # Create translation vector for detector
        movements = dict()
        movements[ref_frame.pointingUpAxis()] = detector_vertical_position
        movements[ref_frame.pointingAlongBeamAxis()] = 0
        movements[ref_frame.pointingHorizontalAxis()] = 0
        # Move detector based on vector
        MoveInstrumentComponent(Workspace=I, ComponentName=detector_name, **movements)
        # Convert from Signed-Theta vs Lam to signed theta
        theta_spectrum_axis = ConvertSpectrumAxis(InputWorkspace=I, OutputWorkspace="SignedTheta_vs_Wavelength", Target="signed_theta")
        # Retrieve point detector from IDF (after translation)
        detector = theta_spectrum_axis.getInstrument().getComponentByName(detector_name)
        # Compare det-position * detector two theta with signed 2 theta (they should always be equal)
        self.assertTrue(
            detector_vertical_position * theta_spectrum_axis.detectorTwoTheta(detector)
            == theta_spectrum_axis.detectorSignedTwoTheta(detector)
        )

        return True

    def runTest(self):
        # Run for INTER
        self.signed_theta_test("INTER_Definition.xml", 1)
        self.signed_theta_test("INTER_Definition.xml", -1)
        # Run for CRISP
        self.signed_theta_test("CRISP_Definition.xml", 1)
        self.signed_theta_test("CRISP_Definition.xml", -1)
        # Run for POLREF

        self.signed_theta_test("POLREF_Definition.xml", 1)
        self.signed_theta_test("POLREF_Definition.xml", -1)
        # Run for SURF
        self.signed_theta_test("SURF_Definition.xml", 1)
        self.signed_theta_test("SURF_Definition.xml", -1)

    def validate(self):
        return True
