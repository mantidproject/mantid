# pylint: disable=invalid-name
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

from mantid.api import IFunction1D, FunctionFactory
from mantid import logger
import numpy as np
import copy
import scipy.fftpack
import scipy.interpolate
import scipy.constants
import h5py
import os
from pdb import set_trace as tr

class StretchedExpFT(IFunction1D):

    # Class variables
    _modeValues = ('Tabulated', 'FFT')
    _planck_constant = scipy.constants.Planck/scipy.constants.e*1E15  # meV*psec
    _hbar = _planck_constant/(2*np.pi)
    _defaultTablesFile = "StretchedExpFTTables.npz"

    # pylint: disable=super-on-old-class
    def __init__(self):
        """declare some constants"""
        super(StretchedExpFT, self).__init__()
        self._mode = None  # selects which mode of calculation to carry out
        self._betas = np.empty(0)  # values of beta for the tables
        self._reduced_energy_values = np.empty(0)  # values of reduced energy for the tables
        self._tabled_values = np.empty(0)  # shape=(len(betas), len(uvals)) function values
        self._parmList = list()

    def category(self):
        return 'QuasiElastic'

    def init(self):
        """Declare parameters that participate in the fitting"""
        # Active fitting parameters
        self.declareParameter('Height', 0.1, "Intensity at the center")
        self.declareParameter('Tau', 100.0, "Relaxation time of the standard exponential")
        self.declareParameter('Beta', 1.0, "Stretching exponent")
        self.declareParameter('Center', 0.0, "Center of the peak")
        # Keep order in which parameters are declared
        self._parmList = ['Height', 'Tau', 'Beta', 'Center']
        # Attributes
        self.declareAttribute('Mode', 'Tabulated')
        self.declareAttribute('TablesFile', "")

    def init_tables(self, fileName):
        """Load tabulated function from HDF5 file
        :pre datasets with following names: 'betas', 'reduced_energies', and 'tables'
        function values are divided by hbar in this initialization
        """
        try:
            handle = h5py.File(fileName, "r")
            sought_sets = set(['betas','reduced_energies', 'tables'])
            stored_sets = set([name for name in handle])
            if sought_sets <= stored_sets:
                rev = self._reduced_energy_values
                tv =  self._tabled_values
                self._betas = handle['betas']
                rev = handle['reduced_energies']
                # Divide by hbar!
                tv = handle['tables'] / self._hbar
                if rev.shape != tv.shape or len(rev) != len(betas):
                    handle.close()
                    msg = "datasets in {0} have inconsistent dimensions".format(fileName)
                    raise ValueError(msg)
                # Tables are defined only for semi-positive energies
                rev = np.concatenate([rev[::-1],rev[1:]]) # extend to E<0
                tv = np.concatenate(tv[:,::-1], tv[:,1:]) # symmetric function
            else:
                handle.close()
                raise KeyError("{0} contains no valid tables".format(filename))
            handle.close()
        except:
            raise IOError("{0} cannot be read".format(fileName))


    def setAttributeValue(self, name, value):
        if name == 'Mode':
            if value not in StretchedExpFT._modeValues:
                raise ValueError("{0} is not a valid mode".format(value))
            self._mode = value
        elif name == 'TablesFile':
            if not value:
                value = StretchedExpFT._defaultTablesFile
                self.init_tables(value)

    def validateParams(self):
        """Check parameters are positive"""
        height = self.getParameterValue('Height')
        tau = self.getParameterValue('Tau')
        beta = self.getParameterValue('Beta')
        center = self.getParameterValue('Center')
        if not self._mode:
            self.setAttributeValue('Mode', self.getAttributeValue('Mode'))
        if self._mode == 'FFT':
            for name, value in {'Height': height, 'Tau': tau, 'Beta': beta}.items():
                if value <= 0:
                    message = 'Parameter {} in StretchedExpFT must be positive. Got {} instead'.format(name, str(value))
                    logger.error(message)
                    return None
        elif self._mode == 'Tabulated':
            if height < 0 or tau <= 0:
                return None
            b = StretchedExpFtTabulated._betas
            if beta < b[0] or beta > b[-1]:
                return None  # Out of tabulated beta values
        return {'Height': height, 'Tau': tau, 'Beta': beta, 'Center': center}

    def function1D(self, xvals, **optparms):
        p = self.validateParams()
        if not p:
            # function goes to zero at high energy values
            return np.zeros(len(xvals), dtype=float)
        if self._mode == 'FFT':
            """ Computes the Fourier transform of the Symmetrized Stretched Exponential

            The Symmetrized Stretched Exponential:
                    Fourier{ height * exp( - |t/tau|**beta ) }

            Given a time step dt and M=2*N+1 time points (N negative, one at zero, N positive),
            then fft will sample frequencies [0, 1/(M*dt), N/(M*dt), -N/(M*dt), (-N+1)/(M*dt),..,1/(M*dt)].

            Given xvals, let:
                1/(M*dt) = xvals[1]-xvals[0]
                N/(M*dt) = max(abs(xvals))
            Thus:
                N = max(abs(xvals)) / (xvals[1]-xvals[0])
                dt = 1/[M*(xvals[1]-xvals[0])]  # M=2*N+1

            Its Fourier transform is real by definition, thus we return the real part
            of the Fast Fourier Transform (FFT). The FFT step is meant to produce
            some residual imaginary part due to numerical inaccuracies which we ignore.

            We take the absolute value of the real part. The Fourier transform introduces
            an extra factor exp(i*pi*E/de) which amounts to alternating sign every
            time E increases by de, the energy bin width
            """
            # override parameter values with optparms (used for the numerical derivative)
            if optparms:
                if set(self._parmList).issubset(set(optparms.keys())):
                    for name in self._parmList:
                        p[name] = optparms[name]

            de = (xvals[1] - xvals[0]) / 2  # increase the long-time range, increase the low-frequency resolution
            emax = max(abs(xvals))
            nmax = 16000  # increase short-times resolution, increase the resolution of the structure factor tail
            while (emax / de) < nmax:
                emax *= 2
            N = int(emax / de)
            # extent to negative times and t==0
            dt = (float(N) / (2 * N + 1)) * (StretchedExpFT._planck_constant / emax)
            sampled_times = dt * np.arange(-N, N + 1)  # len( sampled_times ) < 64000
            exponent = -(np.abs(sampled_times) / p['Tau']) ** p['Beta']
            freqs = de * np.arange(-N, N + 1)
            fourier = p['Height'] * np.abs(scipy.fftpack.fft(np.exp(exponent)).real)
            fourier = np.concatenate((fourier[N + 1:], fourier[0:N + 1]))
            interpolator = scipy.interpolate.interp1d(freqs, fourier)
            fourier = interpolator(xvals - p['Center'])
            return fourier
        elif self._mode == 'Tabulated':
            # Render the energies to their reduced values
            inverse_energy_barrier = 2*np.pi*p['Tau']/StretchedExpFT._planck_constant
            reduced_energies = (xvals - p['Center']) * inverse_energy_barrier
            # Handy shortcuts
            tabled = self._tabled_values
            betas = self._betas
            energies = self._reduced_energy_values
            # Find the tabulated beta (its index) just smaller than p['Beta']
            index_to_closest = np.abs(betas-p['Beta']).argmin()
            if betas[index_to_closest] > p['Beta']:
                index_to_closest -= 1
            # tables for beta values bounding p['Beta']
            # "U" stands for reduced energies, and "B" for beta
            f_Ui_Bj = tabled[index_to_closest]  # beta smaller than p['Beta']
            f_Ui_Bj2 = tabled[index_to_closest + 1]
            # interpolate function values in reduced energies
            f_U_Bj = interp(reduced_energies, energies, f_Ui_Bj)
            f_U_Bj2 = interp(reduced_energies, energies, f_Ui_Bj2)
            # table of the partial derivative with respect to beta
            delta_beta = betas[index_to_closest+1] - betas[index_to_closest]
            df_dB_U_B = (f_U_Bj2 - f_U_Bj) / delta_beta
            # interpolate function values in beta
            f_U_B = f_U_Bj + df_dB_U_B * (p['Beta'] - betas[index_to_closest])
            return p['Height'] * p['Tau'] * f_U_B

    def fillJacobian(self, xvals, jacobian, partials):
        """Fill the jacobian object with the dictionary of partial derivatives
        :param xvals: domain where the derivatives are to be calculated
        :param jacobian: jacobian object
        :param partials: dictionary with partial derivates with respect to the
        fitting parameters
        """
        # Return zero derivatives if empty derivatives
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
        # dictionary of partial derivatives with respect to the fitting parameters
        partials = {}
        p = self.validateParams()
        if not p:
            self.fillJacobian(jacobian, {})
        partials = {}  # stores the derivatives
        # Calculate numerical derivatives in the FFT mode
        if self._mode == 'FFT':
            """Numerical derivative"""
            f0 = self.function1D(xvals)
            dp = {}
            for (key, val) in p.items():
                dp[key] = 0.1 * val  # modify by ten percent
                if val == 0.0:
                    dp[key] = 1E-06  # some small, supposedly, increment
            for name in self._parmList:
                pp = copy.copy(p)
                pp[name] += dp[name]
                partials[name] = (self.function1D(xvals, **pp) - f0) / dp[name]
        # Calculate analytic derivatives in the Tabulated mode
        elif self._mode == 'Tabulated':
            # Render the energies to their reduced mode
            inverse_energy_barrier = 2*np.pi*p['tau']/StretchedExpFT._planck_constant
            reduced_energies = (xvals - p['Center']) * inverse_energy_barrier
            # Handy shortcuts
            tabled = self._tabled_values
            betas = self._betas
            energies = self._reduced_energy_values
            # Find the tabulated beta (its index) just smaller than p['Beta']
            index_to_closest = np.abs(betas-p['Beta']).argmin()
            if betas[index_to_closest] > p['Beta']:
                index_to_closest -= 1
            # tables for beta values bounding p['Beta']
            # "U" stands for reduced energies, and "B" for beta
            f_Ui_Bj = tabled[index_to_closest]  # beta smaller than p['Beta']
            f_Ui_Bj2 = tabled[index_to_closest + 1]
            # interpolate function values in reduced energies
            f_U_Bj = interp(reduced_energies, energies, f_Ui_Bj)
            f_U_Bj2 = interp(reduced_energies, energies, f_Ui_Bj2)
            # table of the partial derivative with respect to beta
            delta_beta = betas[index_to_closest+1] - betas[index_to_closest]
            df_dB_U_B = (f_U_Bj2 - f_U_Bj) / delta_beta
            # interpolate function values in beta
            f_U_B = f_U_Bj + df_dB_U_B * (p['Beta'] - betas[index_to_closest])
            # dictionary of partial derivatives with respect to the fitting parameters
            partials = {}
            # Derivative with respect to Height
            partials['Height'] = p['Tau'] * f_U_B
            # Derivative with respect to Beta
            partials['Beta'] = p['Height'] * p['Tau'] * df_dB_U_B
            # Derivative of the tables with respect to the reduced energies
            df_dU_U_B = (f_U_B[1:] - f_U_B[:-1]) / (energies[1:] - energies[:-1])
            df_dU_U_B.append(df_dU_U_B[-1])
            # Derivative with respect to Tau
            partials['Tau'] = p['Height'] * (f_U_B + reduced_energies * df_dU_U_B)
            # Derivative with respect to Center of the energies
            partials['Center'] = p['Height'] * p['Tau'] * inverse_energy_barrier * df_dU_U_B
        # Fill the Jacobian object
        self.fillJacobian(xvals, jacobian, partials)

FunctionFactory.subscribe(StretchedExpFT)  # Required to have Mantid recognise the new function
