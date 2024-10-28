# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-arguments,too-many-locals

"""
Bayes routines
Fortran programs use fixed length arrays whereas Python has variable length lists
Input : the Python list is padded to Fortrans length using procedure pad_array
Output : the Fortran numpy array is sliced to Python length using dataY = yout[:ny]
"""

from mantid.api import mtd
from IndirectCommon import pad_array
import numpy as np


def CalcErange(inWS, ns, erange, binWidth):
    # length of array in Fortran
    array_len = 4096

    binWidth = int(binWidth)
    bnorm = 1.0 / binWidth

    # get data from input workspace
    _, X, Y, E = GetXYE(inWS, ns, array_len)
    Xdata = mtd[inWS].readX(0)

    # get all x values within the energy range
    rangeMask = (Xdata >= erange[0]) & (Xdata <= erange[1])
    Xin = Xdata[rangeMask]

    # get indices of the bounds of our energy range
    minIndex = np.where(Xdata == Xin[0])[0][0] + 1
    maxIndex = np.where(Xdata == Xin[-1])[0][0]

    # reshape array into sublists of bins
    Xin = Xin.reshape((len(Xin) // binWidth, binWidth))

    # sum and normalise values in bins
    Xout = [sum(bin_val) * bnorm for bin_val in Xin]

    # count number of bins
    nbins = len(Xout)

    nout = [nbins, minIndex, maxIndex]

    # pad array for use in Fortran code
    Xout = pad_array(Xout, array_len)

    return nout, bnorm, Xout, X, Y, E


def GetXYE(inWS, n, array_len):
    Xin = mtd[inWS].readX(n)
    N = len(Xin) - 1  # get no. points from length of x array
    Yin = mtd[inWS].readY(n)
    Ein = mtd[inWS].readE(n)
    X = pad_array(Xin, array_len)
    Y = pad_array(Yin, array_len)
    E = pad_array(Ein, array_len)
    return N, X, Y, E
