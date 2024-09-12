# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

"""
MonteCarloAbsorption is used within various workflow algorithms eg PaalmanPingsMonteCarloAbsorption
but it can be used directly by end users if they first set up a sample shape using SetSample or
LoadSampleShape. These tests cover this mode of use
"""

import systemtesting
import mantid.simpleapi as mantid


class CheckSampleOnlyAndEnvOnlyMatch(systemtesting.MantidSystemTest):
    abs_e = None
    abs_s = None

    def runTest(self):
        testws_e = mantid.CreateWorkspace([1.0, 2.0, 3.0], [4.0, 4.0, 4.0], UnitX="Wavelength")
        mantid.EditInstrumentGeometry(Workspace=testws_e, PrimaryFlightPath=10.0, L2=1.0, Polar=1.0)
        mantid.SetSample(
            testws_e,
            ContainerGeometry={"Shape": "FlatPlate", "Height": 4.0, "Width": 2.0, "Thick": 1.0, "Center": [0.0, 0.0, 0.0]},
            ContainerMaterial={"ChemicalFormula": "Al", "NumberDensity": 0.01},
        )
        mantid.SetBeam(InputWorkspace=testws_e, Geometry={"Shape": "Slit", "Width": 3.0, "Height": 6.75})
        self.abs_e = mantid.MonteCarloAbsorption(InputWorkspace=testws_e, OutputWorkspace="abs_e", EventsPerPoint=100)

        testws_s = mantid.CreateWorkspace([1.0, 2.0, 3.0], [4.0, 4.0, 4.0], UnitX="Wavelength")
        mantid.EditInstrumentGeometry(Workspace=testws_s, PrimaryFlightPath=10.0, L2=1.0, Polar=1.0)
        mantid.SetSample(
            testws_s,
            Geometry={"Shape": "FlatPlate", "Height": 4.0, "Width": 2.0, "Thick": 1.0, "Center": [0.0, 0.0, 0.0]},
            Material={"ChemicalFormula": "Al", "NumberDensity": 0.01},
        )
        mantid.SetBeam(InputWorkspace=testws_s, Geometry={"Shape": "Slit", "Width": 3.0, "Height": 6.75})
        self.abs_s = mantid.MonteCarloAbsorption(InputWorkspace=testws_s, OutputWorkspace="abs_s", EventsPerPoint=100)

    def validateMethod(self):
        self.tolerance = 1.0e-6
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.abs_e.name(), self.abs_s.name()
