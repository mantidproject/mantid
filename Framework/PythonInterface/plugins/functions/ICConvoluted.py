# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ICConvoluted.py
#
# Defines the IPeakFunction IkedaCarpenterConvoluted
# which is the standard Ikeda-Carpenter (IC) function convoluted with
# a square wave and a Gaussian.
#
#
import numpy as np
from mantid.api import IFunction1D, FunctionFactory


class IkedaCarpenterConvoluted(IFunction1D):
    def init(self):
        self.declareParameter("A")  # Alpha
        self.declareParameter("B")  # Beta
        self.declareParameter("R")  # R - ratio of fast to slow neutrons
        self.declareParameter("T0")  # T0 - time offset
        self.declareParameter("Scale")  # amplitude
        self.declareParameter("HatWidth")  # width of square wave
        self.declareParameter("KConv")  # KConv for Gaussian

    # use penalty=None to not use default mantid penalty
    def setPenalizedConstraints(self, A0=None, B0=None, R0=None, T00=None, Scale0=None, HatWidth0=None, KConv0=None, penalty=None):
        if A0 is not None:
            self.addConstraints("{:4.4e} < A < {:4.4e}".format(A0[0], A0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("A", penalty)
        if B0 is not None:
            self.addConstraints("{:4.4e} < B < {:4.4e}".format(B0[0], B0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("B", penalty)
        if R0 is not None:
            self.addConstraints("{:4.4e} < R < {:4.4e}".format(R0[0], R0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("R", penalty)
        if T00 is not None:
            self.addConstraints("{:4.4e} < T0 < {:4.4e}".format(T00[0], T00[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("T0", penalty)
        if Scale0 is not None:
            self.addConstraints("{:4.4e} < Scale < {:4.4e}".format(Scale0[0], Scale0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("Scale", penalty)
        if HatWidth0 is not None:
            self.addConstraints("{:4.4e} < HatWidth < {:4.4e}".format(HatWidth0[0], HatWidth0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("HatWidth", penalty)
        if KConv0 is not None:
            self.addConstraints("{:4.4e} < KConv < {:4.4e}".format(KConv0[0], KConv0[1]))
            if penalty is not None:
                self.setConstraintPenaltyFactor("KConv", penalty)

    def function1D(self, t):
        A = self.getParamValue(0)
        B = self.getParamValue(1)
        R = self.getParamValue(2)
        T0 = self.getParamValue(3)
        Scale = self.getParamValue(4)
        HatWidth = self.getParamValue(5)
        KConv = self.getParamValue(6)

        # A/2 Scale factor has been removed to make A and Scale independent
        f_int = Scale * (
            (1 - R) * np.power((A * (t - T0)), 2) * np.exp(-A * (t - T0))
            + 2
            * R
            * A**2
            * B
            / np.power((A - B), 3)
            * (
                np.exp(-B * (t - T0))
                - np.exp(-A * (t - T0)) * (1 + (A - B) * (t - T0) + 0.5 * np.power((A - B), 2) * np.power((t - T0), 2))
            )
        )
        f_int[t < T0] = 0

        mid_point_hat = len(f_int) // 2
        gc_x = np.array(range(len(f_int))).astype(float)
        ppd = 0.0 * gc_x
        lowIDX = int(np.floor(np.max([mid_point_hat - np.abs(HatWidth), 0])))
        highIDX = int(np.ceil(np.min([mid_point_hat + np.abs(HatWidth), len(gc_x)])))

        ppd[lowIDX:highIDX] = 1.0
        ppd = ppd / sum(ppd)

        gc_x = np.array(range(len(f_int))).astype(float)
        gc_x = 2 * (gc_x - np.min(gc_x)) / (np.max(gc_x) - np.min(gc_x)) - 1
        gc_f = np.exp(-KConv * np.power(gc_x, 2))
        gc_f = gc_f / np.sum(gc_f)

        npad = len(f_int) - 1
        first = npad - npad // 2
        f_int = np.convolve(f_int, ppd, "full")[first : first + len(f_int)]
        f_int = np.convolve(f_int, gc_f, "full")[first : first + len(f_int)]

        return f_int

    # Evaluate the function for a differnt set of paremeters (trialc)
    def function1DDiffParams(self, xvals, trialc):
        # First, grab the original parameters and set to trialc
        c = np.zeros(self.numParams())
        for i in range(self.numParams()):
            c[i] = self.getParamValue(i)
            self.setParameter(i, trialc[i])

        # Get the trial values
        f_trial = self.function1D(xvals)

        # Now return to the orignial
        for i in range(self.numParams()):
            self.setParameter(i, c[i])
        return f_trial

    # Construction the Jacobian (df) for the function
    def functionDeriv1D(self, xvals, jacobian, eps=1.0e-3):
        f_int = self.function1D(xvals)
        # Fetch parameters into array c
        c = np.zeros(self.numParams())
        for i in range(self.numParams()):
            c[i] = self.getParamValue(i)
        nc = np.prod(np.shape(c))
        for k in range(nc):
            dc = np.zeros(nc)
            dc[k] = max(eps, eps * c[k])
            f_new = self.function1DDiffParams(xvals, c + dc)
            for i, dF in enumerate(f_new - f_int):
                jacobian.set(i, k, dF / dc[k])


FunctionFactory.subscribe(IkedaCarpenterConvoluted)
