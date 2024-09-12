# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import *
from mantid.kernel import *
from vesuvio.base import VesuvioBase
from mantid.simpleapi import *
import scipy.constants

import math
import numpy as np

MIN_TEMPERATURE = 1e-6
K_IN_MEV = scipy.constants.value("electron volt-kelvin relationship") / 1e3
EINSTEIN_CONSTANT = 2.0717
DEBYE_CONSTANT = 4.18036


class VesuvioPeakPrediction(VesuvioBase):
    _model = None
    _temperature = None
    _atomic_mass = None
    _frequency = None
    _debye_temp = None

    def summary(self):
        return "Predicts parameters for Vesuvio peak widths using Debye or Einstein methods"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        self.declareProperty(
            name="Model",
            defaultValue="Einstein",
            validator=StringListValidator(["Debye", "Einstein"]),
            doc="Model used to make predictions",
        )

        arrvalid = FloatArrayBoundedValidator(lower=0.0)

        self.declareProperty(FloatArrayProperty(name="Temperature", validator=arrvalid), doc="Temperature (K)")

        floatvalid = FloatBoundedValidator(0.0)
        floatvalid.setLowerExclusive(True)

        self.declareProperty(name="AtomicMass", defaultValue=1.0, validator=floatvalid, doc="Atomic Mass (AMU)")

        self.declareProperty(name="Frequency", defaultValue=1.0, validator=floatvalid, doc="Fundamental frequency of oscillator (mEV)")

        self.declareProperty(name="DebyeTemperature", defaultValue=1.0, validator=floatvalid, doc="Debye Temperature (K)")

        self.declareProperty(
            ITableWorkspaceProperty("OutputTable", "vesuvio_params", direction=Direction.Output), doc="The name of the output table"
        )

    def setup(self):
        self._model = self.getPropertyValue("Model")
        self._temperature = self.getProperty("Temperature").value
        self._atomic_mass = self.getProperty("AtomicMass").value
        self._frequency = self.getProperty("Frequency").value
        self._debye_temp = self.getProperty("DebyeTemperature").value

    def PyExec(self):
        self.setup()

        vesuvio_params = WorkspaceFactory.Instance().createTable()
        vesuvio_params.setTitle("Vesuvio Peak Parameters")
        vesuvio_params.addColumn("float", "Temperature(K)")
        vesuvio_params.addColumn("float", "Atomic Mass(AMU)")

        if self._model == "Einstein":
            vesuvio_params.addColumn("float", "Frequency(mEV)")
            vesuvio_params.addColumn("float", "Kinetic Energy(mEV)")
            vesuvio_params.addColumn("float", "Effective Temp(K)")
            vesuvio_params.addColumn("float", "RMS Momentum(A)")

            for temp in self._temperature:
                if temp == 0:
                    temp = MIN_TEMPERATURE

                # Convert to mEV
                temp_mev = temp / K_IN_MEV
                # Kinetic Energy
                kinetic_energy = 0.25 * self._frequency / math.tanh(self._frequency / (2 * temp_mev))
                # Effective temperature in K
                t_star = 2 * kinetic_energy * K_IN_MEV
                # RMS moment
                sig = math.sqrt(self._atomic_mass * kinetic_energy / EINSTEIN_CONSTANT)

                vesuvio_params.addRow([temp, self._atomic_mass, self._frequency, kinetic_energy, t_star, sig])

        if self._model == "Debye":
            vesuvio_params.addColumn("float", "Debye Temp(K)")
            vesuvio_params.addColumn("float", "Kinetic Energy(mEV)")
            vesuvio_params.addColumn("float", "RMS Momentum(A-1)")
            vesuvio_params.addColumn("float", "RMS Displacement(A)")

            for temp in self._temperature:
                if temp == 0:
                    temp = MIN_TEMPERATURE

                kinetic, rms_momentum = self.mean_energy(temp, self._debye_temp, self._atomic_mass)
                rms_disp = self.displacement(temp, self._debye_temp, self._atomic_mass)

                vesuvio_params.addRow([temp, self._atomic_mass, self._debye_temp, kinetic, rms_momentum, rms_disp])

        self.setProperty("OutputTable", vesuvio_params)

    def mean_energy(self, temp, debye_temp, atomic_mass):
        """
        temp: temperature in K
        debye_temp: debye temperature in K
        atomic_mass: atomic mass in AMU
        """

        n = 1000
        y = np.empty(n + 1, float)
        debye_energy = debye_temp / K_IN_MEV
        temp /= K_IN_MEV

        # calculate mean energy
        dx = debye_energy / (n - 1)
        for i in range(1, n + 1):
            x = dx * (i - 1) + dx / 1e6
            y[i] = x * (3.0 * x**2 / debye_energy**3) / (math.tanh(x / (2 * temp)))

        w_bar = self.r_integral(y, dx, n)

        k = 3.0 * w_bar / 4.0  # mean kinetic energy in 3D in mEV
        y_bar = math.sqrt(atomic_mass * w_bar / (2 * DEBYE_CONSTANT))  # RMS momentum along Q

        return k, y_bar

    def displacement(self, temp, debye_temp, atomic_mass):
        """
        temp: temperature in K
        debye_temp: debye temperature in K
        atomic_mass: atomic mass in AMU
        """

        n = 1000
        y = np.empty(n + 1, float)
        debye_energy = debye_temp / K_IN_MEV
        temp /= K_IN_MEV

        # calculate mean energy
        dx = debye_energy / (n - 1)
        for i in range(1, n + 1):
            x = dx * (i - 1) + dx / 1e6
            y[i] = (3.0 * x**2 / debye_energy**3) / (x * math.tanh(x / (2 * temp)))

        disp = math.sqrt(self.r_integral(y, dx, n) * DEBYE_CONSTANT / (2 * atomic_mass))
        return disp

    def r_integral(self, y, dx, n):
        """
        Function to perform integration using Simpson's Rule
        """
        s_even = np.sum(y[2 : n - 1 : 2])
        s_odd = np.sum(y[3 : n - 2 : 2])
        rint = dx * (y[1] + y[n] + 4 * s_even + 2 * s_odd) / 3.0

        return rint


AlgorithmFactory.subscribe(VesuvioPeakPrediction)
