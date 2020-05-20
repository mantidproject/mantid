# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import unittest
from unittest.mock import MagicMock, patch

from mantid.api import (MatrixWorkspace, IMDEventWorkspace, IMDHistoWorkspace,
                        SpecialCoordinateSystem)
from mantid.simpleapi import (CreateMDHistoWorkspace, CreateWorkspace, CreateMDWorkspace,
                              FakeMDEventData)
from mantidqt.widgets.sliceviewer.model import SliceViewerModel, WS_TYPE
from numpy.testing import assert_equal, assert_allclose
import numpy as np


class ArraysEqual:
    def __init__(self, expected):
        self._expected = expected

    def __eq__(self, other):
        return np.all(self._expected == other)

    def __repr__(self):
        return f"{self._expected}"


def create_mock_sliceinfo(indices):
    slice_info = MagicMock()
    slice_info.transform.side_effect = lambda x: [x[indices[0]], x[indices[1]], x[indices[2]]]
    return slice_info


class SliceViewerModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.ws_MD_3D = CreateMDHistoWorkspace(Dimensionality=3,
                                               Extents='-3,3,-10,10,-1,1',
                                               SignalInput=range(100),
                                               ErrorInput=range(100),
                                               NumberOfBins='5,5,4',
                                               Names='Dim1,Dim2,Dim3',
                                               Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                               OutputWorkspace='ws_MD_3D')
        self.ws_MDE_3D = CreateMDWorkspace(Dimensions='3',
                                           Extents='-3,3,-4,4,-5,5',
                                           Names='h,k,l',
                                           Units='rlu,rlu,rlu',
                                           SplitInto='4',
                                           OutputWorkspace='ws_MDE_3D')
        FakeMDEventData('ws_MDE_3D',
                        PeakParams='100000,0,0,0,0.1',
                        RandomSeed='63759',
                        RandomizeSignal='1')
        FakeMDEventData('ws_MDE_3D',
                        PeakParams='40000,1,0,0,0.1',
                        RandomSeed='63759',
                        RandomizeSignal='1')
        self.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                          DataY=[2, 3, 4, 5],
                                          DataE=[1, 2, 3, 4],
                                          NSpec=2,
                                          Distribution=True,
                                          UnitX='Wavelength',
                                          VerticalAxisUnit='DeltaE',
                                          VerticalAxisValues=[4, 6, 8],
                                          OutputWorkspace='ws2d_histo')

    def test_model_MDH(self):
        model = SliceViewerModel(self.ws_MD_3D)

        self.assertEqual(model.get_ws(), self.ws_MD_3D)
        self.assertEqual(model.get_ws_type(), WS_TYPE.MDH)

        assert_equal(model.get_data((None, 2, 2)), range(90, 95))
        assert_equal(model.get_data((1, 2, None)), range(18, 118, 25))
        assert_equal(model.get_data((None, None, 0)), np.reshape(range(50, 75), (5, 5)).T)

        dim_info = model.get_dim_info(0)
        self.assertEqual(dim_info['minimum'], -3)
        self.assertEqual(dim_info['maximum'], 3)
        self.assertEqual(dim_info['number_of_bins'], 5)
        self.assertAlmostEqual(dim_info['width'], 1.2)
        self.assertEqual(dim_info['name'], 'Dim1')
        self.assertEqual(dim_info['units'], 'MomentumTransfer')
        self.assertEqual(dim_info['type'], 'MDH')

        dim_infos = model.get_dimensions_info()
        self.assertEqual(len(dim_infos), 3)

        dim_info = dim_infos[2]
        self.assertEqual(dim_info['minimum'], -1)
        self.assertEqual(dim_info['maximum'], 1)
        self.assertEqual(dim_info['number_of_bins'], 4)
        self.assertAlmostEqual(dim_info['width'], 0.5)
        self.assertEqual(dim_info['name'], 'Dim3')
        self.assertEqual(dim_info['units'], 'Angstrom')
        self.assertEqual(dim_info['type'], 'MDH')

    def test_model_MDE(self):
        model = SliceViewerModel(self.ws_MDE_3D)

        self.assertNotEqual(model.get_ws((0, 0, 0), (1, 1, 1)), self.ws_MDE_3D)
        self.assertEqual(model._get_ws(), self.ws_MDE_3D)
        self.assertEqual(model.get_ws_type(), WS_TYPE.MDE)

        dim_info = model.get_dim_info(0)
        self.assertEqual(dim_info['minimum'], -3)
        self.assertEqual(dim_info['maximum'], 3)
        self.assertEqual(dim_info['number_of_bins'], 1)
        self.assertAlmostEqual(dim_info['width'], 6)
        self.assertEqual(dim_info['name'], 'h')
        self.assertEqual(dim_info['units'], 'rlu')
        self.assertEqual(dim_info['type'], 'MDE')

        dim_infos = model.get_dimensions_info()
        self.assertEqual(len(dim_infos), 3)

        dim_info = dim_infos[2]
        self.assertEqual(dim_info['minimum'], -5)
        self.assertEqual(dim_info['maximum'], 5)
        self.assertEqual(dim_info['number_of_bins'], 1)
        self.assertAlmostEqual(dim_info['width'], 10)
        self.assertEqual(dim_info['name'], 'l')
        self.assertEqual(dim_info['units'], 'rlu')
        self.assertEqual(dim_info['type'], 'MDE')

        mdh = model.get_ws((None, 0, None), (3, 0.001, 3))
        assert_allclose(mdh.getSignalArray().squeeze(),
                        [[0, 0, 0], [0, 692.237618, 0], [0, 118.362777, 0]])

        d0 = mdh.getDimension(0)
        d1 = mdh.getDimension(1)
        d2 = mdh.getDimension(2)
        self.assertEqual(d0.name, 'h')
        self.assertEqual(d0.getNBins(), 3)
        self.assertEqual(d0.getMinimum(), -3)
        self.assertEqual(d0.getMaximum(), 3)
        self.assertEqual(d1.name, 'k')
        self.assertEqual(d1.getNBins(), 1)
        self.assertAlmostEqual(d1.getMinimum(), -0.0005)
        self.assertAlmostEqual(d1.getMaximum(), 0.0005)
        self.assertEqual(d2.name, 'l')
        self.assertEqual(d2.getNBins(), 3)
        self.assertEqual(d2.getMinimum(), -5)
        self.assertEqual(d2.getMaximum(), 5)

        assert_allclose(model.get_data((None, 0, None), (3, 0.001, 3)),
                        [[0, 0, 0], [0, 692.237618, 0], [0, 118.362777, 0]])

        assert_allclose(model.get_data((None, 0, None), (3, 0.001, 3), transpose=True),
                        [[0, 0, 0], [0, 692.237618, 118.362777], [0, 0, 0]])

    def test_model_matrix(self):
        model = SliceViewerModel(self.ws2d_histo)

        self.assertEqual(model.get_ws(), self.ws2d_histo)
        self.assertEqual(model.get_ws_type(), WS_TYPE.MATRIX)

        dim_info = model.get_dim_info(0)
        self.assertEqual(dim_info['minimum'], 10)
        self.assertEqual(dim_info['maximum'], 30)
        self.assertEqual(dim_info['number_of_bins'], 2)
        self.assertAlmostEqual(dim_info['width'], 10)
        self.assertEqual(dim_info['name'], 'Wavelength')
        self.assertEqual(dim_info['units'], 'Angstrom')
        self.assertEqual(dim_info['type'], 'MATRIX')

        dim_infos = model.get_dimensions_info()
        self.assertEqual(len(dim_infos), 2)

        dim_info = dim_infos[1]
        self.assertEqual(dim_info['minimum'], 4)
        self.assertEqual(dim_info['maximum'], 8)
        self.assertEqual(dim_info['number_of_bins'], 2)
        self.assertAlmostEqual(dim_info['width'], 2)
        self.assertEqual(dim_info['name'], 'Energy transfer')
        self.assertEqual(dim_info['units'], 'meV')
        self.assertEqual(dim_info['type'], 'MATRIX')

    def test_matrix_workspace_can_be_normalized_if_not_a_distribution(self):
        ws2d = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                               DataY=[2, 3, 4, 5],
                               DataE=[1, 2, 3, 4],
                               NSpec=2,
                               Distribution=False,
                               OutputWorkspace='ws2d')
        model = SliceViewerModel(ws2d)
        self.assertTrue(model.can_normalize_workspace())

    def test_matrix_workspace_cannot_be_normalized_if_a_distribution(self):
        model = SliceViewerModel(self.ws2d_histo)
        self.assertFalse(model.can_normalize_workspace())

    def test_MD_workspaces_cannot_be_normalized(self):
        model = SliceViewerModel(self.ws_MD_3D)
        self.assertFalse(model.can_normalize_workspace())

    def test_MDE_workspaces_cannot_be_normalized(self):
        model = SliceViewerModel(self.ws_MDE_3D)
        self.assertFalse(model.can_normalize_workspace())

    def test_MDH_workspace_in_hkl_supports_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(True,
                                                  ws_type=IMDHistoWorkspace,
                                                  units=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=True)

    def test_MDE_workspace_in_hkl_supports_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(True,
                                                  ws_type=IMDEventWorkspace,
                                                  units=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=True)

    def test_matrix_workspace_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=MatrixWorkspace,
                                                  units=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=None)

    def test_MDH_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  units=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=False)

    def test_MDE_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  units=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=False)

    def test_MDH_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  units=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=False)
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  units=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=True)

    def test_MDE_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  units=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=False)
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  units=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=True)

    def test_matrix_workspace_cannot_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(False, MatrixWorkspace)

    def test_md_workspace_with_fewer_than_three_dimensions_cannot_support_peaks_overlay(self):
        for ndims in range(3):
            self._assert_supports_peaks_overlay(False, IMDEventWorkspace, ndims=ndims)
            self._assert_supports_peaks_overlay(False, IMDHistoWorkspace, ndims=ndims)

    def test_md_workspace_with_three_or_more_dimensions_can_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(True, IMDEventWorkspace, ndims=3)
        self._assert_supports_peaks_overlay(True, IMDHistoWorkspace, ndims=3)

    def test_create_non_orthogonal_transform_raises_error_if_not_supported(self):
        model = SliceViewerModel(
            self._create_mock_workspace(MatrixWorkspace,
                                        SpecialCoordinateSystem.QLab,
                                        has_oriented_lattice=False))

        self.assertRaises(RuntimeError, model.create_nonorthogonal_transform, (0, 1, 2))

    @patch("mantidqt.widgets.sliceviewer.model.NonOrthogonalTransform")
    def test_create_non_orthogonal_transform_uses_W_if_avilable(self, mock_nonortho_trans):
        ws = self._create_mock_workspace(IMDEventWorkspace,
                                         SpecialCoordinateSystem.HKL,
                                         has_oriented_lattice=True)
        w_prop = MagicMock()
        w_prop.value = [0, 1, 1, 0, 0, 1, 1, 0, 0]
        run = MagicMock()
        run.get.return_value = w_prop
        ws.getExperimentInfo().run.return_value = run
        lattice = MagicMock()
        ws.getExperimentInfo().sample().getOrientedLattice.return_value = lattice
        model = SliceViewerModel(ws)

        model.create_nonorthogonal_transform(create_mock_sliceinfo([1, 2, 0]))

        mock_nonortho_trans.from_lattice.assert_called_once_with(
            lattice,
            x_proj=ArraysEqual(np.array([1, 0, 0])),
            y_proj=ArraysEqual(np.array([1, 1, 0])))

    @patch("mantidqt.widgets.sliceviewer.model.NonOrthogonalTransform")
    def test_create_non_orthogonal_transform_uses_identity_if_W_unavilable(
            self, mock_nonortho_trans):
        ws = self._create_mock_workspace(IMDEventWorkspace,
                                         SpecialCoordinateSystem.HKL,
                                         has_oriented_lattice=True)
        lattice = MagicMock()
        ws.getExperimentInfo().sample().getOrientedLattice.return_value = lattice
        run = MagicMock()
        run.get.side_effect = KeyError
        ws.getExperimentInfo().run.return_value = run
        model = SliceViewerModel(ws)

        model.create_nonorthogonal_transform(create_mock_sliceinfo([1, 2, 0]))

        mock_nonortho_trans.from_lattice.assert_called_once_with(
            lattice,
            x_proj=ArraysEqual(np.array([0, 1, 0])),
            y_proj=ArraysEqual(np.array([0, 0, 1])))

    # private
    def _assert_supports_non_orthogonal_axes(self, expectation, ws_type, units,
                                             has_oriented_lattice):
        model = SliceViewerModel(self._create_mock_workspace(ws_type, units, has_oriented_lattice))
        self.assertEqual(expectation, model.can_support_nonorthogonal_axes())

    def _assert_supports_peaks_overlay(self, expectation, ws_type, ndims=2):
        ws = self._create_mock_workspace(ws_type,
                                         units=SpecialCoordinateSystem.QLab,
                                         has_oriented_lattice=False,
                                         ndims=ndims)
        model = SliceViewerModel(ws)
        self.assertEqual(expectation, model.can_support_peaks_overlays())

    def _create_mock_workspace(self, ws_type, units, has_oriented_lattice, ndims=2):
        ws = MagicMock(spec=ws_type)
        if hasattr(ws, 'getExperimentInfo'):
            ws.getNumDims.return_value = ndims
            ws.getSpecialCoordinateSystem.return_value = units
            ws.getNonIntegratedDimensions.return_value = [MagicMock(), MagicMock()]
            expt_info = MagicMock()
            sample = MagicMock()
            sample.hasOrientedLattice.return_value = has_oriented_lattice
            expt_info.sample.return_value = sample
            ws.getExperimentInfo.return_value = expt_info
        elif hasattr(ws, 'getNumberHistograms'):
            ws.getNumDims.return_value = 2
            ws.getNumberHistograms.return_value = 3
            mock_dimension = MagicMock()
            mock_dimension.getNBins.return_value = 3
            ws.getDimension.return_value = mock_dimension

        return ws


if __name__ == '__main__':
    unittest.main()
