# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

'''
@author Jose Borreguero, NScD
@date October 06, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
from __future__ import (absolute_import, division, print_function)
import copy
import numpy as np
import scipy.constants
from scipy.fftpack import fft, fftfreq
from scipy.special import gamma
from mantid.api import IFunction1D, FunctionFactory


class PrimStretchedExpFT(IFunction1D):
    # Class variables
    _planck_constant = scipy.constants.Planck/scipy.constants.e*1E15  # meV*psec

    # pylint: disable=super-on-old-class
    def __init__(self):
        """declare some constants"""
        super(PrimStretchedExpFT, self).__init__()
        self._parmList = list()

    def category(self):
        return 'QuasiElastic'

    def init(self):
        """Declare parameters that participate in the fitting"""
        # Active fitting parameters
        self.declareParameter('Height', 0.1, 'Intensity at the origin')
        self.declareParameter('Tau', 100.0, 'Relaxation time')
        self.declareParameter('Beta', 1.0, 'Stretching exponent')
        self.declareParameter('Centre', 0.0, 'Centre of the peak')
        # Keep order in which parameters are declared. Should be a class
        # variable but we initialize it just below parameter declaration
        # to make sure we follow the order.
        self._parmList = ['Height', 'Tau', 'Beta', 'Centre']

    def validateParams(self):
        """Check parameters are positive"""
        height = self.getParameterValue('Height')
        tau = self.getParameterValue('Tau')
        beta = self.getParameterValue('Beta')
        Centre = self.getParameterValue('Centre')

        for _, value in {'Height': height, 'Tau': tau, 'Beta': beta}.items():
            if value <= 0:
                return None
        return {'Height': height, 'Tau': tau, 'Beta': beta, 'Centre': Centre}

    def function1D(self, xvals, **optparms):
        """ Fourier transform of the symmetrized stretched exponential integrated
        within each energy bin.

        The Symmetrized Stretched Exponential:
                height * exp( - |t/tau|**beta )

        The Fourier Transform:
            F(E) \int_{-infty}^{infty} (dt/h) e^{-i2\pi Et/h} f(t)
            F(E) is normalized:
                \int_{-infty}^{infty} dE F(E) = 1

        Let P(E) be the primitive of F(E) from minus infinity to E, then for element i of
        xvals we compute:
            1. bin_boundaries[i] = (xvals[i]+xvals[i+1])/2
            3. P(bin_boundaries[i+1])- P(bin_boundaries[i])
        :param xvals: list of values where to evaluate the function
        :param optparms: alternate list of function parameters
        :return: P(bin_boundaries[i+1])- P(bin_boundaries[i]), the difference of the primitive
        """
        p = self.validateParams()
        if not p:
            # return zeros if parameters not valid
            return np.zeros(len(xvals), dtype=float)
        # override with optparms (used for the numerical derivative)
        if optparms:
            for name in optparms.keys():
                p[name] = optparms[name]

        ne = len(xvals)
        # energy spacing. Assumed xvals is a single-segment grid
        # of increasing energy values
        refine_factor = 16.0
        de = (xvals[-1] - xvals[0]) / (refine_factor*(ne-1))
        erange = 2*max(abs(xvals))
        dt = 0.5*PrimStretchedExpFT._planck_constant/erange  # spacing in time
        tmax = PrimStretchedExpFT._planck_constant/de  # maximum reciprocal time
        # round to an upper power of two
        nt = 2**(1+int(np.log(tmax/dt)/np.log(2)))
        sampled_times = dt * np.arange(-nt, nt)
        decay = np.exp(-(np.abs(sampled_times)/p['Tau'])**p['Beta'])
        # The Fourier transform introduces an extra factor exp(i*pi*E/de),
        # which amounts to alternating sign every time E increases by de,
        # the energy bin width. Thus, we take the absolute value
        fourier = np.abs(fft(decay).real)  # notice the reverse of decay array
        fourier /= fourier[0]  # set maximum to unity
        # Normalize the integral in energies to unity
        fourier *= 2*p['Tau']*gamma(1./p['Beta'])/(p['Beta']*PrimStretchedExpFT._planck_constant)
        # symmetrize to negative energies
        fourier = np.concatenate([fourier[nt:], fourier[:nt]])  # increasing ordering
        # Find corresponding energies
        energies = PrimStretchedExpFT._planck_constant * fftfreq(2*nt, d=dt)  # standard ordering
        energies = np.concatenate([energies[nt:], energies[:nt]])  # increasing ordering
        denergies = (energies[-1] - energies[0]) / (len(energies)-1)
        # Find bin boundaries
        boundaries = (xvals[1:]+xvals[:-1])/2  # internal bin boundaries
        boundaries = np.insert(boundaries, 0, 2*xvals[0]-boundaries[0])  # external lower boundary
        boundaries = np.append(boundaries, 2*xvals[-1]-boundaries[-1])  # external upper boundary
        primitive = np.cumsum(fourier) * (denergies/(refine_factor*de))  # running Riemann sum
        transform = np.interp(boundaries[1:] - p['Centre'], energies, primitive) - \
            np.interp(boundaries[:-1] - p['Centre'], energies, primitive)
        return transform*p['Height']

    def fillJacobian(self, xvals, jacobian, partials):
        """Fill the jacobian object with the dictionary of partial derivatives
        :param xvals: domain where the derivatives are to be calculated
        :param jacobian: jacobian object
        :param partials: dictionary with partial derivates with respect to the
        fitting parameters
        """
        # Return zero derivatives if empty object
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
        """Numerical derivative except for Height parameter"""
        # partial derivatives with respect to the fitting parameters
        partials = {}
        p = self.validateParams()
        if not p:
            self.fillJacobian(xvals, jacobian, {})
            return
        f0 = self.function1D(xvals)
        # Add these quantities to original parameter values
        dp = {'Tau': 1.0,  # change by 1ps
              'Beta': 0.01,
              'Centre': 0.0001  # change by 0.1 micro-eV
              }
        for name in dp.keys():
            pp = copy.copy(p)
            pp[name] += dp[name]
            partials[name] = (self.function1D(xvals, **pp) - f0) / dp[name]
        # Analytical derivative for Height parameter. Note we don't use
        # f0/p['Height'] in case p['Height'] was set to zero by the user
        pp = copy.copy(p)
        pp['Height'] = 1.0
        partials['Height'] = self.function1D(xvals, **pp)
        self.fillJacobian(xvals, jacobian, partials)

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(PrimStretchedExpFT)
