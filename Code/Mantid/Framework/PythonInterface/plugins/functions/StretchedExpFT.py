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

class StretchedExpFT(IFunction1D):
    
    def category(self):
        return 'QENS'

    def init(self):
        '''Declare parameters that participate in the fitting'''
        # Active fitting parameters
        self.declareParameter('height', 1.0, 'Intensity at the origin')
        self.declareParameter('tau', 1.0, 'Relaxation time of the standard exponential')
        self.declareParameter('beta',1.0, 'Stretching exponent')
        #self.addConstraints() # constraints not yet exposed to python
        self.validateParams()
    
    '''
    def addConstraints(self):
        for name in ('height', 'tau', 'beta'):
            constraint = BoundaryConstraint(self, name, 0, True)
            self.addConstraint(constraint);
    '''

    def validateParams(self,):
        '''Check parameters are positive'''
        height = self.getParameterValue('height')
        tau = self.getParameterValue('tau')
        beta = self.getParameterValue('beta')
        for name,parm in {'height':height, 'tau':tau, 'beta':beta}.items():
            if parm <=0:
                message = 'Parameter {} in StretchedExpFT must be positive'.format(name)
                logger.error(message)
                raise ValueError(message)
        return {'height':height, 'tau':tau, 'beta':beta}
       
    def function1D(self, xvals):
        '''
        Computes the Fourier transform of the Symmetrized Stretched Exponential
        
        The Symmetrized Stretched Exponential:
                Fourier{ height * exp( - |t/tau|**beta ) }
        Its Fourier transform is real by definition, thus we return the real part
        of the Fast Fourier Transform (FFT). The FFT step is meant to produce
        some residual imaginary part due to numerical inaccuracies which we ignore
        '''
        p=self.validateParams()
        exponent = -(np.abs(xvals)/p['tau'])**p['beta']
        return p['height']*fft( np.exp(exponent) ).real
    
    '''The derivative of the symmetrized stretched exponential is ill-defined at
    the origin. Thus, we prefer the numerical derivative which will return a
    near-zero derivative close to the origin
    def functionDeriv1D(self, xvals, jacobian):
        pass
    '''

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(StretchedExpFT)
