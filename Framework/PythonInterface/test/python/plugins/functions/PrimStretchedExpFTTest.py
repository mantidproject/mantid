# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from StretchedExpFTTestHelper import isregistered, do_fit


class PrimStretchedExpFTTest(unittest.TestCase):
    def testRegistered(self):
        self.assertTrue(*isregistered("PrimStretchedExpFT"))

    def testGaussian(self):
        """Test PrimStretchedExpFT against the binned-integrated of
        the Fourier transform of a gaussian
        """
        # Target parameters
        tg = {"tau": 20.0, "beta": 2.0, "height": 1.0}  # picoseconds  # gaussian  # We want identity, not just proporcionality
        # Initial guess reasonably far from target parameters
        fString = "name=PrimStretchedExpFT,Height=2.0,Tau=30,Beta=1.5,Centre=0.0002;" + "name=FlatBackground,A0=0.0"
        # Carry out the fit
        self.assertTrue(*do_fit(tg, fString, "GaussianIntegrated"))

    def testLorentzian(self):
        """Test PrimStretchedExpFT against the binned-integrated of
        the Fourier transform of a exponential
        """
        # Target parameters
        tg = {"tau": 100.0, "beta": 1.0, "height": 1.0}  # picoseconds  # exponential  # We want identity, not just proporcionality
        # Initial guess reasonably far from target parameters
        fString = "name=PrimStretchedExpFT,Height=2.0,Tau=300,Beta=1.5,Centre=0.0002;" + "name=FlatBackground,A0=0.0"
        # carry out the fit
        self.assertTrue(*do_fit(tg, fString, "LorentzianIntegrated"))


if __name__ == "__main__":
    unittest.main()
