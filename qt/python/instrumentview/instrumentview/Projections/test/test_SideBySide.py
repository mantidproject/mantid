# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.SideBySide import SideBySide, FlatBankInfo

import numpy as np
from unittest.mock import MagicMock, patch
import unittest
from mantid.kernel import Quat


class TestSideBySideProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root_position = np.array([0, 0, 0])
        cls.sample_position = np.array([0, 0, 0])
        cls.detector_positions = np.array([[0, 1, 0], [2, 1, 0], [-2, 1, 0]])

    def _create_flat_bank_info(self, use_relative_positions: bool, detector_ids: list[int] | None = None) -> FlatBankInfo:
        flat_bank = FlatBankInfo()
        flat_bank.reference_position = np.zeros(3)
        flat_bank.detector_ids = detector_ids if detector_ids is not None else [5, 10, 15]
        if use_relative_positions:
            flat_bank.relative_projected_positions = self.detector_positions.copy()
        else:
            flat_bank.steps = [1, 1, 1]
            flat_bank.pixels = [3, 1, 0]

        return flat_bank

    def test_flat_bank_info_translate_relative(self):
        flat_bank = self._create_flat_bank_info(use_relative_positions=True)
        flat_bank._transform_positions_so_origin_bottom_left = MagicMock()
        flat_bank.calculate_projected_positions()
        flat_bank.translate([1, 1, 1])
        np.testing.assert_allclose([1, 1, 1], flat_bank.reference_position)
        # We should have translated all points by [1,1,1]
        np.testing.assert_allclose([[1, 2, 1], [3, 2, 1], [-1, 2, 1]], list(flat_bank.detector_id_position_map.values()))
        flat_bank._transform_positions_so_origin_bottom_left.assert_called_once()

    def test_flat_bank_info_translate_pixels(self):
        flat_bank = self._create_flat_bank_info(use_relative_positions=False)
        flat_bank._transform_positions_so_origin_bottom_left = MagicMock()
        flat_bank.calculate_projected_positions()
        flat_bank.translate([-5, 10, 0])
        np.testing.assert_allclose([-5, 10, 0], flat_bank.reference_position)
        # Initial positions are from pixels and steps, then translated
        np.testing.assert_allclose([[-5, 10, 0], [-4, 10, 0], [-3, 10, 0]], list(flat_bank.detector_id_position_map.values()))
        flat_bank._transform_positions_so_origin_bottom_left.assert_not_called()

    def test_flat_bank_transform_positions_to_bottom_left(self):
        flat_bank = self._create_flat_bank_info(use_relative_positions=True)
        flat_bank._transform_positions_so_origin_bottom_left()
        np.testing.assert_allclose([[2, 0, 0], [4, 0, 0], [0, 0, 0]], list(flat_bank.relative_projected_positions))

    def test_flat_bank_calculate_projected_positions_relative(self):
        flat_bank = self._create_flat_bank_info(use_relative_positions=True)
        flat_bank.calculate_projected_positions()
        np.testing.assert_allclose([[2, 0, 0], [4, 0, 0], [0, 0, 0]], list(flat_bank.detector_id_position_map.values()))

    def test_flat_bank_calculate_projected_positions_pixels(self):
        flat_bank = self._create_flat_bank_info(use_relative_positions=False)
        flat_bank.calculate_projected_positions()
        np.testing.assert_allclose([[0, 0, 0], [1, 0, 0], [2, 0, 0]], list(flat_bank.detector_id_position_map.values()))

    def _create_side_by_side(self, detector_ids: list[int], has_other_detectors: bool) -> SideBySide:
        mock_workspace = MagicMock()
        mock_detector_info = MagicMock()
        mock_detector_info.detectorIDs.return_value = detector_ids + [100] if has_other_detectors else detector_ids
        mock_workspace.detectorInfo.return_value = mock_detector_info
        side_by_side = SideBySide(
            workspace=mock_workspace,
            detector_ids=np.array(detector_ids),
            sample_position=np.zeros(3),
            root_position=np.zeros(3),
            detector_positions=np.array([[1, 1, 1], [0, 1, 0], [0, 0, 1]]),
            axis=np.array([0, 0, 1]),
        )
        return side_by_side

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_create_side_by_side_no_extra_detectors(self, mock_panels_surface_calculator):
        side_by_side = self._create_side_by_side([5, 10, 15], False)
        side_by_side._workspace.detectorInfo().detectorIDs.assert_called_once()
        self.assertEqual(list(range(3)), list(side_by_side._component_index_detector_id_map.keys()))
        self.assertEqual([5, 10, 15], list(side_by_side._component_index_detector_id_map.values()))
        mock_panels_surface_calculator.assert_called_once()

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_create_side_by_side_subset_detectors(self, mock_panels_surface_calculator):
        side_by_side = self._create_side_by_side([5, 10, 15], True)
        side_by_side._workspace.detectorInfo().detectorIDs.assert_called_once()
        self.assertEqual(list(range(3)), list(side_by_side._component_index_detector_id_map.keys()))
        self.assertEqual([5, 10, 15], list(side_by_side._component_index_detector_id_map.values()))
        mock_panels_surface_calculator.assert_called_once()

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_calculate_axes(self, mock_panels_surface_calculator):
        side_by_side = self._create_side_by_side(list(range(3)), False)
        side_by_side._calculator.setupBasisAxes.assert_called_once()
        args = side_by_side._calculator.setupBasisAxes.call_args_list[0][0]
        self.assertEquals(3, len(args))
        np.testing.assert_allclose([0, 0, 0], args[0])
        np.testing.assert_allclose([0, 0, 0], args[1])
        np.testing.assert_allclose([0, 0, 1], args[2])

    @patch("instrumentview.Projections.SideBySide.SideBySide._arrange_panels")
    @patch("instrumentview.Projections.Projection.Projection._calculate_detector_coordinates")
    @patch("instrumentview.Projections.SideBySide.SideBySide._create_flat_bank_with_missing_detectors")
    @patch("instrumentview.Projections.SideBySide.SideBySide._construct_tube_banks")
    @patch("instrumentview.Projections.SideBySide.SideBySide._construct_rectangles_and_grids")
    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_construct_flat_panels(
        self,
        mock_panels_surface_calculator,
        mock_construct_rectangles,
        mock_construct_tube_banks,
        mock_create_flat_bank,
        mock_calculate_coordinates,
        mock_arrange_panels,
    ):
        side_by_side = self._create_side_by_side([5, 10, 15, 20, 25], True)
        mock_construct_rectangles.return_value = [self._create_flat_bank_info(True, detector_ids=[5, 10, 15])]
        mock_construct_tube_banks.return_value = [self._create_flat_bank_info(True, detector_ids=[20, 25])]
        mock_create_flat_bank.return_value = None
        side_by_side._construct_flat_panels(side_by_side._workspace)
        detector_map = side_by_side._detector_id_to_flat_bank_map
        self.assertEqual([5, 10, 15, 20, 25], list(detector_map.keys()))
        mock_arrange_panels.assert_called_once()
        self.assertEqual(2, len(side_by_side._flat_banks))
        mock_construct_rectangles.assert_called_once()
        mock_construct_tube_banks.assert_called_once()

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_construct_rectangles_and_grids(self, mock_panels_surface_calculator):
        side_by_side = self._create_side_by_side([5, 10, 15, 20, 25], True)
        ws = side_by_side._workspace
        mock_bank = MagicMock()
        mock_bank.getPos.return_value = [2, 2, 2]
        mock_bank.getRotation.return_value = Quat(1, 1, 0, 0)
        mock_bank.minDetectorID.return_value = 5
        mock_bank.maxDetectorID.return_value = 5
        ws.getInstrument().findGridDetectors.return_value = [mock_bank]
        flat_banks = side_by_side._construct_rectangles_and_grids(side_by_side._workspace)
        self.assertEqual(1, len(flat_banks))
        self.assertEqual([5], flat_banks[0].detector_ids)
        side_by_side._calculator.getSideBySideViewPos.assert_called_once()

    @patch("instrumentview.Projections.Projection.Projection._calculate_detector_coordinates")
    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_construct_tube_banks(self, mock_panels_surface_calculator, mock_calculate_detector_coords):
        detector_IDs = [5, 10, 15, 20, 25]
        side_by_side = self._create_side_by_side(detector_IDs, True)
        side_by_side._component_index_detector_id_map = {id: id for id in detector_IDs}
        mock_component_info = side_by_side._workspace.componentInfo()
        side_by_side._calculator.getAllTubeDetectorFlatGroupParents.return_value = [[3]]
        side_by_side._calculator.calculateBankNormal.return_value = [0, 0, -1]
        side_by_side._calculator.calcBankRotation.return_value = Quat(1, 0, 1, 0)
        mock_component_info.children.return_value = detector_IDs
        mock_component_info.position.return_value = [0, 1, 0]
        tube_banks = side_by_side._construct_tube_banks(mock_component_info)
        self.assertEqual(1, len(tube_banks))
        self.assertEqual(detector_IDs, tube_banks[0].detector_ids)
        side_by_side._calculator.getAllTubeDetectorFlatGroupParents.assert_called_once_with(mock_component_info)
        side_by_side._calculator.calculateBankNormal.assert_called_once()
        side_by_side._calculator.calcBankRotation.assert_called_once()

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_calculate_projected_positions_plane(self, mock_panels_surface_calculator):
        # Put three points in a plane, should get the same points back
        side_by_side = self._create_side_by_side([5, 10, 15], False)
        points = np.array([[0, 1, 0], [1, 0, 0], [1, 1, 0]])
        normal = np.array([0, 0, -1])
        flat_bank = self._create_flat_bank_info(True)
        flat_bank.reference_position = np.zeros(3)
        projected_points = side_by_side._calculate_projected_positions(points, normal, flat_bank)
        self.assertEqual(3, len(projected_points))
        np.testing.assert_allclose(points, projected_points)

    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_calculate_projected_positions_angle(self, mock_panels_surface_calculator):
        # Points above/below plane should be projected correctly
        side_by_side = self._create_side_by_side([5, 10, 15], False)
        points = np.array([[0, 1, 0], [1, 0, 0.5], [1, 1, -0.5]])
        normal = np.array([0, 0, -1])
        flat_bank = self._create_flat_bank_info(True)
        flat_bank.reference_position = np.zeros(3)
        projected_points = side_by_side._calculate_projected_positions(points, normal, flat_bank)
        self.assertEqual(3, len(projected_points))
        np.testing.assert_allclose([[0, 1, 0], [1, 0, 0], [1, 1, 0]], projected_points)

    @patch("instrumentview.Projections.Projection.Projection._calculate_detector_coordinates")
    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_create_flat_bank_with_missing_detectors(self, mock_panels_surface_calculator, mock_calculate_detector_coords):
        side_by_side = self._create_side_by_side([5, 10, 15, 20, 25], False)
        flat_bank = side_by_side._create_flat_bank_with_missing_detectors(np.array([5]))
        self.assertTrue(type(flat_bank) is FlatBankInfo)
        self.assertEqual([10, 15, 20, 25], flat_bank.detector_ids)
        separation = 0.01
        # Four remaining detectors should be arranged in a square with the default separation between them
        np.testing.assert_allclose(
            [[0, 0, 0], [0, separation, 0], [separation, 0, 0], [separation, separation, 0]], flat_bank.relative_projected_positions
        )

    @patch("instrumentview.Projections.Projection.Projection._calculate_detector_coordinates")
    @patch("instrumentview.Projections.SideBySide.PanelsSurfaceCalculator")
    def test_arrange_panels(self, mock_panels_surface_calculator, mock_calculate_detector_coords):
        detector_ids = [5, 10, 15, 20, 25]
        side_by_side = self._create_side_by_side(detector_ids, False)
        mock_flat_bank = MagicMock()
        mock_flat_bank.dimensions = [1, 1, 1]
        mock_flat_bank.has_position_in_idf = False
        mock_flat_bank.reference_position = [5, 5, 5]
        side_by_side._flat_banks = [mock_flat_bank]
        side_by_side._arrange_panels()
        mock_flat_bank.translate.assert_called_once()


if __name__ == "__main__":
    unittest.main()
