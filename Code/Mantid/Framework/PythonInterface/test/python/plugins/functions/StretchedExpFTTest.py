import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit
import testhelpers

from pdb import set_trace as tr

class _InternalMakeSEFTData(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty('height', 1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('tau',    1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('beta',   1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('nhalfbins', 100)
        self.declareProperty('de', 0.0004) # energy step in meV
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction = Direction.Output))
        
    def PyExec(self):
        '''Create the data workspace containing one spectrum resulting from the
        Fourier transform of an stretched exponential, plus some noise'''
        import numpy as np
        from scipy.fftpack import fft
        from scipy.interpolate import interp1d
        
        meV2ps = 4.136 # from meV (or ueV) to ps (or ns)
        
        nhalfbins = self.getProperty('nhalfbins').value
        de = self.getProperty('de').value #bin width, in meV or ueV
        nbins = 2*nhalfbins
        xvals = de*np.arange(-nhalfbins, nhalfbins) + de/2  # energy values
        
        #Find the time domain
        N = 1+nhalfbins
        M = 2*N+1
        dt = meV2ps / (M*de) # ps ( or ns), time step
        sampled_times = dt * np.arange(-N, N+1)
        
        height = self.getProperty('height').value
        tau = self.getProperty('tau').value
        beta = self.getProperty('beta').value
        
        #Define the stretched exponential on the time domain, then do its Fourier transform
        exponent = -(np.abs(sampled_times)/tau)**beta
        freqs = de * np.arange(-N, N+1)
        fourier = height*np.abs( fft( np.exp(exponent) ).real )
        fourier = np.concatenate( (fourier[N+1:],fourier[0:N+1]) )
        interpolator = interp1d(freqs, fourier)
        ynominal = interpolator(xvals)
               
        wspace = WorkspaceFactory.create('Workspace2D', NVectors=1, XLength=nbins, YLength=nbins)
        # white noise in the [0.2, 1) range, average is 0.6
        noise = 0.2 + 0.8*np.random.random(nbins)
        sign = np.random.random(nbins)
        sign = np.where(sign>0.5, 1, -1) # random sign
        # average noise is 6% of the signal local intensity

        yvals = (1+0.1*noise*sign) * ynominal
        error = 0.2*noise*ynominal
        wspace.dataX(0)[:] = xvals
        wspace.dataY(0)[:] = yvals
        # average error is 12% of the signal local intensity
        wspace.dataE(0)[:] = error
        
        self.setProperty('OutputWorkspace', wspace) # Stores the workspace as the given name
            
class StretchedExpFTTest(unittest.TestCase):

    def test_registered(self):
        try:
            FunctionFactory.createFunction('StretchedExpFT')
        except RuntimeError, exc:
            self.fail('Could not create StretchedExpFT function: %s' % str(exc))
            
    def test_fit(self):
        from random import random
        variation = lambda x: x+x*(random()-0.5)  # range [-x/2, x/2]
        #Generate a data workspace using random parameters around {'height':0.1,'tau':100,'beta':1}
        parms={'height':variation(0.1), 'tau':variation(100), 'beta':variation(1)}

        Nh = 4000
        de = 0.0004
        AlgorithmFactory.subscribe(_InternalMakeSEFTData)
        alg = testhelpers.run_algorithm('_InternalMakeSEFTData', nhalfbins=Nh, de=de,
                                         OutputWorkspace='_test_seft_data', **parms)
        input_ws = alg.getProperty('OutputWorkspace').value
        
        sx = -Nh*de + de/2
        ex = (Nh-1)*de + de/2
        func_string = 'name=StretchedExpFT,height=0.1,tau=100,beta=1'
        Fit(Function=func_string, InputWorkspace='_test_seft_data',
            StartX=sx, EndX=ex, CreateOutput=1, MaxIterations=20)
        
        ws = mtd['_test_seft_data_Parameters']
        fitted=''
        for irow in range( ws.rowCount() ):
            row = ws.row(irow)
            name = row['Name']
            if name == 'Cost function value':
                value = row['Value']
            elif name in parms.keys():
                fitted += '{0}={1}, '.format(name, row['Value'])
        
        target = ', '.join( '{0}={1}'.format(key,val) for key,val in parms.items() )       
        msg='Cost function {0} too high\nTargets were {1},\nbut obtained {2}'.format(value,target,fitted)
        self.assertTrue(value < 5.0, msg)
        msg='Cost function {0}\nStarted with height=0.1, tau=100, beta=1\nTargets were {1},\nobtained {2}'.format(value,target,fitted)
        
        mtd.remove('_test_seft_data')
        mtd.remove('_test_seft_data_NormalisedCovarianceMatrix')
        mtd.remove('_test_seft_data_Parameters')
        mtd.remove('_test_seft_data_Workspace')

if __name__ == '__main__':
    unittest.main()
