'''
Provide the Fourier Transform of the Symmetrized Stretched Exponential Function
  Fourier{ height * exp( - |t/tau|**beta ) }
    
@author Jose Borreguero, NScD
@date October 06, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
'''
from mantid.api import IFunction1D, FunctionFactory #, BoundaryConstraint
from mantid import logger
import numpy as np
from scipy.fftpack import fft
from scipy.interpolate import interp1d
import copy

from pdb import set_trace as tr

class StretchedExpFT(IFunction1D):
    
    def __init__(self):
        '''declare some constants'''
        super(StretchedExpFT, self).__init__()
        self._meV2ps = 4.136
        self._parmset = {'height','tau','beta'}
        self._parm2index = {'height':0,'tau':1,'beta':2} #order in which they were defined

    def category(self):
        return 'QENS'

    def init(self):
        '''Declare parameters that participate in the fitting'''
        # Active fitting parameters
        self.declareParameter('height', 0.1, 'Intensity at the origin')
        self.declareParameter('tau', 100.0, 'Relaxation time of the standard exponential')
        self.declareParameter('beta',1.0, 'Stretching exponent')
        #self.addConstraints() # constraints not yet exposed to python
        self.validateParams()

    def validateParams(self):
        '''Check parameters are positive'''
        height = self.getParameterValue('height')
        tau = self.getParameterValue('tau')
        beta = self.getParameterValue('beta')
        for name,value in {'height':height, 'tau':tau, 'beta':beta}.items():
            if value <=0:
                message = 'Parameter {} in StretchedExpFT must be positive. Got {} instead'.format(name, str(value))
                logger.error(message)
                raise ValueError(message)
        return {'height':height, 'tau':tau, 'beta':beta}
       
    def function1D(self, xvals, **optparms):
        '''
        Computes the Fourier transform of the Symmetrized Stretched Exponential
        
        The Symmetrized Stretched Exponential:
                Fourier{ height * exp( - |t/tau|**beta ) }
        
        Given a time step dt and M=2*N+1 time points (N negative, one at zero, N positive),
        then fft will sample frequencies [0, 1/(M*dt), N/(M*dt), -N/(M*dt), (-N+1)/(M*dt),..,1/(M*dt)].
        
        Given xvals, let:
            1/(M*dt) = xvals[1]-xvals[0]
            N/(M*dt) = max(abs(xvals))
        Thus:
            dt = 1/[M*(xvals[1]-xvals[0])]  # M=2*N+1
            N = max(abs(xvals)) / (xvals[1]-xvals[0])

        Its Fourier transform is real by definition, thus we return the real part
        of the Fast Fourier Transform (FFT). The FFT step is meant to produce
        some residual imaginary part due to numerical inaccuracies which we ignore.
        
        We take the absolute value of the real part. The Fourier transform introduces
        an extra factor exp(i*pi*E/de) which amounts to alternating sign every
        time E increases by de, the energy bin width
        '''
        p=self.validateParams()
        # override parameter values with optparms (used for the numerical derivative)
        if optparms:
            if self._parmset.issubset( set(optparms.keys()) ):
                for name in self._parmset: p[name] = optparms[name]
        de = xvals[1]-xvals[0] # meV (or ueV) , energy step
        # make sure M > len(xvals) so that we can use interp1d later
        N = 1+ int( max(np.abs(xvals)) / de )
        M = 2*N+1
        dt = self._meV2ps / (M*de) # ps ( or ns), time step
        sampled_times = dt * np.arange(-N, N+1)
        exponent = -(np.abs(sampled_times)/p['tau'])**p['beta']
        freqs = de * np.arange(-N, N+1)
        fourier = p['height']*np.abs( fft( np.exp(exponent) ).real )
        fourier = np.concatenate( (fourier[N+1:],fourier[0:N+1]) )
        interpolator = interp1d(freqs, fourier)
        fourier = interpolator(xvals)
        return fourier
    
    def functionDeriv1D(self, xvals, jacobian):
        '''Numerical derivative'''
        p = self.validateParams()
        f0 = self.function1D(xvals)
        dp = {}
        for (key,val) in p.items(): dp[key] = 0.1 * val #modify by ten percent
        for name in self._parmset:
            pp = copy.copy(p)
            pp[name] += dp[name]
            df = (self.function1D(xvals, **pp) - f0) / dp[name]
            ip = self._parm2index[name]
            for ix in range(len(xvals)):
                jacobian.set(ix, ip, df[ix])  

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(StretchedExpFT)
