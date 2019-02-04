# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest

from StretchedExpFTTestHelper import isregistered, do_fit

class StretchedExpFTTest(unittest.TestCase):

    def testRegistered(self):
        self.assertTrue(*isregistered('StretchedExpFT'))

    def testGaussian(self):
        """ Test the Fourier transform of a gaussian is a Gaussian"""
        # Target parameters
        tg = {'tau': 20.0,    # picoseconds
              'beta': 2.0,    # gaussian
              'height': 1.0  # We want identity, not just proporcionality
              }
        # Initial guess reasonably far from target parameters
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Centre=0.0002;" +\
                  "name=FlatBackground,A0=0.0"
        # Carry out the fit
        self.assertTrue(*do_fit(tg, fString, 'Gaussian'))

    def testLorentzian(self):
        """ Test the Fourier transform of a exponential is a Lorentzian"""
        # Target parameters
        tg = {'tau': 100.0,    # picoseconds
              'beta': 1.0,    # exponential
              'height': 1.0  # We want identity, not just proporcionality
              }
        # Initial guess reasonably far from target parameters
        fString = "name=StretchedExpFT,Height=3.0,Tau=50,Beta=1.5,Centre=0.0002;" +\
                  "name=FlatBackground,A0=0.0"
        # carry out the fit
        self.assertTrue(*do_fit(tg, fString, 'Lorentzian'))


if __name__ == '__main__':
    unittest.main()

