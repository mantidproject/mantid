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
from unittest.mock import MagicMock, call, patch, DEFAULT, Mock

from mantid.api import MatrixWorkspace, IMDEventWorkspace, IMDHistoWorkspace, MultipleExperimentInfos
from mantid.kernel import SpecialCoordinateSystem
from mantid.geometry import IMDDimension, OrientedLattice
import numpy as np
from mantidqt.widgets.sliceviewer.models.model import SliceViewerModel, MIN_WIDTH


# Mock helpers
def _create_mock_histoworkspace(
    ndims: int,
    coords: SpecialCoordinateSystem,
    extents: tuple,
    signal: np.array,
    error: np.array,
    nbins: tuple,
    names: tuple,
    units: tuple,
    isq: tuple,
):
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
    ws.hasOriginalWorkspace.side_effect = lambda index: False
    return _add_dimensions(ws, names, isq, extents, nbins, units)


@contextmanager
def _attach_as_original(histo_ws, mde_ws):
    """
    Temporarily attach an MDEventWorkspace to a MDHistoWorkspace
    as an original workspace
    :param histo_ws: A mock MDHistoWorkspace
    :param mde_ws: A mock MDEventWorkspace
    """
    histo_ws.hasOriginalWorkspace.side_effect = lambda index: True
    yield
    histo_ws.hasOriginalWorkspace.side_effect = lambda index: False


def _create_mock_mdeventworkspace(ndims: int, coords: SpecialCoordinateSystem, extents: tuple, names: tuple, units: tuple, isq: tuple):
    """
    :param ndims: The number of dimensions
    :param coords: MD coordinate system
    :param extents: Extents of each dimension
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param isq: Boolean for each dimension defining if Q or not
    """
    ws = _create_mock_workspace(IMDEventWorkspace, coords, has_oriented_lattice=False, ndims=ndims)
    return _add_dimensions(ws, names, isq, extents, nbins=(1,) * ndims, units=units)


def _create_mock_matrixworkspace(
    x_axis: tuple, y_axis: tuple, distribution: bool, names: tuple = None, units: tuple = None, y_is_spectra=True
):
    """
    :param x_axis: X axis values
    :param y_axis: Y axis values
    :param distribution: Value of distribution flag
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param y_is_spectra: True if the Y axis is a spectra axis, else it's a Numeric
    """
    ws = MagicMock(MatrixWorkspace)
    ws.getNumDims.return_value = 2
    ws.getNumberHistograms.return_value = len(y_axis)
    ws.isDistribution.return_value = distribution
    ws.extractX.return_value = x_axis
    axes = [MagicMock(), MagicMock(isSpectra=lambda: y_is_spectra, isNumeric=lambda: not y_is_spectra)]
    ws.getAxis.side_effect = lambda i: axes[i]
    axes[1].extractValues.return_value = np.array(y_axis)
    if y_is_spectra:
        axes[1].indexOfValue.side_effect = lambda i: i + 1

    if names is None:
        names = ("X", "Y")

    extents = (x_axis[0], x_axis[-1], y_axis[0], y_axis[-1])
    nbins = (len(x_axis) - 1, len(y_axis) - 1)
    return _add_dimensions(ws, names, (False, False), extents, nbins, units)


def _create_mock_workspace(ws_type, coords: SpecialCoordinateSystem = None, has_oriented_lattice: bool = None, ndims: int = 2):
    """
    :param ws_type: Used this as spec for Mock
    :param coords: MD coordinate system for MD workspaces
    :param has_oriented_lattice: If the mock should claim to have an attached a lattice
    :param ndims: The number of dimensions
    """
    ws = MagicMock(spec=ws_type)
    if hasattr(ws, "getExperimentInfo"):
        ws.getNumDims.return_value = ndims
        ws.getNumNonIntegratedDims.return_value = ndims
        if ws_type == IMDHistoWorkspace:
            ws.isMDHistoWorkspace.return_value = True
            ws.getNonIntegratedDimensions.return_value = [MagicMock(), MagicMock()]
            ws.hasOriginalWorkspace.return_value = False
            basis_mat = np.eye(ndims)
            ws.getBasisVector.side_effect = lambda idim: basis_mat[:, idim]
            ws.getDimension().getMDFrame().isQ.return_value = True
        else:
            ws.isMDHistoWorkspace.return_value = False

        ws.getSpecialCoordinateSystem.return_value = coords
        run = MagicMock()
        run.get.return_value = MagicMock()
        run.get().value = np.eye(3).flatten()  # proj matrix is always 3x3
        expt_info = MagicMock()
        sample = MagicMock()
        sample.hasOrientedLattice.return_value = has_oriented_lattice
        if has_oriented_lattice:
            lattice = OrientedLattice(1, 1, 1, 90, 90, 90)
            sample.getOrientedLattice.return_value = lattice
        expt_info.sample.return_value = sample
        expt_info.run.return_value = run
        ws.getExperimentInfo.return_value = expt_info
        ws.getExperimentInfo().sample()
    elif hasattr(ws, "getNumberHistograms"):
        ws.getNumDims.return_value = 2
        ws.getNumberHistograms.return_value = 3
        mock_dimension = MagicMock()
        mock_dimension.getNBins.return_value = 3
        ws.getDimension.return_value = mock_dimension
    return ws


def _add_dimensions(mock_ws, names, isq, extents: tuple = None, nbins: tuple = None, units: tuple = None):
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
    """Compare arrays for equality in mock.assert_called_with calls."""

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
    :param indices: 3D indices defining permutation order of dimensions
    """
    slice_info = MagicMock()
    slice_info.transform.side_effect = lambda x: np.array([x[indices[0]], x[indices[1]], x[indices[2]]])
    return slice_info


class SliceViewerModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.ws_MD_3D = _create_mock_histoworkspace(
            ndims=3,
            coords=SpecialCoordinateSystem.NONE,
            extents=(-3, 3, -10, 10, -1, 1),
            signal=np.arange(100.0).reshape(5, 5, 4),
            error=np.arange(100.0).reshape(5, 5, 4),
            nbins=(5, 5, 4),
            names=("Dim1", "Dim2", "Dim3"),
            units=("MomentumTransfer", "EnergyTransfer", "Angstrom"),
            isq=(False, False, False),
        )
        self.ws_MD_3D.name.return_value = "ws_MD_3D"
        self.ws_MDE_3D = _create_mock_mdeventworkspace(
            ndims=3,
            coords=SpecialCoordinateSystem.NONE,
            extents=(-3, 3, -4, 4, -5, 5),
            names=("h", "k", "l"),
            units=("rlu", "rlu", "rlu"),
            isq=(True, True, True),
        )
        self.ws_MDE_4D = _create_mock_mdeventworkspace(
            ndims=4,
            coords=SpecialCoordinateSystem.NONE,
            extents=(-2, 2, -3, 3, -4, 4, -5, 5),
            names=("e", "h", "k", "l"),
            units=("meV", "rlu", "rlu", "rlu"),
            isq=(False, True, True, True),
        )
        self.ws_MDE_3D.name.return_value = "ws_MDE_3D"

        self.ws2d_histo = _create_mock_matrixworkspace(
            x_axis=(10, 20, 30), y_axis=(4, 6, 8), distribution=True, names=("Wavelength", "Energy transfer"), units=("Angstrom", "meV")
        )
        self.ws2d_histo.name.return_value = "ws2d_histo"

    def setUp(self):
        self.ws2d_histo.reset_mock()

    def test_init_with_valid_MatrixWorkspace(self):
        mock_ws = MagicMock(spec=MatrixWorkspace)
        mock_ws.getNumberHistograms.return_value = 2
        mock_ws.getDimension.return_value.getNBins.return_value = 2.0

        self.assertIsNotNone(SliceViewerModel(mock_ws))

    def test_init_with_valid_MDHistoWorkspace(self):
        mock_ws = MagicMock(spec=MultipleExperimentInfos)
        mock_ws.name = Mock(return_value="")
        mock_ws.isMDHistoWorkspace = Mock(return_value=True)
        mock_ws.getNumNonIntegratedDims = Mock(return_value=700)
        mock_ws.numOriginalWorkspaces = Mock(return_value=0)

        with patch.object(SliceViewerModel, "_calculate_axes_angles"):
            self.assertIsNotNone(SliceViewerModel(mock_ws))

    def test_init_with_valid_MDEventWorkspace(self):
        mock_ws = MagicMock(spec=MultipleExperimentInfos)
        mock_ws.name = Mock(return_value="")
        mock_ws.isMDHistoWorkspace = Mock(return_value=False)
        mock_ws.getNumDims = Mock(return_value=4)
        mock_ws.numOriginalWorkspaces = Mock(return_value=0)

        with patch.object(SliceViewerModel, "_calculate_axes_angles"):
            self.assertIsNotNone(SliceViewerModel(mock_ws))

    def test_init_raises_for_incorrect_workspace_type(self):
        mock_ws = MagicMock()

        self.assertRaisesRegex(ValueError, "MatrixWorkspace and MDWorkspace", SliceViewerModel, mock_ws)

    def test_init_raises_if_fewer_than_two_histograms(self):
        mock_ws = MagicMock(spec=MatrixWorkspace)
        mock_ws.getNumberHistograms.return_value = 1

        self.assertRaisesRegex(ValueError, "contain at least 2 spectra", SliceViewerModel, mock_ws)

    def test_init_raises_if_fewer_than_two_bins(self):
        mock_ws = MagicMock(spec=MatrixWorkspace)
        mock_ws.getNumberHistograms.return_value = 2
        mock_ws.getDimension.return_value.getNBins.return_value = 1

        self.assertRaisesRegex(ValueError, "contain at least 2 bins", SliceViewerModel, mock_ws)

    def test_init_raises_if_fewer_than_two_integrated_dimensions(self):
        mock_ws = MagicMock(spec=MultipleExperimentInfos)
        mock_ws.isMDHistoWorkspace = Mock(return_value=True)
        mock_ws.getNumNonIntegratedDims = Mock(return_value=1)
        mock_ws.name = Mock(return_value="")

        self.assertRaisesRegex(ValueError, "at least 2 non-integrated dimensions", SliceViewerModel, mock_ws)

    def test_init_raises_if_fewer_than_two_dimensions(self):
        mock_ws = MagicMock(spec=MultipleExperimentInfos)
        mock_ws.isMDHistoWorkspace = Mock(return_value=False)
        mock_ws.getNumDims = Mock(return_value=1)
        mock_ws.name = Mock(return_value="")

        self.assertRaisesRegex(ValueError, "at least 2 dimensions", SliceViewerModel, mock_ws)

    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    def test_model_MDE_basis_vectors_not_normalised_when_HKL(self, mock_binmd):
        ws = _create_mock_mdeventworkspace(
            ndims=3,
            coords=SpecialCoordinateSystem.HKL,
            extents=(-3, 3, -4, 4, -5, 5),
            names=("h", "k", "l"),
            units=("r.l.u.", "r.l.u.", "r.l.u."),
            isq=(True, True, True),
        )
        model = SliceViewerModel(ws)
        mock_binmd.return_value = self.ws_MD_3D  # different workspace

        self.assertNotEqual(model.get_ws((None, None, 0), (1, 2, 4)), ws)

        mock_binmd.assert_called_once_with(
            AxisAligned=False,
            NormalizeBasisVectors=False,
            BasisVector0="h,r.l.u.,1.0,0.0,0.0",
            BasisVector1="k,r.l.u.,0.0,1.0,0.0",
            BasisVector2="l,r.l.u.,0.0,0.0,1.0",
            EnableLogging=False,
            InputWorkspace=ws,
            OutputBins=[1, 2, 1],
            OutputExtents=[-3, 3, -4, 4, -2.0, 2.0],
            OutputWorkspace=ws.name() + "_svrebinned",
        )
        mock_binmd.reset_mock()

    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    def test_get_ws_MDE_with_limits_uses_limits_over_dimension_extents(self, mock_binmd):
        model = SliceViewerModel(self.ws_MDE_3D)
        mock_binmd.return_value = self.ws_MD_3D

        self.assertNotEqual(model.get_ws((None, None, 0), (1, 2, 4), ((-2, 2), (-1, 1)), [0, 1, None]), self.ws_MDE_3D)

        call_params = dict(
            AxisAligned=False,
            BasisVector0="h,rlu,1.0,0.0,0.0",
            BasisVector1="k,rlu,0.0,1.0,0.0",
            BasisVector2="l,rlu,0.0,0.0,1.0",
            EnableLogging=False,
            InputWorkspace=self.ws_MDE_3D,
            OutputBins=[1, 2, 1],
            OutputExtents=[-2, 2, -1, 1, -2.0, 2.0],
            OutputWorkspace="ws_MDE_3D_svrebinned",
        )
        mock_binmd.assert_called_once_with(**call_params)
        mock_binmd.reset_mock()

        model.get_data((None, None, 0), (1, 2, 4), [0, 1, None], ((-2, 2), (-1, 1)))
        mock_binmd.assert_called_once_with(**call_params)

    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    def test_get_ws_mde_sets_minimum_width_on_data_limits(self, mock_binmd):
        model = SliceViewerModel(self.ws_MDE_3D)
        mock_binmd.return_value = self.ws_MD_3D
        xmin = -5e-8
        xmax = 5e-8

        self.assertNotEqual(model.get_ws((None, None, 0), (1, 2, 4), ((xmin, xmax), (-1, 1)), [0, 1, None]), self.ws_MDE_3D)

        call_params = dict(
            AxisAligned=False,
            BasisVector0="h,rlu,1.0,0.0,0.0",
            BasisVector1="k,rlu,0.0,1.0,0.0",
            BasisVector2="l,rlu,0.0,0.0,1.0",
            EnableLogging=False,
            InputWorkspace=self.ws_MDE_3D,
            OutputBins=[1, 2, 1],
            OutputExtents=[xmin, xmin + MIN_WIDTH, -1, 1, -2.0, 2.0],
            OutputWorkspace="ws_MDE_3D_svrebinned",
        )
        mock_binmd.assert_called_once_with(**call_params)
        mock_binmd.reset_mock()

    def test_matrix_workspace_can_be_normalized_if_not_a_distribution(self):
        non_distrib_ws2d = _create_mock_matrixworkspace((1, 2, 3), (4, 5, 6), distribution=False, names=("a", "b"))
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
        self._assert_supports_non_orthogonal_axes(
            True, ws_type=IMDHistoWorkspace, coords=SpecialCoordinateSystem.HKL, has_oriented_lattice=True
        )

    def test_matrix_workspace_cannot_support_non_axis_aligned_cuts(self):
        self._assert_supports_non_axis_aligned_cuts(False, ws_type=MatrixWorkspace)

    def test_MDE_requires_3dims_to_support_non_axis_aligned_cuts(self):
        self._assert_supports_non_axis_aligned_cuts(False, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.HKL, ndims=2)
        self._assert_supports_non_axis_aligned_cuts(True, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.HKL, ndims=3)
        self._assert_supports_non_axis_aligned_cuts(False, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.HKL, ndims=4)

    def test_MDE_workspace_in_hkl_supports_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            True, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.HKL, has_oriented_lattice=True
        )

    def test_matrix_workspace_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=MatrixWorkspace, coords=SpecialCoordinateSystem.HKL, has_oriented_lattice=None
        )

    def test_MDH_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDHistoWorkspace, coords=SpecialCoordinateSystem.HKL, has_oriented_lattice=False
        )

    def test_MDE_workspace_in_hkl_without_lattice_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.HKL, has_oriented_lattice=False
        )

    def test_MDH_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDHistoWorkspace, coords=SpecialCoordinateSystem.QLab, has_oriented_lattice=False
        )
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDHistoWorkspace, coords=SpecialCoordinateSystem.QLab, has_oriented_lattice=True
        )

    def test_MDE_workspace_in_non_hkl_cannot_support_non_orthogonal_axes(self):
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.QLab, has_oriented_lattice=False
        )
        self._assert_supports_non_orthogonal_axes(
            False, ws_type=IMDEventWorkspace, coords=SpecialCoordinateSystem.QLab, has_oriented_lattice=True
        )

    def test_matrix_workspace_cannot_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(False, MatrixWorkspace)

    def test_md_workspace_with_fewer_than_three_dimensions_cannot_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(False, IMDEventWorkspace, ndims=2)
        self._assert_supports_peaks_overlay(False, IMDHistoWorkspace, ndims=2)

    def test_md_workspace_with_three_or_more_dimensions_can_support_peaks_overlay(self):
        self._assert_supports_peaks_overlay(True, IMDEventWorkspace, ndims=3)
        self._assert_supports_peaks_overlay(True, IMDHistoWorkspace, ndims=3)

    def test_title_for_matrixworkspace_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws2d_histo)

        self.assertEqual("Sliceviewer - ws2d_histo", model.get_title())

    def test_title_for_mdeventworkspace_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws_MDE_3D)

        self.assertEqual("Sliceviewer - ws_MDE_3D", model.get_title())

    def test_title_for_mdhistoworkspace_without_original_just_contains_ws_name(self):
        model = SliceViewerModel(self.ws_MD_3D)

        self.assertEqual("Sliceviewer - ws_MD_3D", model.get_title())

    def test_title_for_mdhistoworkspace_with_original(self):
        with _attach_as_original(self.ws_MD_3D, self.ws_MDE_3D):
            model = SliceViewerModel(self.ws_MD_3D)

            self.assertEqual("Sliceviewer - ws_MD_3D", model.get_title())

    def test_calculate_axes_angles_returns_none_if_nonorthogonal_transform_not_supported(self):
        model = SliceViewerModel(_create_mock_workspace(MatrixWorkspace, SpecialCoordinateSystem.QLab, has_oriented_lattice=False))

        self.assertIsNone(model.get_axes_angles())

    def test_calculate_axes_angles_uses_W_if_available_MDEvent(self):
        # test MD event
        ws = _create_mock_workspace(IMDEventWorkspace, SpecialCoordinateSystem.HKL, has_oriented_lattice=True)
        ws.getExperimentInfo().run().get().value = [0, 1, 1, 0, 0, 1, 1, 0, 0]
        model = SliceViewerModel(ws)

        axes_angles = model.get_axes_angles()
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 4, delta=1e-10)
        for iy in range(1, 3):
            self.assertAlmostEqual(axes_angles[0, iy], np.pi / 2, delta=1e-10)
        # test force_orthog works
        axes_angles = model.get_axes_angles(force_orthogonal=True)
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 2, delta=1e-10)

    def test_calculate_axes_angles_uses_basis_vectors_even_if_WMatrix_log_available_MDHisto(self):
        # test MD histo
        ws = _create_mock_workspace(IMDHistoWorkspace, SpecialCoordinateSystem.HKL, has_oriented_lattice=True)
        ws.getExperimentInfo().run().get().value = [0, 1, 1, 0, 0, 1, 1, 0, 0]
        model = SliceViewerModel(ws)

        # should revert to orthogonal (as given by basis vectors on workspace)
        # i.e. not angles returned by proj_matrix in ws.getExperimentInfo().run().get().value
        axes_angles = model.get_axes_angles()
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 2, delta=1e-10)
        for iy in range(1, 3):
            self.assertAlmostEqual(axes_angles[0, iy], np.pi / 2, delta=1e-10)

    def test_calculate_axes_angles_uses_identity_if_W_unavailable_MDEvent(self):
        # test MD event
        ws = _create_mock_workspace(IMDEventWorkspace, SpecialCoordinateSystem.HKL, has_oriented_lattice=True)
        ws.getExperimentInfo().run().get.side_effect = KeyError
        model = SliceViewerModel(ws)

        axes_angles = model.get_axes_angles()
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 2, delta=1e-10)
        for iy in range(1, 3):
            self.assertAlmostEqual(axes_angles[0, iy], np.pi / 2, delta=1e-10)

    def test_calculate_axes_angles_uses_identity_if_W_unavailable_MDHisto(self):
        # test MD histo
        ws = _create_mock_workspace(IMDHistoWorkspace, SpecialCoordinateSystem.HKL, has_oriented_lattice=True)
        ws.getExperimentInfo().run().get.side_effect = KeyError
        model = SliceViewerModel(ws)

        axes_angles = model.get_axes_angles()
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 2, delta=1e-10)
        for iy in range(1, 3):
            self.assertAlmostEqual(axes_angles[0, iy], np.pi / 2, delta=1e-10)

    def test_calculate_axes_angles_uses_W_if_basis_vectors_unavailable_and_W_available_MDHisto(self):
        # test MD histo
        ws = _create_mock_workspace(IMDHistoWorkspace, SpecialCoordinateSystem.HKL, has_oriented_lattice=True, ndims=3)
        ws.getBasisVector.side_effect = lambda x: [0.0]  # will cause proj_matrix to be all zeros
        ws.getExperimentInfo().run().get().value = [0, 1, 1, 0, 0, 1, 1, 0, 0]
        model = SliceViewerModel(ws)

        axes_angles = model.get_axes_angles()
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 4, delta=1e-10)
        for iy in range(1, 3):
            self.assertAlmostEqual(axes_angles[0, iy], np.pi / 2, delta=1e-10)
        # test force_orthog works
        axes_angles = model.get_axes_angles(force_orthogonal=True)
        self.assertAlmostEqual(axes_angles[1, 2], np.pi / 2, delta=1e-10)

    def test_calc_proj_matrix_4D_workspace_nonQ_dims(self):
        ws = _create_mock_histoworkspace(
            ndims=4,
            coords=SpecialCoordinateSystem.NONE,
            extents=(-3, 3, -10, 10, -1, 1, -2, 2),
            signal=np.arange(100.0).reshape(5, 5, 2, 2),
            error=np.arange(100.0).reshape(5, 5, 2, 2),
            nbins=(5, 5, 2, 2),
            names=("00L", "HH0", "-KK0", "E"),
            units=("rlu", "rlu", "rlu", "EnergyTransfer"),
            isq=(True, True, True, False),
        )
        # basis vectors as if ws was produced from BinMD call on MDEvent with dims E, H, K, L
        basis_mat = np.array([[0, 0, 0, 1], [0, 1, -1, 0], [0, 1, 1, 0], [1, 0, 0, 0]])
        ws.getBasisVector.side_effect = lambda idim: basis_mat[:, idim]
        model = SliceViewerModel(ws)

        proj_mat = model.get_proj_matrix()

        self.assertTrue(np.all(np.isclose(proj_mat, [[0, 1, -1], [0, 1, 1], [1, 0, 0]])))

    @patch.multiple("mantidqt.widgets.sliceviewer.models.roi", ExtractSpectra=DEFAULT, Rebin=DEFAULT, SumSpectra=DEFAULT, Transpose=DEFAULT)
    def test_export_roi_for_matrixworkspace(self, ExtractSpectra, **_):
        xmin, xmax, ymin, ymax = -1.0, 3.0, 2.0, 4.0

        def assert_call_as_expected(exp_xmin, exp_xmax, exp_start_index, exp_end_index, transpose, is_spectra):
            mock_ws = _create_mock_matrixworkspace(x_axis=[10, 20, 30], y_axis=[1, 2, 3, 4, 5], distribution=False, y_is_spectra=is_spectra)
            mock_ws.name.return_value = "mock_ws"
            model = SliceViewerModel(mock_ws)
            slicepoint, bin_params, dimension_indices = MagicMock(), MagicMock(), MagicMock()

            help_msg = model.export_roi_to_workspace(slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices)

            self.assertEqual("ROI created: mock_ws_roi", help_msg)
            if is_spectra:
                self.assertEqual(2, mock_ws.getAxis(1).indexOfValue.call_count)
            else:
                mock_ws.getAxis(1).extractValues.assert_called_once()

            ExtractSpectra.assert_called_once_with(
                InputWorkspace=mock_ws,
                OutputWorkspace="mock_ws_roi",
                XMin=exp_xmin,
                XMax=exp_xmax,
                StartWorkspaceIndex=exp_start_index,
                EndWorkspaceIndex=exp_end_index,
                EnableLogging=True,
            )
            ExtractSpectra.reset_mock()

        assert_call_as_expected(xmin, xmax, 3, 5, transpose=False, is_spectra=True)
        assert_call_as_expected(2.0, 4.0, 0, 4, transpose=True, is_spectra=True)
        assert_call_as_expected(xmin, xmax, 1, 3, transpose=False, is_spectra=False)
        assert_call_as_expected(ymin, ymax, 0, 2, transpose=True, is_spectra=False)

    @patch("mantidqt.widgets.sliceviewer.models.roi.ExtractSpectra")
    @patch("mantidqt.widgets.sliceviewer.models.roi.Rebin")
    @patch("mantidqt.widgets.sliceviewer.models.roi.SumSpectra")
    @patch("mantidqt.widgets.sliceviewer.models.roi.Transpose")
    def test_export_cuts_for_matrixworkspace(self, mock_transpose, mock_sum_spectra, mock_rebin, mock_extract_spectra):
        xmin, xmax, ymin, ymax = -4.0, 5.0, 6.0, 9.0

        def assert_call_as_expected(mock_ws, transpose, export_type, is_spectra, is_ragged):
            model = SliceViewerModel(mock_ws)
            slicepoint, bin_params, dimension_indices = MagicMock(), MagicMock(), MagicMock()

            help_msg = model.export_cuts_to_workspace(
                slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices, export_type
            )

            if export_type == "c":
                if is_spectra:
                    mock_extract_spectra.assert_called_once()
                    if is_ragged:
                        mock_rebin.assert_called_once()
                    mock_sum_spectra.assert_called_once()
                else:
                    if is_ragged:
                        self.assertEqual(2, mock_rebin.call_count)
                    else:
                        mock_rebin.assert_called_once()
                    self.assertEqual(1, mock_transpose.call_count)
                    self.assertEqual(1, mock_extract_spectra.call_count)
                self.assertEqual("Cuts along X/Y created: mock_ws_cut_x & mock_ws_cut_y", help_msg)
            elif export_type == "x":
                mock_extract_spectra.assert_called_once()
                if is_ragged:
                    self.assertEqual(1, mock_rebin.call_count)
                if is_spectra:
                    mock_sum_spectra.assert_called_once()
                else:
                    mock_transpose.assert_not_called()
                self.assertEqual("Cut along X created: mock_ws_cut_x", help_msg)
            elif export_type == "y":
                mock_extract_spectra.assert_called_once()
                mock_transpose.assert_called_once()
                if is_ragged:
                    self.assertEqual(2, mock_rebin.call_count)
                else:
                    self.assertEqual(1, mock_rebin.call_count)
                self.assertEqual("Cut along Y created: mock_ws_cut_y", help_msg)

            mock_transpose.reset_mock()
            mock_rebin.reset_mock()
            mock_extract_spectra.reset_mock()
            mock_sum_spectra.reset_mock()

        # end def

        for export_type in ("c", "x", "y"):
            is_ragged = False
            for is_spectra in (True, False):
                mock_ws = _create_mock_matrixworkspace(
                    x_axis=[10, 20, 30], y_axis=[1, 2, 3, 4, 5], distribution=False, y_is_spectra=is_spectra
                )
                mock_ws.name.return_value = "mock_ws"
                assert_call_as_expected(mock_ws, transpose=False, export_type=export_type, is_spectra=is_spectra, is_ragged=is_ragged)

            is_ragged = True
            mock_ws = _create_mock_matrixworkspace(
                x_axis=[10, 20, 30, 11, 21, 31], y_axis=[1, 2, 3, 4, 5], distribution=False, y_is_spectra=is_spectra
            )
            mock_ws.isCommonBins.return_value = False
            mock_ws.name.return_value = "mock_ws"
            assert_call_as_expected(mock_ws, transpose=False, export_type=export_type, is_spectra=is_spectra, is_ragged=is_ragged)

    @patch("mantidqt.widgets.sliceviewer.models.model.IntegrateMDHistoWorkspace")
    @patch("mantidqt.widgets.sliceviewer.models.model.TransposeMD")
    def test_export_pixel_cut_to_workspace_mdhisto(self, mock_transpose, mock_intMD):
        slicepoint = [None, None, 0.5]
        bin_params = [100, 100, 0.1]
        dimension_indices = [0, 1, None]

        def assert_call_as_expected(transpose, cut_type):
            xpos, ypos = 0.0, 2.0
            model = SliceViewerModel(self.ws_MD_3D)
            help_msg = model.export_pixel_cut_to_workspace(slicepoint, bin_params, (xpos, ypos), transpose, dimension_indices, cut_type)

            dim0 = self.ws_MD_3D.getDimension(0)
            dim1 = self.ws_MD_3D.getDimension(1)

            dim0_delta = dim0.getBinWidth()
            dim1_delta = dim1.getBinWidth()

            if cut_type == "x" and not transpose or cut_type == "y" and transpose:
                xmin, xmax = dim0.getMinimum(), dim0.getMaximum()
                ymin, ymax = ypos - 0.5 * dim1_delta, ypos + 0.5 * dim1_delta
            else:
                xmin, xmax = xpos - 0.5 * dim0_delta, xpos + 0.5 * dim0_delta
                ymin, ymax = dim1.getMinimum(), dim1.getMaximum()

            zmin, zmax = 0.45, 0.55

            if not transpose:
                if cut_type == "x":
                    xcut_params = dict(
                        InputWorkspace=self.ws_MD_3D,
                        P1Bin=[xmin, 0, xmax],
                        P2Bin=[ymin, ymax],
                        P3Bin=[zmin, zmax],
                        OutputWorkspace="ws_MD_3D_cut_x",
                    )
                    mock_intMD.assert_called_once_with(**xcut_params)
                    self.assertEqual("Cut along X created: ws_MD_3D_cut_x", help_msg)
                elif cut_type == "y":
                    ycut_params = dict(
                        InputWorkspace=self.ws_MD_3D,
                        P1Bin=[xmin, xmax],
                        P2Bin=[ymin, 0, ymax],
                        P3Bin=[zmin, zmax],
                        OutputWorkspace="ws_MD_3D_cut_y",
                    )
                    mock_intMD.assert_called_once_with(**ycut_params)
                    self.assertEqual("Cut along Y created: ws_MD_3D_cut_y", help_msg)
            else:
                if cut_type == "x":
                    xcut_params = dict(
                        InputWorkspace=self.ws_MD_3D,
                        P1Bin=[xmin, xmax],
                        P2Bin=[ymin, 0, ymax],
                        P3Bin=[zmin, zmax],
                        OutputWorkspace="ws_MD_3D_cut_x",
                    )
                    mock_intMD.assert_called_once_with(**xcut_params)
                    self.assertEqual("Cut along X created: ws_MD_3D_cut_x", help_msg)
                elif cut_type == "y":
                    ycut_params = dict(
                        InputWorkspace=self.ws_MD_3D,
                        P1Bin=[xmin, 0, xmax],
                        P2Bin=[ymin, ymax],
                        P3Bin=[zmin, zmax],
                        OutputWorkspace="ws_MD_3D_cut_y",
                    )
                    mock_intMD.assert_called_once_with(**ycut_params)
                    self.assertEqual("Cut along Y created: ws_MD_3D_cut_y", help_msg)

            mock_intMD.reset_mock()

        for cut_type in ("x", "y"):
            assert_call_as_expected(transpose=False, cut_type=cut_type)
            assert_call_as_expected(transpose=True, cut_type=cut_type)

    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    @patch("mantidqt.widgets.sliceviewer.models.model.TransposeMD")
    def test_export_pixel_cut_to_workspace_mdevent(self, mock_transpose, mock_binmd):
        slicepoint = [None, None, 0.5]
        bin_params = [100, 100, 0.1]
        dimension_indices = [0, 1, None]

        def assert_call_as_expected(transpose, cut_type):
            xpos, ypos = 0.0, 2.0
            model = SliceViewerModel(self.ws_MDE_3D)
            help_msg = model.export_pixel_cut_to_workspace(slicepoint, bin_params, (xpos, ypos), transpose, dimension_indices, cut_type)

            dim0 = self.ws_MDE_3D.getDimension(0)
            dim1 = self.ws_MDE_3D.getDimension(1)

            self.assertEqual(dim0.getBinWidth(), 6)
            self.assertEqual(dim1.getBinWidth(), 8)

            if transpose:
                xpos, ypos = ypos, xpos
                dim0, dim1 = dim1, dim0

            dim0_delta = dim0.getBinWidth()
            dim1_delta = dim1.getBinWidth()

            if cut_type == "x":
                xmin, xmax = dim0.getMinimum(), dim0.getMaximum()
                ymin, ymax = ypos - 0.5 * dim1_delta, ypos + 0.5 * dim1_delta
            else:
                xmin, xmax = xpos - 0.5 * dim0_delta, xpos + 0.5 * dim0_delta
                ymin, ymax = dim1.getMinimum(), dim1.getMaximum()

            zmin, zmax = 0.45, 0.55

            common_params = dict(
                InputWorkspace=self.ws_MDE_3D,
                AxisAligned=False,
                BasisVector0="h,rlu,1.0,0.0,0.0",
                BasisVector1="k,rlu,0.0,1.0,0.0",
                BasisVector2="l,rlu,0.0,0.0,1.0",
            )

            if not transpose:
                extents = [xmin, xmax, ymin, ymax, zmin, zmax]

                if cut_type == "x":
                    expected_bins = [100, 1, 1]
                    expected_ws = "ws_MDE_3D_cut_x"
                    expected_msg = "Cut along X created: ws_MDE_3D_cut_x"
                else:
                    expected_bins = [1, 100, 1]
                    expected_ws = "ws_MDE_3D_cut_y"
                    expected_msg = "Cut along Y created: ws_MDE_3D_cut_y"
            else:
                extents = [ymin, ymax, xmin, xmax, zmin, zmax]

                if cut_type == "x":
                    expected_bins = [1, 100, 1]
                    expected_ws = "ws_MDE_3D_cut_x"
                    expected_msg = "Cut along X created: ws_MDE_3D_cut_x"
                else:
                    expected_bins = [100, 1, 1]
                    expected_ws = "ws_MDE_3D_cut_y"
                    expected_msg = "Cut along Y created: ws_MDE_3D_cut_y"

            mock_binmd.assert_called_once_with(
                **common_params, OutputExtents=extents, OutputBins=expected_bins, OutputWorkspace=expected_ws
            )

            self.assertEqual(expected_msg, help_msg)
            mock_binmd.reset_mock()

        for cut_type in ("x", "y"):
            assert_call_as_expected(transpose=False, cut_type=cut_type)
            assert_call_as_expected(transpose=True, cut_type=cut_type)

    @patch("mantidqt.widgets.sliceviewer.models.roi.extract_roi_matrix")
    def test_export_pixel_cut_to_workspace_matrix(self, mock_extract_roi):
        slicepoint = [None, None]
        bin_params = [100, 100]
        dimension_indices = [0, 1]
        xpos, ypos = 1.5, 3.0

        def assert_call_as_expected(transpose, cut_type):
            model = SliceViewerModel(self.ws2d_histo)
            mock_extract_roi.reset_mock()

            help_msg = model.export_pixel_cut_to_workspace(slicepoint, bin_params, (xpos, ypos), transpose, dimension_indices, cut_type)

            if not transpose:
                if cut_type == "x":
                    mock_extract_roi.assert_called_once_with(self.ws2d_histo, None, None, ypos, ypos, False, "ws2d_histo_cut_x")
                    self.assertEqual("Cut along X created: ws2d_histo_cut_x", help_msg)
                elif cut_type == "y":
                    mock_extract_roi.assert_called_once_with(self.ws2d_histo, xpos, xpos, None, None, True, "ws2d_histo_cut_y")
                    self.assertEqual("Cut along Y created: ws2d_histo_cut_y", help_msg)
            else:
                if cut_type == "x":
                    mock_extract_roi.assert_called_once_with(self.ws2d_histo, ypos, ypos, None, None, True, "ws2d_histo_cut_y")
                    self.assertEqual("Cut along X created: ws2d_histo_cut_y", help_msg)
                elif cut_type == "y":
                    mock_extract_roi.assert_called_once_with(self.ws2d_histo, None, None, xpos, xpos, False, "ws2d_histo_cut_x")
                    self.assertEqual("Cut along Y created: ws2d_histo_cut_x", help_msg)

            for cut_type in ("x", "y"):
                assert_call_as_expected(transpose=False, cut_type=cut_type)
                assert_call_as_expected(transpose=False, cut_type=cut_type)

    @patch("mantidqt.widgets.sliceviewer.models.model.TransposeMD")
    @patch("mantidqt.widgets.sliceviewer.models.model.IntegrateMDHistoWorkspace")
    def test_export_region_for_mdhisto_workspace(self, mock_intMD, mock_transposemd):
        xmin, xmax, ymin, ymax = -1.0, 3.0, 2.0, 4.0
        slicepoint, bin_params = (None, None, 0.5), (100, 100, 0.1)
        dimension_indices = [0, 1, None]  # Value at index i is the index of the axis that dimension i is displayed on
        transposed_dimension_indices = [1, 0, None]

        def assert_call_as_expected(transpose, dimension_indices, export_type, bin_params):
            model = SliceViewerModel(self.ws_MD_3D)

            if export_type == "r":
                model.export_roi_to_workspace(slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices)
            else:
                model.export_cuts_to_workspace(
                    slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices, export_type
                )

            if export_type == "c":
                export_type = "xy"  # will loop over this string as 'c' performs both 'x' and 'y'

            for export in export_type:
                xbin, ybin = [xmin, xmax], [ymin, ymax]  # create in loop as these are altered in case of both cuts
                # perform transpose on limits - i.e map x/y on plot to basis of MD workspace p1/p2
                if not transpose:
                    p1_bin, p2_bin = xbin, ybin
                else:
                    p2_bin, p1_bin = xbin, ybin
                # determine which axis was binnned
                if export == "x":
                    xbin.insert(1, 0.0)  # insert 0 between min,max - this means preserve binning along this dim
                    out_name = "ws_MD_3D_cut_x"
                    transpose_axes = [1 if transpose else 0]  # Axes argument of TransposeMD
                elif export == "y":
                    ybin.insert(1, 0.0)
                    out_name = "ws_MD_3D_cut_y"
                    # check call to transposeMD
                    transpose_axes = [0 if transpose else 1]
                else:
                    # export == 'r'
                    xbin.insert(1, 0.0)
                    ybin.insert(1, 0.0)
                    out_name = "ws_MD_3D_roi"
                    transpose_axes = [1, 0] if transpose else None

                dim = self.ws_MD_3D.getDimension(2)
                half_bin_width = bin_params[2] / 2 if bin_params is not None else dim.getBinWidth() / 2

                # check call to IntegrateMDHistoWorkspace
                mock_intMD.assert_has_calls(
                    [
                        call(
                            InputWorkspace=self.ws_MD_3D,
                            P1Bin=p1_bin,
                            P2Bin=p2_bin,
                            P3Bin=[slicepoint[2] - half_bin_width, slicepoint[2] + half_bin_width],
                            OutputWorkspace=out_name,
                        )
                    ],
                    any_order=False,
                )
                if transpose_axes is not None:
                    mock_transposemd.assert_has_calls(
                        [call(InputWorkspace=out_name, OutputWorkspace=out_name, Axes=transpose_axes)], any_order=False
                    )
                else:
                    mock_transposemd.assert_not_called()  # ROI with Transpose == False

            mock_intMD.reset_mock()
            mock_transposemd.reset_mock()

        for export_type in ("r", "x", "y", "c"):
            assert_call_as_expected(transpose=False, dimension_indices=dimension_indices, export_type=export_type, bin_params=bin_params)
            assert_call_as_expected(transpose=False, dimension_indices=dimension_indices, export_type=export_type, bin_params=None)
            assert_call_as_expected(
                transpose=True, dimension_indices=transposed_dimension_indices, export_type=export_type, bin_params=bin_params
            )
            assert_call_as_expected(
                transpose=True, dimension_indices=transposed_dimension_indices, export_type=export_type, bin_params=None
            )

    @patch("mantidqt.widgets.sliceviewer.models.model.TransposeMD")
    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    def test_export_region_for_mdevent_workspace(self, mock_binmd, mock_transposemd):
        xmin, xmax, ymin, ymax = -1.0, 3.0, 2.0, 4.0
        slicepoint, bin_params = (None, None, 0.5), (100, 100, 0.1)
        zmin, zmax = 0.45, 0.55  # 3rd dimension extents
        dimension_indices = [0, 1, None]  # Value at index i is the index of the axis that dimension i is displayed on
        transposed_dimension_indices = [1, 0, None]

        def assert_call_as_expected(transpose, dimension_indices, export_type):
            model = SliceViewerModel(self.ws_MDE_3D)

            if export_type == "r":
                help_msg = model.export_roi_to_workspace(slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices)
            else:
                help_msg = model.export_cuts_to_workspace(
                    slicepoint, bin_params, ((xmin, xmax), (ymin, ymax)), transpose, dimension_indices, export_type
                )

            if transpose:
                extents = [ymin, ymax, xmin, xmax, zmin, zmax]
            else:
                extents = [xmin, xmax, ymin, ymax, zmin, zmax]
            common_call_params = dict(
                InputWorkspace=self.ws_MDE_3D,
                AxisAligned=False,
                BasisVector0="h,rlu,1.0,0.0,0.0",
                BasisVector1="k,rlu,0.0,1.0,0.0",
                BasisVector2="l,rlu,0.0,0.0,1.0",
                OutputExtents=extents,
            )
            xcut_name, ycut_name = "ws_MDE_3D_cut_x", "ws_MDE_3D_cut_y"
            if export_type == "r":
                expected_help_msg = "ROI created: ws_MDE_3D_roi"
                expected_calls = [call(**common_call_params, OutputBins=[100, 100, 1], OutputWorkspace="ws_MDE_3D_roi")]
            elif export_type == "x":
                expected_help_msg = f"Cut along X created: {xcut_name}"
                expected_bins = [1, 100, 1] if transpose else [100, 1, 1]
                expected_calls = [call(**common_call_params, OutputBins=expected_bins, OutputWorkspace=xcut_name)]
            elif export_type == "y":
                expected_help_msg = f"Cut along Y created: {ycut_name}"
                expected_bins = [100, 1, 1] if transpose else [1, 100, 1]
                expected_calls = [call(**common_call_params, OutputBins=expected_bins, OutputWorkspace=ycut_name)]
            elif export_type == "c":
                expected_help_msg = f"Cuts along X/Y created: {xcut_name} & {ycut_name}"
                expected_bins = [100, 1, 1] if transpose else [1, 100, 1]
                expected_calls = [
                    call(**common_call_params, OutputBins=expected_bins, OutputWorkspace=xcut_name),
                    call(**common_call_params, OutputBins=expected_bins, OutputWorkspace=ycut_name),
                ]

            mock_binmd.assert_has_calls(expected_calls, any_order=True)
            if export_type == "r":
                if transpose:
                    mock_transposemd.assert_called_once()
                else:
                    mock_transposemd.assert_not_called()
            else:
                if export_type == "x":
                    index = 1 if transpose else 0
                    expected_calls = [call(InputWorkspace=xcut_name, OutputWorkspace=xcut_name, Axes=[index])]
                elif export_type == "y":
                    index = 0 if transpose else 1
                    expected_calls = [call(InputWorkspace=ycut_name, OutputWorkspace=ycut_name, Axes=[index])]
                elif export_type == "c":
                    xindex = 1 if transpose else 0
                    yindex = 0 if transpose else 1
                    expected_calls = [
                        call(InputWorkspace=xcut_name, OutputWorkspace=xcut_name, Axes=[xindex]),
                        call(InputWorkspace=ycut_name, OutputWorkspace=ycut_name, Axes=[yindex]),
                    ]

                mock_transposemd.assert_has_calls(expected_calls, any_order=True)

            self.assertEqual(expected_help_msg, help_msg)
            mock_binmd.reset_mock()
            mock_transposemd.reset_mock()

        for export_type in ("r", "x", "y", "c"):
            assert_call_as_expected(transpose=False, dimension_indices=dimension_indices, export_type=export_type)
            assert_call_as_expected(transpose=True, dimension_indices=transposed_dimension_indices, export_type=export_type)

    @patch("mantidqt.widgets.sliceviewer.models.model.BinMD")
    @patch("mantidqt.widgets.sliceviewer.models.roi.ExtractSpectra")
    def test_export_region_raises_exception_if_operation_failed(self, mock_extract_spectra, mock_binmd):
        def assert_error_returned_in_help(workspace, export_type, mock_alg, err_msg):
            model = SliceViewerModel(workspace)
            slicepoint, bin_params, dimension_indices = (None, None, None), MagicMock(), [0, 1, None]
            mock_alg.side_effect = RuntimeError(err_msg)
            try:
                if export_type == "r":
                    help_msg = model.export_roi_to_workspace(slicepoint, bin_params, ((1.0, 2.0), (-1, 2.0)), True, dimension_indices)
                else:
                    help_msg = model.export_cuts_to_workspace(
                        slicepoint, bin_params, ((1.0, 2.0), (-1, 2.0)), True, dimension_indices, export_type
                    )
            except Exception as exc:
                help_msg = str(exc)
            mock_alg.reset_mock()

            self.assertTrue(err_msg in help_msg)

        for export_type in ("r", "c", "x", "y"):
            assert_error_returned_in_help(self.ws2d_histo, export_type, mock_extract_spectra, "ExtractSpectra failed")
        for export_type in ("r", "c", "x", "y"):
            assert_error_returned_in_help(self.ws_MDE_3D, export_type, mock_binmd, "BinMD failed")

    @patch("mantidqt.widgets.sliceviewer.models.model.SliceViewerModel.get_proj_matrix")
    def test_get_hkl_from_full_point_returns_zeros_for_a_none_transform(self, mock_get_proj_matrix):
        qdims = [0, 1, 2]
        point_3d = [1.0, 2.0, 3.0]

        model = SliceViewerModel(self.ws_MDE_3D)
        mock_get_proj_matrix.return_value = None

        hkl = model.get_hkl_from_full_point(point_3d, qdims)

        self.assertEqual((0.0, 0.0, 0.0), hkl)

    @patch("mantidqt.widgets.sliceviewer.models.model.SliceViewerModel.get_proj_matrix")
    def test_get_hkl_from_full_point_for_3D_point(self, mock_get_proj_matrix):
        qdims = [0, 1, 2]
        point_3d = [1.0, 2.0, 3.0]  # [x, y, z] = [h, k, l]

        model = SliceViewerModel(self.ws_MDE_3D)
        mock_get_proj_matrix.return_value = np.array([[0, 1, -1], [0, 1, 1], [1, 0, 0]])

        hkl = model.get_hkl_from_full_point(point_3d, qdims)

        self.assertEqual([-1.0, 5.0, 1.0], list(hkl))

    @patch("mantidqt.widgets.sliceviewer.models.model.SliceViewerModel.get_proj_matrix")
    def test_get_hkl_from_full_point_for_4D_point(self, mock_get_proj_matrix):
        qdims = [1, 2, 3]
        point_4d = [1.0, 2.0, 3.0, 4.0]  # [y, x, z, ...] = [e, h, k, l]

        model = SliceViewerModel(self.ws_MDE_4D)
        mock_get_proj_matrix.return_value = np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]])

        hkl = model.get_hkl_from_full_point(point_4d, qdims)

        self.assertEqual([2.0, 3.0, 4.0], list(hkl))

    @patch("mantidqt.widgets.sliceviewer.models.model.SliceViewerModel.get_proj_matrix")
    def test_get_hkl_from_full_point_for_4D_point_with_transformation(self, mock_get_proj_matrix):
        qdims = [1, 2, 3]
        point_4d = [1.0, 2.0, 3.0, 4.0]  # [y, z, ..., x] = [e, h, k, l]

        model = SliceViewerModel(self.ws_MDE_4D)
        mock_get_proj_matrix.return_value = np.array([[2, 0, 0], [0, -1, 0], [0, 0, 3]])

        hkl = model.get_hkl_from_full_point(point_4d, qdims)

        self.assertEqual([4.0, -3.0, 12.0], list(hkl))

    @patch("mantidqt.widgets.sliceviewer.models.model.SliceViewerModel.number_of_active_original_workspaces")
    def test_check_for_removed_original_workspace(self, mock_num_original_workspaces):
        self.ws_MDE_4D.hasOriginalWorkspace.side_effect = lambda index: True
        mock_num_original_workspaces.return_value = 1

        model = SliceViewerModel(self.ws_MDE_4D)

        self.assertEqual(model.num_original_workspaces_at_init, 1)
        self.assertFalse(model.check_for_removed_original_workspace())
        # original workspace has been deleted
        mock_num_original_workspaces.return_value = 0
        self.assertTrue(model.check_for_removed_original_workspace())

    # private
    def _assert_supports_non_orthogonal_axes(self, expectation, ws_type, coords, has_oriented_lattice):
        model = SliceViewerModel(_create_mock_workspace(ws_type, coords, has_oriented_lattice))
        self.assertEqual(expectation, model.can_support_nonorthogonal_axes())

    def _assert_supports_non_axis_aligned_cuts(self, expectation, ws_type, coords=SpecialCoordinateSystem.HKL, ndims=3):
        model = SliceViewerModel(_create_mock_workspace(ws_type, coords, has_oriented_lattice=False, ndims=ndims))
        self.assertEqual(expectation, model.can_support_non_axis_cuts())

    def _assert_supports_peaks_overlay(self, expectation, ws_type, ndims=2):
        ws = _create_mock_workspace(ws_type, coords=SpecialCoordinateSystem.QLab, has_oriented_lattice=False, ndims=ndims)
        model = SliceViewerModel(ws)
        self.assertEqual(expectation, model.can_support_peaks_overlays())


if __name__ == "__main__":
    unittest.main()
