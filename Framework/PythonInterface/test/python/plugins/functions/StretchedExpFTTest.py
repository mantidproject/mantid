from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit, CreateWorkspace, SaveNexus
import scipy.constants
import numpy as np

class StretchedExpFTTest(unittest.TestCase):
    # Class variables
    _planck_constant = scipy.constants.Planck/scipy.constants.e*1E15  # meV*psec

    @property
    def skipTest(self):
        try:
            import scipy.fftpack  # Scipy not available in windows debug
            return False
        except ImportError:
            return True

    def asserFit(self, workspace, height, beta, tau):
        unacceptable_chi_square = 1.0
        msg = ""
        for irow in range(workspace.rowCount()):
            row = workspace.row(irow)
            name = row['Name']
            if name == "Cost function value":
                chi_square = row['Value']
                msg += " chi_square=" + str(chi_square)
            elif name == "f0.Tau":
                tauOptimal = row['Value']
                msg += " tauOptimal=" + str(tauOptimal)
            elif name == "f0.Beta":
                betaOptimal = row['Value']
                msg += " betaOptimal=" + str(betaOptimal)
            elif name == "f0.Height":
                heightOptimal = row['Value']
                msg += " heightOptimal=" + str(heightOptimal)
        self.assertTrue(chi_square < unacceptable_chi_square, msg)
        self.assertTrue(abs(height - heightOptimal)/height < 0.01, msg)
        self.assertTrue(abs(beta - betaOptimal) < 0.01, msg)
        self.assertTrue(abs(tau - tauOptimal)/tauOptimal < 0.01, msg)

    def createData(self, functor):
        """Generate data for the fit"""
        de = 0.0004  # energy spacing in meV
        energies = np.arange(-0.1, 0.5, de)
        data = functor(energies)
        background = 0.01*max(data)
        data += background
        errorBars = data*0.1
        return CreateWorkspace(energies, data, errorBars, UnitX='DeltaE',
                               OutputWorkspace="data")

    def cleanFit(self):
        """Removes workspaces created during the fit"""
        mtd.remove('data')
        mtd.remove('fit_NormalisedCovarianceMatrix')
        mtd.remove('fit_Parameters')
        mtd.remove('fit_Workspace')

    def testRegistered(self):
        try:
            FunctionFactory.createFunction('StretchedExpFT')
        except RuntimeError as exc:
            self.fail('Could not create StretchedExpFT function: %s' % str(exc))

    def testGaussian(self):
        """ Test the Fourier transform of a gaussian is a Gaussian"""
        if self.skipTest:  # python2.6 doesn't have skipping decorators
            return
        # Target parameters
        tau = 20.0    # picoseconds
        beta = 2.0    # gaussian
        height = 1.0  # We want identity, not just proporcionality
        # Analytical Fourier transform of exp(-(t/tau)**2)
        E0 = StretchedExpFTTest._planck_constant/tau
        functor = lambda E: np.sqrt(np.pi)/E0 * np.exp(-(np.pi*E/E0)**2)
        self.createData(functor)  # Create workspace "data"
        # Initial guess reasonably far from target parameters
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Centre=0.0002;" +\
                  "name=FlatBackground,A0=0.0"
        Fit(Function=fString, InputWorkspace="data", MaxIterations=100, Output="fit")
        self.asserFit(mtd["fit_Parameters"], height, beta, tau)
        self.cleanFit()

    def testLorentzian(self):
        """ Test the Fourier transform of a exponential is a Lorentzian"""
        if self.skipTest:  # python2.6 doesn't have skipping decorators
            return
        # Target parameters
        tau = 100.0  # picoseconds
        beta = 1.0  # exponential
        height = 1.0  # We want identity, not just proporcionality
        # Analytical Fourier transform of exp(-t/tau)
        hwhm = StretchedExpFTTest._planck_constant/(2*np.pi*tau)
        functor = lambda E: (1.0/np.pi) * hwhm/(hwhm**2 + E**2)
        self.createData(functor)  # Create workspace "data"
        # Initial guess reasonably far from target parameters
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Centre=0.0002;" +\
                  "name=FlatBackground,A0=0.0"
        Fit(Function=fString, InputWorkspace="data", MaxIterations=100, Output="fit")
        self.asserFit(mtd["fit_Parameters"], height, beta, tau)
        self.cleanFit()

if __name__ == '__main__':
    unittest.main()
