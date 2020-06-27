# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from contextlib import contextmanager
import unittest
from unittest.mock import MagicMock, patch
import sys

from mantid.api import (MatrixWorkspace, IMDEventWorkspace, IMDHistoWorkspace,
                        SpecialCoordinateSystem)
from mantid.geometry import IMDDimension
from numpy.testing import assert_equal
import numpy as np

# Mock out simpleapi to import expensive import of something we patch out anyway
sys.modules['mantid.simpleapi'] = MagicMock()
from mantidqt.widgets.sliceviewer.model import SliceViewerModel, WS_TYPE  # noqa: E402


# Mock helpers
def _create_mock_histoworkspace(ndims: int, coords: SpecialCoordinateSystem, extents: tuple,
                                signal: np.array, error: np.array, nbins: tuple, names: tuple,
                                units: tuple, isq: tuple):
    """
    :param ndims: The number of dimensions
    :param coords: MD coordinate system
    :param extents: Extents of each dimension
    :param signal: Array to be returned as signal
    :param error: Array to be returned as errors
    :param nbins: Number of bins in each dimension
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param isq: Boolean for each dimension defining if Q or not
    """
    ws = _create_mock_workspace(IMDHistoWorkspace, coords, has_oriented_lattice=False, ndims=ndims)
    ws.getSignalArray.return_value = signal
    ws.getErrorSquaredArray.return_value = error
    ws.hasOriginalWorkspace.return_value = False
    return _add_dimensions(ws, names, isq, extents, nbins, units)


@contextmanager
def _attach_as_original(histo_ws, mde_ws):
    """
    Temporarily attach an MDEventWorkspace to a MDHistoWorkspace
    as an original workspace
    :param histo_ws: A mock MDHistoWorkspace
    :param mde_ws: A mock MDEventWorkspace
    """
    histo_ws.hasOriginalWorkspace.return_value = True
    histo_ws.getOriginalWorkspace.return_value = mde_ws
    yield
    histo_ws.hasOriginalWorkspace.return_value = False
    histo_ws.getOriginalWorkspace.return_value = MagicMock()


def _create_mock_mdeventworkspace(ndims: int, coords: SpecialCoordinateSystem, extents: tuple,
                                  names: tuple, units: tuple, isq: tuple):
    """
    :param ndims: The number of dimensions
    :param coords: MD coordinate system
    :param extents: Extents of each dimension
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param isq: Boolean for each dimension defining if Q or not
    """
    ws = _create_mock_workspace(IMDEventWorkspace, coords, has_oriented_lattice=False, ndims=ndims)
    return _add_dimensions(ws, names, isq, extents, nbins=(1, ) * ndims, units=units)


def _create_mock_matrixworkspace(x_axis: tuple,
                                 y_axis: tuple,
                                 distribution: bool,
                                 names: tuple,
                                 units: tuple = None):
    """
    :param x_axis: X axis values
    :param y_axis: Y axis values
    :param distribution: Value of distribution flag
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    """
    ws = MagicMock(MatrixWorkspace)
    ws.getNumDims.return_value = 2
    ws.getNumberHistograms.return_value = len(y_axis)
    ws.isDistribution.return_value = distribution
    extents = (x_axis[0], x_axis[-1], y_axis[0], y_axis[-1])
    nbins = (len(x_axis) - 1, len(y_axis) - 1)
    return _add_dimensions(ws, names, (False, False), extents, nbins, units)


def _create_mock_workspace(ws_type,
                           coords: SpecialCoordinateSystem = None,
                           has_oriented_lattice: bool = None,
                           ndims: int = 2):
    """
    :param ws_type: Used this as spec for Mock
    :param coords: MD coordinate system for MD workspaces
    :param has_oriented_lattice: If the mock should claim to have an attached a lattice
    :param ndims: The number of dimensions
    """
    ws = MagicMock(spec=ws_type)
    if hasattr(ws, 'getExperimentInfo'):
        ws.getNumDims.return_value = ndims
        if ws_type == IMDHistoWorkspace:
            ws.isMDHistoWorkspace.return_value = True
            ws.getNonIntegratedDimensions.return_value = [MagicMock(), MagicMock()]
            ws.hasOriginalWorkspace.return_value = False
        else:
            ws.isMDHistoWorkspace.return_value = False

        ws.getSpecialCoordinateSystem.return_value = coords
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


def _add_dimensions(mock_ws,
                    names,
                    isq,
                    extents: tuple = None,
                    nbins: tuple = None,
                    units: tuple = None):
    """
    :param mock_ws: An existing mock workspace object
    :param names: The name of each dimension
    :param isq: Boolean for each dimension defining if Q or not
    :param extents: Extents of each dimension
    :param nbins: Number of bins in each dimension
    :param units: Unit labels for each dimension
    """
    def create_dimension(index):
        dimension = MagicMock(spec=IMDDimension)
        dimension.name = names[index]
        mdframe = MagicMock()
        mdframe.isQ.return_value = isq[index]
        dimension.getMDFrame.return_value = mdframe
        if units is not None:
            dimension.getUnits.return_value = units[index]
        if extents is not None:
            dim_min, dim_max = extents[2 * index], extents[2 * index + 1]
            dimension.getMinimum.return_value = dim_min
            dimension.getMaximum.return_value = dim_max
            if nbins is not None:
                bin_width = (dim_max - dim_min) / nbins[index]
                dimension.getBinWidth.return_value = bin_width
                dimension.getX.side_effect = lambda i: dim_min + bin_width * i
                dimension.getNBins.return_value = nbins[index]
        return dimension

    dimensions = [create_dimension(index) for index in range(len(names))]
    mock_ws.getDimension.side_effect = lambda index: dimensions[index]
    return mock_ws


class ArraysEqual:
    """Compare arrays for equality in mock.assert_called_with calls.
    """
    def __init__(self, expected):
        self._expected = expected

    def __eq__(self, other):
        return np.all(self._expected == other)

    def __repr__(self):
        """Return a string when the test comparison fails"""
        return f"{self._expected}"


def create_mock_sliceinfo(indices: tuple):
    """
    Create mock sliceinfo
    :param indices: 3D indices defining permuation order of dimensions
    """
    slice_info = MagicMock()
    slice_info.transform.side_effect = lambda x: [x[indices[0]], x[indices[1]], x[indices[2]]]
    return slice_info


class SliceViewerModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.ws_MD_3D = _create_mock_histoworkspace(ndims=3,
                                                    coords=SpecialCoordinateSystem.NONE,
                                                    extents=(-3, 3, -10, 10, -1, 1),
                                                    signal=np.arange(100.).reshape(5, 5, 4),
                                                    error=np.arange(100.).reshape(5, 5, 4),
                                                    nbins=(5, 5, 4),
                                                    names=('Dim1', 'Dim2', 'Dim3'),
                                                    units=('MomentumTransfer', 'EnergyTransfer',
                                                           'Angstrom'),
                                                    isq=(False, False, False))
        self.ws_MD_3D.name.return_value = 'ws_MD_3D'
        self.ws_MDE_3D = _create_mock_mdeventworkspace(ndims=3,
                                                       coords=SpecialCoordinateSystem.NONE,
                                                       extents=(-3, 3, -4, 4, -5, 5),
                                                       names=('h', 'k', 'l'),
                                                       units=('rlu', 'rlu', 'rlu'),
                                                       isq=(False, False, False))
        self.ws_MDE_3D.name.return_value = 'ws_MDE_3D'

        self.ws2d_histo = _create_mock_matrixworkspace(x_axis=(10, 20, 30),
                                                       y_axis=(4, 6, 8),
                                                       distribution=True,
                                                       names=('Wavelength', 'Energy transfer'),
                                                       units=('Angstrom', 'meV'))
        self.ws2d_histo.name.return_value = 'ws2d_histo'

    def test_model_MDH(self):
        model = SliceViewerModel(self.ws_MD_3D)

        self.assertEqual(model.get_ws(), self.ws_MD_3D)
        self.assertEqual(model.get_ws_type(), WS_TYPE.MDH)

        signal = np.arange(100).reshape(5, 5, 4)
        assert_equal(model.get_data((None, 2, 2)), signal[(slice(None), 3, 3)])
        assert_equal(model.get_data((1, 2, None)), range(72, 76))
        assert_equal(model.get_data((None, None, 0)), signal[(slice(None), slice(None), 2)])

        dim_info = model.get_dim_info(0)
        self.assertEqual(dim_info['minimum'], -3)
        self.assertEqual(dim_info['maximum'], 3)
        self.assertEqual(dim_info['number_of_bins'], 5)
        self.assertAlmostEqual(dim_info['width'], 1.2)
        self.assertEqual(dim_info['name'], 'Dim1')
        self.assertEqual(dim_info['units'], 'MomentumTransfer')
        self.assertEqual(dim_info['type'], 'MDH')
        self.assertEqual(dim_info['qdim'], False)

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
        self.assertEqual(dim_info['qdim'], False)

    @patch('mantidqt.widgets.sliceviewer.model.BinMD')
    def test_model_MDE(self, mock_binmd):
        model = SliceViewerModel(self.ws_MDE_3D)
        mock_binmd.return_value = self.ws_MD_3D

        self.assertNotEqual(model.get_ws((None, None, 0), (1, 2, 4)), self.ws_MDE_3D)
        mock_binmd.assert_called_once_with(InputWorkspace=self.ws_MDE_3D,
                                           OutputWorkspace='ws_MDE_3D_svrebinned',
                                           AlignedDim0='h,-3,3,1',
                                           AlignedDim1='k,-4,4,2',
                                           AlignedDim2='l,-2.0,2.0,1',
                                           EnableLogging=False)
        mock_binmd.reset_mock()
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
        self.assertEqual(dim_info['qdim'], False)

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
        self.assertEqual(dim_info['qdim'], False)

    def test_model_given_MDH_with_original_uses_original_as_data_ws(self):
        with _attach_as_original(self.ws_MD_3D, self.ws_MDE_3D):
            model = SliceViewerModel(self.ws_MD_3D)

            self.assertEqual(self.ws_MDE_3D, model._get_ws())

    @patch('mantidqt.widgets.sliceviewer.model.BinMD')
    def test_get_ws_MDE_with_limits_uses_limits_over_dimension_extents(self, mock_binmd):
        model = SliceViewerModel(self.ws_MDE_3D)
        mock_binmd.return_value = self.ws_MD_3D

        self.assertNotEqual(model.get_ws((None, None, 0), (1, 2, 4), ((-2, 2), (-1, 1))),
                            self.ws_MDE_3D)
        call_params = dict(InputWorkspace=self.ws_MDE_3D,
                           OutputWorkspace='ws_MDE_3D_svrebinned',
                           AlignedDim0='h,-2,2,1',
                           AlignedDim1='k,-1,1,2',
                           AlignedDim2='l,-2.0,2.0,1',
                           EnableLogging=False)
        mock_binmd.assert_called_once_with(**call_params)
        mock_binmd.reset_mock()

        model.get_data((None, None, 0), (1, 2, 4), ((-2, 2), (-1, 1)))
        mock_binmd.assert_called_once_with(**call_params)

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
        self.assertEqual(dim_info['qdim'], False)

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
        self.assertEqual(dim_info['qdim'], False)

    def test_qflags_for_qlab_coordinates_detected(self):
        mock_q3d = _create_mock_workspace(IMDEventWorkspace,
                                          coords=SpecialCoordinateSystem.QLab,
                                          has_oriented_lattice=False)
        mock_q3d = _add_dimensions(mock_q3d, ('Q_lab_x', 'Q_lab_y', 'Q_lab_z', 'DeltaE'),
                                   isq=(True, True, True, False))
        model = SliceViewerModel(mock_q3d)

        for i in range(3):
            dim_info = model.get_dim_info(i)
            self.assertTrue(dim_info['qdim'], msg=f'Dimension {i} not spatial as expected')
        self.assertFalse(model.get_dim_info(3)['qdim'])

    def test_qflags_for_qsample_coordinates_detected(self):
        mock_q3d = _create_mock_workspace(IMDEventWorkspace,
                                          coords=SpecialCoordinateSystem.QSample,
                                          has_oriented_lattice=False)
        mock_q3d = _add_dimensions(mock_q3d, ('Q_sample_x', 'Q_sample_y', 'Q_sample_z', 'DeltaE'),
                                   isq=(True, True, True, False))

        model = SliceViewerModel(mock_q3d)

        for i in range(3):
            dim_info = model.get_dim_info(i)
            self.assertTrue(dim_info['qdim'], msg=f'Dimension {i} not spatial as expected')
        self.assertFalse(model.get_dim_info(3)['qdim'])

    def test_qflags_for_hkl_coordinates_detected(self):
        mock_q3d = _create_mock_workspace(IMDEventWorkspace,
                                          coords=SpecialCoordinateSystem.HKL,
                                          has_oriented_lattice=False)
        mock_q3d = _add_dimensions(mock_q3d, ('[H,0,0]', '[0,K,0]', '[0,0,L]', 'DeltaE'),
                                   isq=(True, True, True, False))
        model = SliceViewerModel(mock_q3d)

        for i in range(3):
            dim_info = model.get_dim_info(i)
            self.assertTrue(dim_info['qdim'], msg=f'Dimension {i} not spatial as expected')
        self.assertFalse(model.get_dim_info(3)['qdim'])

    def test_matrix_workspace_can_be_normalized_if_not_a_distribution(self):
        non_distrib_ws2d = _create_mock_matrixworkspace((1, 2, 3), (4, 5, 6),
                                                        distribution=False,
                                                        names=('a', 'b'))
        model = SliceViewerModel(non_distrib_ws2d)
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
                                                  coords=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=True)

    def test_MDE_workspace_in_hkl_supports_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(True,
                                                  ws_type=IMDEventWorkspace,
                                                  coords=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=True)

    def test_matrix_workspace_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=MatrixWorkspace,
                                                  coords=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=None)

    def test_MDH_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  coords=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=False)

    def test_MDE_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  coords=SpecialCoordinateSystem.HKL,
                                                  has_oriented_lattice=False)

    def test_MDH_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  coords=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=False)
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDHistoWorkspace,
                                                  coords=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=True)

    def test_MDE_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  coords=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=False)
        self._assert_supports_non_orthogonal_axes(False,
                                                  ws_type=IMDEventWorkspace,
                                                  coords=SpecialCoordinateSystem.QLab,
                                                  has_oriented_lattice=True)

    def test_matrix_workspace_cannot_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(False, MatrixWorkspace)

    def test_md_workspace_with_fewer_than_three_dimensions_cannot_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(False, IMDEventWorkspace, ndims=2)
        self._assert_supports_peaks_overlay(False, IMDHistoWorkspace, ndims=2)

    def test_md_workspace_with_three_or_more_dimensions_can_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(True, IMDEventWorkspace, ndims=3)
        self._assert_supports_peaks_overlay(True, IMDHistoWorkspace, ndims=3)

    def test_mdeventworkspace_supports_dynamic_rebinning(self):
        self._assert_supports_dynamic_rebinning(True,
                                                IMDEventWorkspace,
                                                has_original_workspace=True)
        self._assert_supports_dynamic_rebinning(True,
                                                IMDEventWorkspace,
                                                has_original_workspace=False)

    def test_mdhistoworkspace_supports_dynamic_rebinning(self):
        self._assert_supports_dynamic_rebinning(True,
                                                IMDHistoWorkspace,
                                                has_original_workspace=True)
        self._assert_supports_dynamic_rebinning(False,
                                                IMDHistoWorkspace,
                                                has_original_workspace=False)

    def test_matrixworkspace_does_not_dynamic_rebinning(self):
        self._assert_supports_dynamic_rebinning(False, MatrixWorkspace)

    def test_title_for_matrixworkspace_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws2d_histo)

        self.assertEqual('Sliceviewer - ws2d_histo', model.get_title())

    def test_title_for_mdeventworkspace_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws_MDE_3D)

        self.assertEqual('Sliceviewer - ws_MDE_3D', model.get_title())

    def test_title_for_mdhistoworkspace_without_original_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws_MD_3D)

        self.assertEqual('Sliceviewer - ws_MD_3D', model.get_title())

    def test_title_for_mdhistoworkspace_with_original_contains_original_name(self):
        with _attach_as_original(self.ws_MD_3D, self.ws_MDE_3D):
            model = SliceViewerModel(self.ws_MD_3D)

            self.assertEqual('Sliceviewer - ws_MDE_3D (original of ws_MD_3D)', model.get_title())

    def test_create_non_orthogonal_transform_raises_error_if_not_supported(self):
        model = SliceViewerModel(
            _create_mock_workspace(MatrixWorkspace,
                                   SpecialCoordinateSystem.QLab,
                                   has_oriented_lattice=False))

        self.assertRaises(RuntimeError, model.create_nonorthogonal_transform, (0, 1, 2))

    @patch("mantidqt.widgets.sliceviewer.model.NonOrthogonalTransform")
    def test_create_non_orthogonal_transform_uses_W_if_avilable(self, mock_nonortho_trans):
        ws = _create_mock_workspace(IMDEventWorkspace,
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
        ws = _create_mock_workspace(IMDEventWorkspace,
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

    def test_get_dim_limits_returns_limits_for_display_dimensions_for_matrix(self):
        model = SliceViewerModel(self.ws2d_histo)
        data_limits = ((10, 30), (4, 8))

        limits = model.get_dim_limits(slicepoint=(None, None), transpose=False)
        self.assertEqual(data_limits, limits)
        limits = model.get_dim_limits(slicepoint=(None, None), transpose=True)
        self.assertEqual((data_limits[1], data_limits[0]), limits)

    def test_get_dim_limits_returns_limits_for_display_dimensions_for_md(self):
        model = SliceViewerModel(self.ws_MDE_3D)
        data_limits = ((-3, 3), (-4, 4), (-5, 5))

        limits = model.get_dim_limits(slicepoint=(None, None, 0), transpose=False)
        self.assertEqual(data_limits[:2], limits)
        limits = model.get_dim_limits(slicepoint=(None, None, 0), transpose=True)
        self.assertEqual((data_limits[1], data_limits[0]), limits)
        limits = model.get_dim_limits(slicepoint=(None, 0, None), transpose=False)
        self.assertEqual((data_limits[0], data_limits[2]), limits)
        limits = model.get_dim_limits(slicepoint=(None, 0, None), transpose=True)
        self.assertEqual((data_limits[2], data_limits[0]), limits)

    def test_get_dim_limits_raises_error_num_display_dims_ne_2(self):
        model = SliceViewerModel(self.ws_MDE_3D)

        self.assertRaises(AssertionError,
                          model.get_dim_limits,
                          slicepoint=(0, 0, 0),
                          transpose=False)
        self.assertRaises(AssertionError,
                          model.get_dim_limits,
                          slicepoint=(None, 0, 0),
                          transpose=False)
        self.assertRaises(AssertionError,
                          model.get_dim_limits,
                          slicepoint=(None, None, None),
                          transpose=False)

    def test_get_dim_limits_raises_if_slicepoint_length_ne_ndims(self):
        model = SliceViewerModel(self.ws_MDE_3D)

        self.assertRaises(AssertionError,
                          model.get_dim_limits,
                          slicepoint=(None, None),
                          transpose=False)
        self.assertRaises(AssertionError,
                          model.get_dim_limits,
                          slicepoint=(None, None, 0, 0),
                          transpose=False)

    # private
    def _assert_supports_non_orthogonal_axes(self, expectation, ws_type, coords,
                                             has_oriented_lattice):
        model = SliceViewerModel(_create_mock_workspace(ws_type, coords, has_oriented_lattice))
        self.assertEqual(expectation, model.can_support_nonorthogonal_axes())

    def _assert_supports_peaks_overlay(self, expectation, ws_type, ndims=2):
        ws = _create_mock_workspace(ws_type,
                                    coords=SpecialCoordinateSystem.QLab,
                                    has_oriented_lattice=False,
                                    ndims=ndims)
        model = SliceViewerModel(ws)
        self.assertEqual(expectation, model.can_support_peaks_overlays())

    def _assert_supports_dynamic_rebinning(self, expectation, ws_type, has_original_workspace=None):
        ws = _create_mock_workspace(ws_type,
                                    coords=SpecialCoordinateSystem.QLab,
                                    has_oriented_lattice=False,
                                    ndims=3)
        if ws_type == MatrixWorkspace:
            ws.hasOriginalWorkspace.return_value = False
        elif has_original_workspace is not None:
            ws.hasOriginalWorkspace.return_value = has_original_workspace
            if has_original_workspace:
                ws.getOriginalWorkspace.return_value = self.ws_MDE_3D
        model = SliceViewerModel(ws)
        self.assertEqual(expectation, model.can_support_dynamic_rebinning())


if __name__ == '__main__':
    unittest.main()
