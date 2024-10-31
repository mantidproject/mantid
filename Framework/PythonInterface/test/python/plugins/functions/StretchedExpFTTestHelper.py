# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from scipy import constants

from mantid.simpleapi import Fit, CreateWorkspace
from mantid.api import mtd, FunctionFactory

planck_constant = constants.Planck / constants.e * 1e15  # meV*psec


def createData(functor, startX=-0.1, endX=0.5, de=0.0004):
    """Generate data for the fit
    :param startX: lower boundary for the data, in meV
    :param endX: upper boundary for the data, in meV
    :param de: energy bin, in meV
    """
    energies = np.arange(startX, endX, de)
    data = functor(energies)
    background = 0.01 * max(data)
    data += background
    errorBars = data * 0.1
    return CreateWorkspace(energies, data, errorBars, UnitX="DeltaE", OutputWorkspace="data")


def cleanFit():
    """Removes workspaces created during the fit"""
    mtd.remove("data")
    mtd.remove("fit_NormalisedCovarianceMatrix")
    mtd.remove("fit_Parameters")
    mtd.remove("fit_Workspace")


def assertFit(workspace, tg):
    """

    :param workspace: data MatrixHistogram
    :param tg: dictionary of target fitting parameters
    :return:
    """
    unacceptable_chi_square = 1.0
    msg = ""
    for irow in range(workspace.rowCount()):
        row = workspace.row(irow)
        name = row["Name"]
        if name == "Cost function value":
            chi_square = row["Value"]
            msg += " chi_square=" + str(chi_square)
        elif name == "f0.Tau":
            tauOptimal = row["Value"]
            msg += " tauOptimal=" + str(tauOptimal)
        elif name == "f0.Beta":
            betaOptimal = row["Value"]
            msg += " betaOptimal=" + str(betaOptimal)
        elif name == "f0.Height":
            heightOptimal = row["Value"]
            msg += " heightOptimal=" + str(heightOptimal)
    cleanFit()
    beta = tg["beta"]
    height = tg["height"]
    check = (
        (chi_square < unacceptable_chi_square)
        and (abs(height - heightOptimal) / height < 0.01)
        and (abs(beta - betaOptimal) < 0.01)
        and (abs(beta - betaOptimal) < 0.01)
    )
    return check, msg


def isregistered(function):
    status, msg = True, ""
    try:
        FunctionFactory.createFunction(function)
    except RuntimeError as exc:
        status, msg = False, "Could not create {} function: {}".format(function, str(exc))
    return status, msg


def do_fit(tg, fString, shape):
    """
    Given a target shape and initial fit function guess, carry out the fit
    :param tg: dictionary of target fitting parameters
    :param fString: initial guess of the fit function
    :param shape: Gaussian or Lorentzian, either integrated or not
    :return: success or failure of the fit
    """
    if "Gaussian" in shape:
        E0 = planck_constant / tg["tau"]

        # Analytical Fourier transform of exp(-(t/tau)**2)
        def functor(E):
            return np.sqrt(np.pi) / E0 * np.exp(-((np.pi * E / E0) ** 2))
    elif "Lorentzian" in shape:
        hwhm = planck_constant / (2 * np.pi * tg["tau"])

        # Analytical Fourier transform of exp(-t/tau)
        def functor(E):
            return (1.0 / np.pi) * hwhm / (hwhm**2 + E**2)

    if "Integrated" in shape:
        # when testing function PrimStretchedExpFT
        def ifunctor(E):
            """Numerical integral of the functor within each energy bin"""
            de = (E[-1] - E[0]) / (len(E) - 1.0)  # energy spacing
            rf = 100  # make the energy domain a grid 100 times finer
            efine = np.arange(E[0] - de, E[-1] + 2 * de, de / rf)
            values = functor(efine)  # evaluate on the finer grid
            primitive = np.cumsum(values) / rf  # cummulative sum, giving the integral
            # bb are bin boundaries delimiting bins of width de and centered at the E values
            bb = (E[1:] + E[:-1]) / 2  # internal bin boundaries
            bb = np.insert(bb, 0, 2 * E[0] - bb[0])  # external lower bin boundary
            bb = np.append(bb, 2 * E[-1] - bb[-1])  # external upper bin boundary
            # return the integral over each energy bin
            return np.interp(bb[1:], efine, primitive) - np.interp(bb[:-1], efine, primitive)

        createData(ifunctor)
    else:
        # when testing function StretchedExpFT
        createData(functor)  # Create workspace "data"
    Fit(Function=fString, InputWorkspace="data", MaxIterations=100, Output="fit")
    return assertFit(mtd["fit_Parameters"], tg)
