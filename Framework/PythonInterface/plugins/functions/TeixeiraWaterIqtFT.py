# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy as np

from mantid.api import IFunction1D, FunctionFactory
from scipy import constants
from scipy.fftpack import fft, fftfreq
from scipy.special import spherical_jn
import copy

from TeixeiraWaterIqtHelper import functionTeixeiraWaterIQT


class TeixeiraWaterIqtFT(IFunction1D):
    def category(self):
        return "QuasiElastic"

    def init(self):
        self.declareParameter("Amp", 1.00, "Amplitude")
        self.declareParameter("Tau1", 1, "Relaxation time")
        self.declareParameter("Gamma", 1.2, "Line Width")
        self.addConstraints("Tau1 > 0")

        self.declareAttribute("Q", 0.4)
        self.declareAttribute("a", 0.98)

        self._parmList = ["Amp", "Tau1", "Gamma"]

    def validateParams(self):
        amp = self.getParameterValue("Amp")
        tau1 = self.getParameterValue("Tau1")
        gamma = self.getParameterValue("Gamma")

        for value in (amp, tau1, gamma):
            if value <= 0:
                return None

        return {"Amp": amp, "Tau1": tau1, "Gamma": gamma}

    def function1D(self, xvals, **optparms):
        p = self.validateParams()
        if p is None:
            return np.zeros(len(xvals), dtype=float)

        if optparms:
            for name in optparms.keys():
                p[name] = optparms[name]

        amp = p["Amp"]
        tau1 = p["Tau1"]
        gamma = p["Gamma"]

        q_value = self.getAttributeValue("Q")
        radius = self.getAttributeValue("a")

        planck_constant = constants.Planck / constants.e * 1e15

        ne = len(xvals)

        de = (xvals[-1] - xvals[0]) / (16 * (ne - 1))
        tmax = planck_constant / de

        erange = 2 * max(abs(xvals))
        dt = 0.5 * planck_constant / erange

        nt = 2 ** (1 + int(np.log(tmax / dt) / np.log(2)))
        sampled_times = dt * np.arange(-nt, nt)

        iqt = functionTeixeiraWaterIQT(amp, tau1, gamma, q_value, radius, np.abs(sampled_times))

        fourier = np.abs(fft(iqt).real)

        fourier /= fourier[0]

        qr = q_value * radius
        j0 = spherical_jn(0, qr)
        j1 = spherical_jn(1, qr)
        j2 = spherical_jn(2, qr)

        norm_j0 = np.square(j0) / gamma
        norm_j1 = 3 * np.square(j1) / (1 / (3 * tau1) + gamma)
        norm_j2 = 5 * np.square(j2) / (1 / tau1 + gamma)

        total_norm = amp * (norm_j0 + norm_j1 + norm_j2) / planck_constant

        fourier *= 2 * total_norm

        fourier = np.concatenate([fourier[nt:], fourier[:nt]])

        energies = planck_constant * fftfreq(2 * nt, d=dt)
        energies = np.concatenate([energies[nt:], energies[:nt]])

        return np.interp(xvals, energies, fourier)

    def fillJacobian(self, xvals, jacobian, partials):
        if not partials:
            for ip in range(len(self._parmList)):
                for ix in range(len(xvals)):
                    jacobian.set(ix, ip, 0.0)
        else:
            for ip in range(len(self._parmList)):
                name = self._parmList[ip]
                pd = partials[name]
                for ix in range(len(xvals)):
                    jacobian.set(ix, ip, pd[ix])

    def functionDeriv1D(self, xvals, jacobian):
        partials = {}
        p = self.validateParams()
        if not p:
            self.fillJacobian(xvals, jacobian, {})
            return

        f0 = self.function1D(xvals)

        dp = {"Tau1": 1.0, "Gamma": 0.01}
        for name in dp.keys():
            pp = copy.copy(p)
            pp[name] += dp[name]
            partials[name] = (self.function1D(xvals, **pp) - f0) / dp[name]

        pp = copy.copy(p)
        pp["Amp"] = 1.0
        partials["Amp"] = self.function1D(xvals, **pp)

        self.fillJacobian(xvals, jacobian, partials)


FunctionFactory.subscribe(TeixeiraWaterIqtFT)
