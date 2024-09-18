# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Author: Alex Phimister 08/2016
# pylint disable: invalid-name
import unittest

from sys import version_info

from mantid.simpleapi import *
from mantid.api import *

import numpy as np


class NMoldyn4InterpolationTest(unittest.TestCase):
    def setUp(self):
        idf_dir = config["instrumentDefinition.directory"]
        osiris = LoadEmptyInstrument(idf_dir + "OSIRIS_Definition.xml")
        osiris = CropWorkspace(osiris, StartWorkspaceIndex=970, EndWorkspaceIndex=980)
        osiris = Rebin(osiris, [-0.6, 0.02, 0.6])
        self.osiris = osiris

    def test_interpolation_change(self):
        x_data = np.arange(-2.0, 2.0, 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data), VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=q_data)

        osiris_X = self.osiris.readX(0)
        osiris_Q_length = self.osiris.getAxis(1).length()
        inter = NMoldyn4Interpolation(sim, self.osiris)
        inter_X = inter.readX(0)
        inter_Q_length = inter.getAxis(1).length()
        sample_Y_data = inter.readY(5)

        self.assertTrue(np.all(inter_X == osiris_X))
        self.assertNotEqual(len(inter_X), sim.getAxis(0).length())
        self.assertEqual(inter_Q_length, osiris_Q_length)
        self.assertNotEqual(inter_Q_length, sim.getNumberHistograms())
        self.assertEqual(len(sample_Y_data), len(self.TEST_DATA))
        for i in range(len(sample_Y_data)):
            self.assertAlmostEqual(sample_Y_data[i], self.TEST_DATA[i], 7)

    def test_X_min_too_big(self):
        x_data = np.arange(-0.5, 2.0, 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data), VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=q_data)
        self.assertRaisesRegex(
            RuntimeError,
            "Minimum simulated X value is higher than minimum reference X value",
            NMoldyn4Interpolation,
            InputWorkspace=sim,
            ReferenceWorkspace=self.osiris,
            OutputWorkspace="__NMoldyn4Interpolation_test",
        )

    def test_X_max_too_small(self):
        x_data = np.arange(-2, 0.5, 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data), VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=q_data)
        self.assertRaisesRegex(
            RuntimeError,
            "Maximum simulated X value is lower than maximum reference X value",
            NMoldyn4Interpolation,
            InputWorkspace=sim,
            ReferenceWorkspace=self.osiris,
            OutputWorkspace="__NMoldyn4Interpolation_test",
        )

    def test_Q_min_too_big(self):
        x_data = np.arange(-2, 2, 0.05)
        q_data = np.arange(0.8, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data), VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=q_data)
        self.assertRaisesRegex(
            RuntimeError,
            "Minimum simulated Q value is higher than minimum reference Q value",
            NMoldyn4Interpolation,
            InputWorkspace=sim,
            ReferenceWorkspace=self.osiris,
            OutputWorkspace="__NMoldyn4Interpolation_test",
        )

    def test_Q_max_too_small(self):
        x_data = np.arange(-2, 2, 0.05)
        q_data = np.arange(0.5, 1.0, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data), VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=q_data)
        self.assertRaisesRegex(
            RuntimeError,
            "Maximum simulated Q value is lower than maximum reference Q value",
            NMoldyn4Interpolation,
            InputWorkspace=sim,
            ReferenceWorkspace=self.osiris,
            OutputWorkspace="__NMoldyn4Interpolation_test",
        )

    TEST_DATA = [
        0.01588991,
        0.03665782,
        0.06572187,
        0.10281931,
        0.14755363,
        0.19949125,
        0.25811529,
        0.32282554,
        0.39299542,
        0.46790749,
        0.54681968,
        0.62894665,
        0.71345976,
        0.79952206,
        0.8862708,
        0.9728371,
        1.05835944,
        1.14198366,
        1.22286732,
        1.30021361,
        1.37323957,
        1.44121843,
        1.50347964,
        1.55938151,
        1.60838829,
        1.64999461,
        1.6837864,
        1.70944087,
        1.72667424,
        1.73534293,
        1.73534293,
        1.72667424,
        1.70944087,
        1.6837864,
        1.64999461,
        1.60838829,
        1.55938151,
        1.50347964,
        1.44121843,
        1.37323957,
        1.30021361,
        1.22286732,
        1.14198366,
        1.05835944,
        0.9728371,
        0.8862708,
        0.79952206,
        0.71345976,
        0.62894665,
        0.54681968,
        0.46790749,
        0.39299542,
        0.32282554,
        0.25811529,
        0.19949125,
        0.14755363,
        0.10281931,
        0.06572187,
        0.03665782,
        0.01588991,
    ]


if __name__ == "__main__":
    unittest.main()
