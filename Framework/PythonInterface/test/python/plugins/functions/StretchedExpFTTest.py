import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit, CreateWorkspace, SaveNexus
import scipy.constants
import numpy as np

import testhelpers
from pdb import set_trace as tr

class _InternalMakeSEFTData( PythonAlgorithm ):

    def PyInit(self):
        self.declareProperty('height', 1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('tau', 100.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('beta', 1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('nhalfbins', 100)
        self.declareProperty('de', 0.0004)  # energy step, in meV
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output))

    def PyExec( self ):
        """Create the data workspace containing one spectrum resulting from the
        Fourier transform of an stretched exponential, plus some noise"""
        import numpy as np
        from scipy.fftpack import fft
        from scipy.interpolate import interp1d

        plank_constant = 4.135665616  # in units of meV*THz

        nhalfbins = self.getProperty( 'nhalfbins' ).value
        de = self.getProperty('de').value  # bin width, in meV or ueV
        nbins = 2*nhalfbins
        xvals = de*np.arange(-nhalfbins, nhalfbins+1)+de/2

        # Find the time domain
        M = 2*nhalfbins+1
        dt = plank_constant/(M*de)  # ps ( or ns), time step
        sampled_times = dt*np.arange(-nhalfbins, nhalfbins+1)

        height = self.getProperty('height').value
        tau = self.getProperty('tau').value
        beta = self.getProperty('beta').value

        # Define the stretched exponential on the time domain, then do its Fourier transform
        exponent = -(np.abs(sampled_times)/tau)**beta
        freqs = de*np.arange(-nhalfbins, nhalfbins+1)
        fourier = height*np.abs(fft(np.exp(exponent)).real)
        fourier = np.concatenate((fourier[nhalfbins+1:], fourier[0: nhalfbins+1]))
        interpolator = interp1d(freqs, fourier)
        N = nhalfbins/2
        ynominal = interpolator(xvals[N: -N-1])  # high energies are not well represented, avoid them

        wspace = WorkspaceFactory.create('Workspace2D', NVectors=1, XLength=2*N, YLength=2*N)
        # white noise in the [0.2, 1) range, average is 0.6
        noise = 0.2+0.8*np.random.random(2*N)
        sign = np.random.random(2*N)
        sign = np.where(sign > 0.5, 1, -1)  # random sign
        # average noise is 6% of the signal local intensity

        yvals = (1+0.05*noise*sign)*ynominal
        error = 0.1*noise*ynominal
        wspace.dataX(0)[:] = xvals[N: -N-1]
        wspace.dataY(0)[:] = yvals
        # average error is 12% of the signal local intensity
        wspace.dataE(0)[:] = error

        """ # Only for debugging purposes
        buffer = '#h = {0},  tau = {1},  beta = {2}\n'.format( height, tau, beta )
        for iy in range( len(yvals) ):
            buffer += '{0} {1} {2}\n'.format( xvals[N+iy], yvals[iy], error[iy] )
            open( '/tmp/junk.dat', 'w' ).write( buffer )
        """

        self.setProperty('OutputWorkspace', wspace)  # Stores the workspace as the given name


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
        print msg

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
        except RuntimeError, exc:
            self.fail('Could not create StretchedExpFT function: %s' % str(exc))

    def testGaussian(self):
        """ Test the Fourier transform of a gaussian is a Gaussian"""
        print "testGaussian"
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
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Center=0.0002;" +\
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
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Center=0.0002;" +\
                  "name=FlatBackground,A0=0.0"
        Fit(Function=fString, InputWorkspace="data", MaxIterations=100, Output="fit")
        self.asserFit(mtd["fit_Parameters"], height, beta, tau)
        self.cleanFit()

if __name__ == '__main__':
    unittest.main()
