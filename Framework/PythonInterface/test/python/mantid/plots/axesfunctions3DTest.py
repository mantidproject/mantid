# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
import unittest

import mantid.api
import mantid.plots.axesfunctions3D as funcs
from mantid.kernel import config
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, CreateMDHistoWorkspace, ConjoinWorkspaces


class PlotFunctions3DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.g1da = config["graph1d.autodistribution"]
        config["graph1d.autodistribution"] = "On"
        cls.ws2d_histo = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            Distribution=True,
            UnitX="Wavelength",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_histo",
        )
        cls.ws2d_histo_rag = CreateWorkspace(
            DataX=[1, 2, 3, 4, 5, 2, 4, 6, 8, 10],
            DataY=[2] * 8,
            NSpec=2,
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[5, 7, 9],
            OutputWorkspace="ws2d_histo_rag",
        )
        cls.ws_MD_2d = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-3,3,-10,10,-1,1",
            SignalInput=range(25),
            ErrorInput=range(25),
            NumberOfEvents=10 * np.ones(25),
            NumberOfBins="5,5,1",
            Names="Dim1,Dim2,Dim3",
            Units="MomentumTransfer,EnergyTransfer,Angstrom",
            OutputWorkspace="ws_MD_2d",
        )
        cls.ws_MD_1d = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-3,3,-10,10,-1,1",
            SignalInput=range(5),
            ErrorInput=range(5),
            NumberOfEvents=10 * np.ones(5),
            NumberOfBins="1,5,1",
            Names="Dim1,Dim2,Dim3",
            Units="MomentumTransfer,EnergyTransfer,Angstrom",
            OutputWorkspace="ws_MD_1d",
        )
        cls.ws2d_point_uneven = CreateWorkspace(DataX=[10, 20, 30], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="ws2d_point_uneven")
        wp = CreateWorkspace(DataX=[15, 25, 35, 45], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(cls.ws2d_point_uneven, wp, CheckOverlapping=False, CheckMatchingBins=False)
        cls.ws2d_point_uneven = mantid.mtd["ws2d_point_uneven"]
        cls.ws2d_histo_uneven = CreateWorkspace(DataX=[10, 20, 30, 40], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="ws2d_histo_uneven")

    @classmethod
    def tearDownClass(cls):
        config["graph1d.autodistribution"] = cls.g1da
        DeleteWorkspace("ws2d_histo")
        DeleteWorkspace("ws_MD_2d")
        DeleteWorkspace("ws_MD_1d")
        DeleteWorkspace("ws2d_point_uneven")

    def test_3d_plot(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        funcs.plot(ax, self.ws2d_histo, "rs", specNum=1)
        funcs.plot(ax, self.ws2d_histo, specNum=2)
        funcs.plot(ax, self.ws_MD_1d)

    def test_3d_scatter(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        funcs.scatter(ax, self.ws2d_histo)
        funcs.scatter(ax, self.ws_MD_2d)

    def test_3d_wireframe(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        funcs.plot_wireframe(ax, self.ws2d_histo)
        funcs.plot_wireframe(ax, self.ws_MD_2d)

    def test_3d_surface(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        funcs.plot_surface(ax, self.ws2d_histo_rag)
        funcs.plot_surface(ax, self.ws2d_histo, vmin=0)
        funcs.plot_surface(ax, self.ws_MD_2d)

    def test_3d_contour(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        funcs.contour(ax, self.ws2d_histo_rag)
        funcs.contourf(ax, self.ws2d_histo, vmin=0)
        self.assertRaises(ValueError, funcs.contour, ax, self.ws2d_point_uneven)


if __name__ == "__main__":
    unittest.main()
