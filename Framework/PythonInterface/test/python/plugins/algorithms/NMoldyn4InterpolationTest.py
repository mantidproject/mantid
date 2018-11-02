# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
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
        idf_dir = config['instrumentDefinition.directory']
        osiris = LoadEmptyInstrument(idf_dir + 'OSIRIS_Definition.xml')
        osiris = CropWorkspace(osiris, StartWorkspaceIndex=970, EndWorkspaceIndex=980)
        osiris = Rebin(osiris, [-0.6, 0.02, 0.6])
        self.osiris = osiris

    def test_interpolation_change(self):
        x_data = np.arange(-2., 2., 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data),
                              VerticalAxisUnit='MomentumTransfer',
                              VerticalAxisValues=q_data)

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
        self.assertAlmostEqual(sample_Y_data[0], 0.02009531, 8)
        self.assertAlmostEqual(sample_Y_data[30], 1.73103349, 7)

    def test_X_min_too_big(self):
        x_data = np.arange(-0.5, 2., 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data),
                              VerticalAxisUnit='MomentumTransfer',
                              VerticalAxisValues=q_data)
        self.assertRaises(RuntimeError,
                          NMoldyn4Interpolation,
                          InputWorkspace=sim,
                          ReferenceWorkspace=self.osiris,
                          OutputWorkspace='__NMoldyn4Interpolation_test')

    def test_X_max_too_small(self):
        x_data = np.arange(-2, 0.5, 0.05)
        q_data = np.arange(0.5, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data),
                              VerticalAxisUnit='MomentumTransfer',
                              VerticalAxisValues=q_data)
        self.assertRaises(RuntimeError,
                          NMoldyn4Interpolation,
                          InputWorkspace=sim,
                          ReferenceWorkspace=self.osiris,
                          OutputWorkspace='__NMoldyn4Interpolation_test')

    def test_Q_max_too_small(self):
        x_data = np.arange(-2, 2, 0.05)
        q_data = np.arange(0.8, 1.3, 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data),
                              VerticalAxisUnit='MomentumTransfer',
                              VerticalAxisValues=q_data)
        self.assertRaises(RuntimeError,
                          NMoldyn4Interpolation,
                          InputWorkspace=sim,
                          ReferenceWorkspace=self.osiris,
                          OutputWorkspace='__NMoldyn4Interpolation_test')

    def test_Q_min_too_large(self):
        x_data = np.arange(-2, 2, 0.05)
        q_data = np.arange(0.5, 1., 0.1)
        y_data = np.asarray([val * (np.cos(5 * x_data) + 1) for val in q_data])
        y_data = y_data.flatten()
        x_data = np.tile(x_data, len(q_data))
        sim = CreateWorkspace(DataX=x_data, DataY=y_data, NSpec=len(q_data),
                              VerticalAxisUnit='MomentumTransfer',
                              VerticalAxisValues=q_data)
        self.assertRaises(RuntimeError,
                          NMoldyn4Interpolation,
                          InputWorkspace=sim,
                          ReferenceWorkspace=self.osiris,
                          OutputWorkspace='__NMoldyn4Interpolation_test')


if __name__ == "__main__":
    python_version = version_info
    if python_version < (2, 7, 0):
        logger.warning("Not running this test as it requires Python >= 2.7. Version found: {0}".
                       format(python_version))
    else:
        unittest.main()
