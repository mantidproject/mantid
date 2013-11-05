import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit
import testhelpers


class _InternalMakeSEFTData(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty('height', 1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('tau',    1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('beta',   1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty('nhalfbins', 100)
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction = Direction.Output))
        
    def PyExec(self):
        import numpy as np
        from scipy.fftpack import fft
        
        nhalfbins = self.getProperty('nhalfbins').value
        nbins = 1 + 2*nhalfbins
        wspace = WorkspaceFactory.create('Workspace2D', NVectors=1, XLength=nbins, YLength=nbins)
        
        height = self.getProperty('height').value
        tau    = self.getProperty('tau').value
        beta   = self.getProperty('beta').value

        xvals = np.arange(-nhalfbins, 1+nhalfbins) * (1./nhalfbins) # from -1 to 1
        exponent = -(np.abs(xvals)/tau)**beta
        ynominal = height*fft( np.exp(exponent) ).real
        # white noise in the [0.2, 1) range, average is 0.6
        noise = 0.2 + 0.8*np.random.random(nbins)
        sign = np.random.random(nbins)
        sign = np.where(sign>0.5, 1, -1) # random sign
        # average noise is 6% of the signal local intensity
        yvals = (1+0.1*noise*sign) * ynominal
        
        wspace.dataX(0)[:] = xvals
        wspace.dataY(0)[:] = yvals
        # average error is 12% of the signal local intensity
        wspace.dataE(0)[:] = 0.2*noise*ynominal

        self.setProperty('OutputWorkspace', wspace) # Stores the workspace as the given name

class StretchedExpFTTest(unittest.TestCase):

    def test_registered(self):
        try:
            FunctionFactory.createFunction('StretchedExpFT')
        except RuntimeError, exc:
            self.fail('Could not create StretchedExpFT function: %s' % str(exc))
            
    def test_fit(self):
        parms={'height':0.83, 'tau':2.45, 'beta':0.43}
        AlgorithmFactory.subscribe(_InternalMakeSEFTData)
        alg = testhelpers.run_algorithm('_InternalMakeSEFTData', nhalfbins=4000,
                                         OutputWorkspace='_test_seft_data', **parms)
        input_ws = alg.getProperty('OutputWorkspace').value
        
        func_string = 'name=StretechedExpFT,' + ','.join( '{}={}'.format(key,val) for key,val in parms.items() )
        Fit(Function=func_string, InputWorkspace='_test_data',
            StartX=-1, EndX=1, CreateOutput=1, MaxIterations=20)
        
        ws = mtd['_test_seft_data_Parameters']
        for (index, name) in enumerate(ws.column(0)):
            if name in parms.keys():
                target = parms[name]
                value = ws.column(1)[index]
                delta = 0.05 * target # 5% error
                self.assertAlmostEqual(value, target, delta=delta)

        mtd.remove('_test_seft_data')
        mtd.remove('_test_seft_data_NormalisedCovarianceMatrix')
        mtd.remove('_test_seft_data_Parameters')
        mtd.remove('_test_seft_data_Workspace')

if __name__ == '__main__':
    unittest.main()
