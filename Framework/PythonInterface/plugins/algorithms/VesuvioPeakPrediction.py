from mantid.api import *
from mantid.kernel import *
from mantid import config
from vesuvio.base import VesuvioBase
from mantid.simpleapi import *

import math
import numpy as np


class VesuvioPeakPrediction(VesuvioBase):
    _model = None
    _temperature = None
    _atomic_mass = None
    _frequency = None
    _debye_temp = None

    def summary(self):
        return "Predicts parameters for Vesuvio peak widths using Debye or Einstein methods"

    def category(self):
        return 'Inelastic\\Indirect\\Vesuvio'

    def PyInit(self):

        self.declareProperty(name='Model', defaultValue='Einstein',
                             validator=StringListValidator(['Debye', 'Einstein']),
                             doc='Model used to make predictions')

        self.declareProperty(FloatArrayProperty(name='Temperature', validator=FloatArrayMandatoryValidator()),
                             doc='Temperature (K)')

        self.declareProperty(name='AtomicMass', defaultValue=0.0,
                             validator=FloatMandatoryValidator(),
                             doc='Atomic Mass (AMU)')

        self.declareProperty(name='Frequency', defaultValue=0.0,
                             validator=FloatMandatoryValidator(),
                             doc='Fundamental frequency of oscillator (mEV)')

        self.declareProperty(name='DebyeTemperature', defaultValue=0.0,
                             validator=FloatMandatoryValidator(),
                             doc='Debye Temperature (K)')

    def setup(self):

        self._model = self.getPropertyValue('Model')
        self._temperature = self.getProperty('Temperature').value
        self._atomic_mass = self.getProperty('AtomicMass').value
        self._frequency = self.getProperty('Frequency').value
        self._debye_temp = self.getProperty('DebyeTemperature').value

    def PyExec(self):

        self.setup()

        vesuvio_params = CreateEmptyTableWorkspace()
        vesuvio_params.setTitle('Vesuvio Peak Parameters')
        vesuvio_params.addColumn('float', 'Temperature')
        vesuvio_params.addColumn('float', 'AtomicMass')

        if self._model == 'Einstein':
            vesuvio_params.addColumn('float', 'Frequency')
            vesuvio_params.addColumn('float', 'Kinetic Energy')
            vesuvio_params.addColumn('float', 'Effective Temp')
            vesuvio_params.addColumn('float', 'RMS Momentum')

            for temp in self._temperature:

                if temp == 0:
                    temp = 1e-6
                elif temp < 0:
                    raise ValueError

                # Convert to mEV
                temp_mev = temp / 11.604
                # Kinetic Energy
                kinetic_energy = 0.25 * self._frequency / math.tanh(self._frequency / (2 * temp_mev))
                # Effective temperature in K
                t_star = 2 * kinetic_energy * 11.604
                # RMS moment
                sig = math.sqrt(self._atomic_mass * kinetic_energy / 2.0717)

                vesuvio_params.addRow([temp, self._atomic_mass, self._frequency, kinetic_energy, t_star, sig])

        if self._model == 'Debye':
            vesuvio_params.addColumn('float', 'Debye Temp')
            vesuvio_params.addColumn('float', 'Kinetic Energy')
            vesuvio_params.addColumn('float', 'RMS Momentum')
            vesuvio_params.addColumn('float', 'RMS Displacement')

            print(self._temperature)
            for temp in self._temperature:

                kinetic, rms_momentum = self.mean_energy(temp, self._debye_temp, self._atomic_mass)
                rms_disp = self.displacement(temp, self._debye_temp, self._atomic_mass)

                vesuvio_params.addRow([temp, self._atomic_mass, self._debye_temp, kinetic, rms_momentum, rms_disp])

    def mean_energy(self, temp, debye_temp, atomic_mass):
        """
        temp: temperature in K
        debye_temp: debye temperature in K
        atomic_mass: atomic mass in AMU
        """

        n = 1000
        y = np.empty(n + 1, float)
        debye_energy = debye_temp / 11.604
        temp /= 11.604

        # calculate mean energy
        dx = debye_energy / (n - 1)
        for i in range(1, n + 1):
            x = dx * (i - 1) + dx / 1e6
            y[i] = x * (3.0 * x ** 2 / debye_energy ** 3) / (math.tanh(x / (2 * temp)))

        w_bar = self.r_integral(y, dx, n)

        k = 3.0 * w_bar / 4.0  # mean kinetic energy in 3D in mEV
        eff_temp = 2.0 * k * 11.604 / 3.0  # effective temperature
        y_bar = math.sqrt(atomic_mass * w_bar / (2 * 4.18036))  # RMS momentum along Q

        return k, y_bar

    def displacement(self, temp, debye_temp, atomic_mass):
        """
        temp: temperature in K
        debye_temp: debye temperature in K
        atomic_mass: atomic mass in AMU
        """

        n = 1000
        y = np.empty(n + 1, float)
        debye_energy = debye_temp / 11.604
        temp /= 11.604

        # calculate mean energy
        dx = debye_energy / (n - 1)
        for i in range(1, n + 1):
            x = dx * (i - 1) + dx / 1e6
            y[i] = (3.0 * x ** 2 / debye_energy ** 3) / (x * math.tanh(x / (2 * temp)))

        disp = math.sqrt(self.r_integral(y, dx, n) * 4.18036 / (2 * atomic_mass))
        return disp

    def r_integral(self, y, dx, n):
        """
        Function to perform integration using Simpson's Rule
        """
        s_even = np.sum(y[2:n - 1:2])
        s_odd = np.sum(y[3:n - 2:2])
        rint = dx * (y[1] + y[n] + 4 * s_even + 2 * s_odd) / 3.0

        return rint




AlgorithmFactory.subscribe(VesuvioPeakPrediction)
