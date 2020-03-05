# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

"""
@author Jose Borreguero, NScD
@date July 19, 2017

This module provides functionality common to classes StretchedExpFT and PrimStretchedExpFT
"""

from __future__ import (absolute_import, division, print_function)
import copy
from scipy.fftpack import fft, fftfreq
from scipy.special import gamma
from scipy import constants
import numpy as np


def fillJacobian(function, xvals, jacobian, partials):
    """Fill the jacobian object with the dictionary of partial derivatives
    :param function: instance of StretchedExpFT or PrimStretchedExpFT
    :param xvals: domain where the derivatives are to be calculated
    :param jacobian: object to store partial derivatives with respect to fitting parameters
    :param partials: dictionary with partial derivates with respect to the
    fitting parameters
    """
    # Return zero derivatives if empty object
    if not partials:
        for ip in range(len(function._parmList)):
            for ix in range(len(xvals)):
                jacobian.set(ix, ip, 0.0)
    else:
        for ip in range(len(function._parmList)):
            name = function._parmList[ip]
            pd = partials[name]
            for ix in range(len(xvals)):
                jacobian.set(ix, ip, pd[ix])


def functionDeriv1D(function, xvals, jacobian):
    """Numerical derivative except for Height parameter
    :param function: instance of StretchedExpFT or PrimStretchedExpFT
    :param xvals: energy domain
    :param jacobian: object to store partial derivatives with respect to fitting parameters
    """
    # partial derivatives with respect to the fitting parameters
    partials = {}
    p = function.validateParams()
    if not p:
        function.fillJacobian(xvals, jacobian, {})
        return
    f0 = function.function1D(xvals)
    # Add these quantities to original parameter values
    dp = {'Tau': 1.0,  # change by 1ps
          'Beta': 0.01,
          'Centre': 0.0001  # change by 0.1 micro-eV
          }
    for name in dp.keys():
        pp = copy.copy(p)
        pp[name] += dp[name]
        partials[name] = (function.function1D(xvals, **pp) - f0) / dp[name]
    # Analytical derivative for Height parameter. Note we don't use
    # f0/p['Height'] in case p['Height'] was set to zero by the user
    pp = copy.copy(p)
    pp['Height'] = 1.0
    partials['Height'] = function.function1D(xvals, **pp)
    function.fillJacobian(xvals, jacobian, partials)


def init(function):
    """Declare parameters that participate in the fitting
    :param function: instance of StretchedExpFT or PrimStretchedExpFT
    """
    # Active fitting parameters
    function.declareParameter('Height', 0.1, 'Intensity at the origin')
    function.declareParameter('Tau', 100.0, 'Relaxation time')
    function.declareParameter('Beta', 1.0, 'Stretching exponent')
    function.declareParameter('Centre', 0.0, 'Centre of the peak')
    # Keep order in which parameters are declared. Should be a class
    # variable but we initialize it just below parameter declaration
    function._parmList = ['Height', 'Tau', 'Beta', 'Centre']


def validateParams(function):
    """Check parameters are positive
    :param function: instance of StretchedExpFT or PrimStretchedExpFT
    :return: dictionary of validated parameters
    """
    height = function.getParameterValue('Height')
    tau = function.getParameterValue('Tau')
    beta = function.getParameterValue('Beta')
    Centre = function.getParameterValue('Centre')
    for value in (height, tau, beta):
        if value <= 0:
            return None
    return {'Height': height, 'Tau': tau, 'Beta': beta, 'Centre': Centre}


surrogates = {'fillJacobian': fillJacobian,
              'functionDeriv1D': functionDeriv1D,
              'init': init,
              'validateParams': validateParams, }


def surrogate(method):
    """
    Decorator that replaces the passed method with a function
    of the same name defined in this module
    :param method: method of class StretchedExpFT or PrimStretchedExpFT
    :return: replacing function
    """
    return surrogates[method.__name__]


def function1Dcommon(function, xvals, refine_factor=16, **optparms):
    """Fourier transform of the symmetrized stretched exponential
    :param function: instance of StretchedExpFT or PrimStretchedExpFT
    :param xvals: energy domain
    :param refine_factor: divide the natural energy width by this value
    :param optparms: optional parameters used when evaluating the numerical derivative
    :return: parameters, energy width, energies, and function values
    """
    planck_constant = constants.Planck / constants.e * 1E15  # meV*psec
    p = function.validateParams()
    if p is None:
        # return zeros if parameters not valid
        return p, None, None, np.zeros(len(xvals), dtype=float)
    # override with optparms (used for the numerical derivative)
    if optparms:
        for name in optparms.keys():
            p[name] = optparms[name]

    ne = len(xvals)
    # energy spacing. Assumed xvals is a single-segment grid
    # of increasing energy values
    de = (xvals[-1] - xvals[0]) / (refine_factor * (ne - 1))
    erange = 2 * max(abs(xvals))
    dt = 0.5 * planck_constant / erange  # spacing in time
    tmax = planck_constant / de  # maximum reciprocal time
    # round to an upper power of two
    nt = 2 ** (1 + int(np.log(tmax / dt) / np.log(2)))
    sampled_times = dt * np.arange(-nt, nt)
    decay = np.exp(-(np.abs(sampled_times) / p['Tau']) ** p['Beta'])
    # The Fourier transform introduces an extra factor exp(i*pi*E/de),
    # which amounts to alternating sign every time E increases by de,
    # the energy bin width. Thus, we take the absolute value
    fourier = np.abs(fft(decay).real)  # notice the reverse of decay array
    fourier /= fourier[0]  # set maximum to unity
    # Normalize the integral in energies to unity
    fourier *= 2*p['Tau']*gamma(1./p['Beta']) / (p['Beta']*planck_constant)
    # symmetrize to negative energies
    fourier = np.concatenate(
        [fourier[nt:], fourier[:nt]])  # increasing ordering
    # Find energy values corresponding to the fourier values
    energies = planck_constant * fftfreq(2 * nt, d=dt)  # standard ordering
    energies = np.concatenate(
        [energies[nt:], energies[:nt]])  # increasing ordering
    return p, de, energies, fourier
