# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Original Author: Chris Ridley
from mantid.kernel import Direction, FloatBoundedValidator
from mantid.api import AlgorithmFactory, PythonAlgorithm, ITableWorkspaceProperty
from mantid.simpleapi import CreateEmptyTableWorkspace, DeleteWorkspace
import numpy as np

TOL = 0.01


# this calculation taken from the EOS equations in [1]
def calculate_pressure(d_spacing, temp):
    v00 = 121.41813  # zero pressure unit cell volume at (0) (A^3)
    a = 1.0582e-02  # volume fit parameter
    b = 3.493e-06  # volume fit parameter
    k00 = 41.7253509951022  # zero pressure bulk modulus at (0) (GPa)
    c = -2.54374974975855e-02  # bulk modulus fit parameter
    d = -2.7567006420938e-06  # bulk modulus fit parameter
    k00p = 5.3944  # K' at (0)
    e = 0.00111  # K' fit parameter
    k00pp = -0.3272  # K'' at (0)
    v_calc = (np.sqrt(3) * d_spacing) ** 3  # unit cell volume (A^3)
    ltemp = temp - 300

    v_quad = v00 + a * ltemp + b * (ltemp**2)
    k_quad = k00 + (c * ltemp) + (d * (ltemp**2))
    factor = k00p + (e * temp)

    return (
        1.5
        * k_quad
        * (((v_calc / v_quad) ** (-2 / 3)) - 1)
        * (1 + ((v_calc / v_quad) ** (-2 / 3) - 1)) ** (5 / 2)
        * (
            (1 + ((3 / 2) * (factor - 4) * ((((v_calc / v_quad) ** (-2 / 3)) - 1) / 2)))
            + ((3 / 2) * (((k_quad * k00pp) + (factor - 4) * (factor - 3) + (35 / 9)) * ((((v_calc / v_quad) ** (-2 / 3)) - 1) / 2) ** 2))
        )
    )


class LeadPressureCalc(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Calibration"

    def version(self):
        return 1

    def name(self):
        return "LeadPressureCalc"

    def summary(self):
        return "Find the pressure of a sample given the dSpacing of the lead peak at 111."

    def PyInit(self):
        self.declareProperty(
            name="DSpacing",
            defaultValue=2.8589,
            validator=FloatBoundedValidator(lower=2, upper=2.95),
            doc="Position of (111) reflection in dSpacing",
            direction=Direction.Input,
        )
        self.declareProperty(
            name="T",
            defaultValue=300.0,
            validator=FloatBoundedValidator(lower=100),
            doc="Sample temperature in K",
            direction=Direction.Input,
        )
        self.declareProperty(
            name="TargetPressure",
            defaultValue=0.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Optional: search for (111) position for a given pressure (GPa) and temperature, leave at default value to disable.",
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="OutputWorkspace", direction=Direction.Output, defaultValue="LeadPressureCalcResults"),
            doc="Name of Output Table workspace holding the values",
        )

    def PyExec(self):
        d_spacing = self.getProperty("DSpacing").value
        temp = self.getProperty("T").value
        p_calc = calculate_pressure(d_spacing, temp)
        self.log().notice("The calculated pressure is " + str(p_calc) + " GPa")
        ws = CreateEmptyTableWorkspace()
        ws.addColumn(type="double", name="Input dSpacing-111 (A)")
        ws.addColumn(type="double", name="Temperature (K)")
        ws.addColumn(type="double", name="Calculated Pressure (GPa)")

        p_target = self.getProperty("TargetPressure").value
        use_input_target = True
        if p_target == 0.0:
            use_input_target = False
            p_target = p_calc
        test_dspacing = np.arange(2, 2.95, 0.0001)
        pressure = calculate_pressure(test_dspacing, temp)
        diff = abs(pressure - p_target)
        index = np.argmin(diff)
        diff = diff[index]
        if diff < TOL:
            found_d = test_dspacing[index]
        else:
            diff, found_d = 0, 0
        if found_d != 0:
            if use_input_target:
                self.log().notice("Temperature: " + str(temp) + " K")
                self.log().notice("Target pressure: " + str(round(p_target, 6)) + " GPa")
                self.log().notice("Pressure difference: " + str(round(diff, 6)) + " GPa")
                self.log().notice("d(111): " + str(round(found_d, 6)) + " A")

            else:
                self.log().notice("Temperature: " + str(temp) + " K")
                self.log().notice("Target pressure (calculated): " + str(round(p_target, 6)) + " GPa")
                self.log().notice("Pressure difference: " + str(round(diff, 6)) + " GPa")
                self.log().notice("d(111) : " + str(round(found_d, 6)) + " A")
            ws.addColumn(type="double", name="Pressure Target (GPa)")
            ws.addColumn(type="double", name="Pressure difference (GPa)")
            ws.addColumn(type="double", name="dSpacing found (A)")
            row = {
                "Input dSpacing-111 (A)": d_spacing,
                "Temperature (K)": temp,
                "Calculated Pressure (GPa)": p_calc,
                "Pressure Target (GPa)": p_target,
                "Pressure difference (GPa)": diff,
                "dSpacing found (A)": found_d,
            }
        else:
            self.log().notice(
                "dSpacing corresponding to the Target Pressure and Temperature given not found in range"
                " 2-2.95. Please try different parameters."
            )
            row = {"Input dSpacing-111 (A)": d_spacing, "Temperature (K)": temp, "Calculated Pressure (GPa)": p_calc}
        ws.addRow(row)
        self.setProperty("OutputWorkspace", ws)
        DeleteWorkspace(ws)


AlgorithmFactory.subscribe(LeadPressureCalc)

# [1] Fortes, A. D. (2019). A revised equation of state for in situ pressure determination
# using fcc-Pb (0 < P < 13 GPa, T > 100 K)
# RAL Technical Report, RAL-TR-2019-002
# https://epubs.stfc.ac.uk/work/40740875
