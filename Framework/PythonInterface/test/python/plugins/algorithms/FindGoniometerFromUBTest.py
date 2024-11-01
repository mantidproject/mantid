# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
# NScD Oak Ridge National Laboratory, European Spallation Source
# & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
# System test that loads short SXD numors and runs LinkedUBs.
from __future__ import absolute_import, division, print_function
import unittest
from testhelpers import create_algorithm
from mantid.simpleapi import CreateSampleWorkspace, FindGoniometerFromUB, LoadIsawUB
import numpy as np


class FindGoniometerFromUB_Test(unittest.TestCase):
    def runTest(self):
        ubList = ["WISH00043350", "WISH00043351", "WISH00043353"]
        chiRef = 45.0
        chiTol = 5
        phiTol = 10
        phiHand = -1
        dOmega = 0
        omegaHand = 1

        tab = FindGoniometerFromUB(
            UBfiles=",".join(ubList), chi=chiRef, chiTol=chiTol, phiTol=phiTol, phiHand=phiHand, dOmega=dOmega, omegaHand=omegaHand
        )

        # compare the table values with expected
        chi = [46.02283477783203, 43.43744659423828]
        chiFound = tab.column(1)
        phi = [277.477294921875, 59.260677337646484]
        phiFound = tab.column(2)
        u = [[-0.103205, 0.708069, 0.694372], [-0.125878, 0.67274, 0.726125]]
        uFound = tab.column(3)
        for irun in range(0, len(chi)):
            print(chi[irun], chiFound[irun])
            self.assertAlmostEqual(chi[irun], chiFound[irun], delta=1e-5, msg="Discrepancy in chi for run {}.".format(ubList[irun]))
            self.assertAlmostEqual(phi[irun], phiFound[irun], delta=1e-5, msg="Discrepancy in phi for run {}.".format(ubList[irun]))
            for icomp in range(0, 3):
                self.assertAlmostEqual(
                    u[irun][icomp], uFound[irun][icomp], delta=1e-5, msg="Discrepancy in gonio axis for run {}.".format(ubList[irun])
                )

        # compare to the following UB
        correctUB = [
            np.array([[-0.0629347, -0.18008481, 0.14623738], [-0.11503921, 0.1722375, 0.07898539], [-0.29513327, -0.20543161, -0.0619714]]),
            np.array([[0.14322252, -0.12656543, 0.0974932], [0.27376461, 0.28443044, -0.00069533], [-0.09228303, 0.08400258, 0.14924592]]),
        ]
        # load the saved UBs using LoadIsawUB to check formatting is correct and compare
        tmpWS = CreateSampleWorkspace()
        for irun in range(1, len(ubList)):
            print(ubList[irun] + "_consistent.mat")
            LoadIsawUB(InputWorkspace=tmpWS, Filename=ubList[irun] + "_consistent.mat", CheckUMatrix=True)
            aUB = tmpWS.sample().getOrientedLattice().getUB()
            for ii in range(0, 3):
                for jj in range(0, 3):
                    self.assertAlmostEqual(
                        aUB[ii, jj], correctUB[irun - 1][ii, jj], delta=1e-5, msg="Discrepancy in UB file {}".format(ubList[irun])
                    )

    def test_InputValidation(self):
        alg = create_algorithm(
            "FindGoniometerFromUB",
            UBfiles="WISH00043350",
            chi=45,
            chiTol=5,
            phiTol=10,
            phiHand=-1,
            dOmega=0,
            omegaHand=1,
            OutputTable="test",
        )
        with self.assertRaisesRegex(RuntimeError, "At least two UB files are required"):
            alg.execute()


if __name__ == "__main__":
    unittest.main()
