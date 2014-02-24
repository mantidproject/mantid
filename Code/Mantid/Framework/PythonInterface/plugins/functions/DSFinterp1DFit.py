'''*WIKI* 


*WIKI*
    
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
import copy

from pdb import set_trace as tr

class DSFinterp1DFit(IFunction1D):

  def __init__(self):
    '''declare some constants'''
    super(DSFinterp1DFit, self).__init__()
    self._regressionTypes = set(['linear','quadratic']) #valid syntaxfor python >= 2.6
    self._minWindow = { 'linear':3, 'quadratic':4 }

  def category(self):
    return 'QENS'

  def init(self):
    '''Declare parameters and attributes that participate in the fitting'''
    # Active fitting parameters
    self.declareParameter('Intensity', 1.0, 'Intensity')
    self.declareParameter('TargetParameter', 1.0, 'Target value of the structure factor parameter')

    self.declareAttribute('Workspaces','')
    self.declareAttribute('LoadErrors', False)
    self.declareAttribute('ParameterValues')
    self.declareAttribute('LocalRegression', True)
    self.declareAttribute('RegressionType', 'quadratic')
    self.declareAttribute('RegressionWindow', 6)

  def setAttributeValue(self, name, value):
    if name == "Workspaces":
      self._Workspaces = value.split()
      if ',' in value:
        self._Workspaces = [x.strip() for x in value.split(',')]
    elif name == 'ParameterValues':
      self._ParameterValues = [ float(f) for f in value.split() ]
      if len(self._ParameterValues) != len(self._Workspaces):
        message = 'Number of Workspaces and ParameterValues should be the same. Found {0} and {1}, respectively'.format(len(self._ParameterValues), len(self._Workspaces))
        logger.error(message)
        raise ValueError
      self._fmin = min(self._ParameterValues)
      self._fmax = max(self._ParameterValues)
    elif name == 'LocalRegression':
      self._localRegression = bool(value)
    elif self._localRegression and name == 'RegressionType':
      self._regressionType = value.lower()
      if self._regressionType not in self._regressionTypes:
        message = 'Regression type {0} not implemented. choose one of {1}'.format(value, ', '.join(self._regressionTypes))
        logger.error(message)
        raise NotImplementedError
    elif self._localRegression and name == 'RegressionWindow':
      if value < self._minWindow[self._regressionType]:
        message = 'RegressionWindow must be equal or bigger than {0} for regression type {1}'.format(self._minWindow[self._regressionType], self._regressionType)
        logger.error(message)
        raise ValueError

  def validateParams(self):
    '''Check parameters are positive'''
    height = self.getParameterValue('height')
    if height <=0:
      message = 'Parameter height in DSFinterp1DFit must be positive. Got {0} instead'.format(height)
      logger.error(message)
      return None
    f = self.getParameterValue('f')
    if f < self_fmin or f > self._fmax:
      message = 'TargetParameter is out of bounds [{0}, {1}]'.format(self._fmin, self._fmax)
      logger.error(message)
      return None
    return {'height':h, 'f':f}

  def function1D(self, xvals, **optparms):
    ''' Does something :)
    '''
        import scipy.fftpack
        import scipy.interpolate

        p=self.validateParams()
        if not p:
            return np.zeros(len(xvals), dtype=float) # return zeros if parameters not valid
        # override parameter values with optparms (used for the numerical derivative)
        if optparms:
            if self._parmset.issubset( set(optparms.keys()) ):
                for name in self._parmset: p[name] = optparms[name]

        de = (xvals[1]-xvals[0]) / 2 # increase the long-time range, increase the low-frequency resolution
        emax = max( abs(xvals) )
        Nmax = 16000 # increase short-times resolution, increase the resolution of the structure factor tail
        while ( emax / de ) < Nmax:
            emax = 2 * emax
        N = int( emax / de )
        dt = ( float(N) / ( 2 * N + 1 ) ) * (self._h / emax)  # extent to negative times and t==0
        sampled_times = dt * np.arange(-N, N+1) # len( sampled_times ) < 64000
        exponent = -(np.abs(sampled_times)/p['tau'])**p['beta']
        freqs = de * np.arange(-N, N+1)
        fourier = p['height']*np.abs( scipy.fftpack.fft( np.exp(exponent) ).real )
        fourier = np.concatenate( (fourier[N+1:],fourier[0:N+1]) )
        interpolator = scipy.interpolate.interp1d(freqs, fourier)
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
FunctionFactory.subscribe(DSFinterp1DFit)
