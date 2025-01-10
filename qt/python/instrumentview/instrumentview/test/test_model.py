# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
import unittest
from unittest import mock
import numpy as np


class TestFullInstrumentViewModel(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewModel", XUnit="TOF")

    def setUp(self):
        self._model = FullInstrumentViewModel(self._ws, False)

    def test_union_with_current_bin_min_max(self):
        current_min = self._model._bin_min
        current_max = self._model._bin_max
        self._model._union_with_current_bin_min_max(current_min - 1)
        self.assertEqual(self._model._bin_min, current_min - 1)
        self._model._union_with_current_bin_min_max(current_min)
        self.assertEqual(self._model._bin_min, current_min - 1)
        self.assertEqual(self._model._bin_max, current_max)
        self._model._union_with_current_bin_min_max(current_max + 1)
        self.assertEqual(self._model._bin_max, current_max + 1)

    def test_update_time_of_flight_range(self):
        integrated_spectra = list(range(len(self._ws.spectrumInfo())))
        self._model._workspace = mock.MagicMock()
        self._model._workspace.getIntegratedSpectra.return_value = integrated_spectra
        self._model.update_time_of_flight_range(200, 10000, False)
        self._model._workspace.getIntegratedSpectra.assert_called_once()
        self.assertEqual(min(integrated_spectra), self._model._data_min)
        self.assertEqual(max(integrated_spectra), self._model._data_max)

    @mock.patch("pyvista.PolyData")
    def test_draw_rectangular_bank(self, mock_polyData):
        component_info = mock.MagicMock()
        component_info.quadrilateralComponentCornerIndices.return_value = [0, 1, 2, 3]
        component_info.position.return_value = [2, 2, 2]
        component_info.scaleFactor.return_value = [1, 1, 1]
        self._model.drawRectangularBank(component_info, 0)
        mock_polyData.assert_called_once()
        polyData_args = mock_polyData.call_args_list[0][0]
        self.assertEqual((4, 3), polyData_args[0].shape)
        self.assertTrue(polyData_args[0].all(where=lambda x: x == 2))
        self.assertEqual([4, 0, 1, 2, 3], polyData_args[1])

    @mock.patch("pyvista.Sphere")
    def test_draw_single_detector_sphere(self, mock_sphere):
        component_info = self._setup_draw_single_detector("SPHERE", [[0, 0, 0]], [10, 10])
        self._model.drawSingleDetector(component_info, 0)
        mock_sphere.assert_called_once()

    @mock.patch("pyvista.UnstructuredGrid")
    def test_draw_single_detector_cuboid(self, mock_unstructedGrid):
        component_info = self._setup_draw_single_detector("CUBOID", [[0, 0, 0], [1, 1, 1], [2, 2, 2], [3, 3, 3]], [10, 10])
        self._model.drawSingleDetector(component_info, 0)
        mock_unstructedGrid.assert_called_once()

    @mock.patch("pyvista.UnstructuredGrid")
    def test_draw_single_detector_hexahedron(self, mock_unstructuredGrid):
        component_info = self._setup_draw_single_detector("HEXAHEDRON", [[0, 0, 0], [1, 1, 1], [2, 2, 2], [3, 3, 3]] * 2, [])
        self._model.drawSingleDetector(component_info, 0)
        mock_unstructuredGrid.assert_called_once()

    @mock.patch("pyvista.Cone")
    def test_draw_single_detector_cone(self, mock_cone):
        component_info = self._setup_draw_single_detector("CONE", [[0, 0, 0], [1, 1, 1]], [1, 10, 5])
        self._model.drawSingleDetector(component_info, 0)
        mock_cone.assert_called_once()

    @mock.patch("pyvista.Cylinder")
    def test_draw_single_detector_cylinder(self, mock_cylinder):
        component_info = self._setup_draw_single_detector("CYLINDER", [[0, 0, 0], [1, 1, 1]], [1, 10, 5])
        self._model.drawSingleDetector(component_info, 0)
        mock_cylinder.assert_called_once()

    @mock.patch("pyvista.PolyData")
    def test_draw_single_detector_other(self, mock_polyData):
        component_info = self._setup_draw_single_detector("unknown", [], [])
        self._model.drawSingleDetector(component_info, 0)
        mock_polyData.assert_called_once()

    def _setup_draw_single_detector(self, shape: str, points: list[list[float]], dimensions: list[int]) -> mock.MagicMock:
        component_info = mock.MagicMock()
        component_info.position.return_value = [1, 1, 1]
        mock_rotation = mock.MagicMock()
        mock_rotation.getAngleAxis.return_value = [0, 0, 1, 0]
        component_info.rotation.return_value = mock_rotation
        component_info.scaleFactor.return_value = [1, 1, 1]
        mock_shape = mock.MagicMock()
        mock_shape.getGeometryShape.return_value = shape
        mock_shape.getGeometryPoints.return_value = points
        mock_shape.getGeometryDimensions.return_value = dimensions
        mock_shape.getMesh.return_value = np.array(
            [
                [[0.025, 0.025, 0.02], [0.025, 0.025, 0.0], [-0.025, 0.025, 0.0]],
                [[0.025, 0.025, 0.02], [-0.025, 0.025, 0.0], [-0.025, 0.025, 0.02]],
                [[0.025, 0.025, 0.02], [-0.025, 0.025, 0.02], [-0.025, -0.025, 0.02]],
            ]
        )
        component_info.shape.return_value = mock_shape
        return component_info

    @mock.patch("instrumentview.FullInstrumentViewModel.DetectorInfo")
    def test_get_detector_info_text(self, mock_detectorInfo):
        self._model.get_detector_info_text(0)
        mock_detectorInfo.assert_called_once()

    @mock.patch("instrumentview.Projections.spherical_projection.spherical_projection")
    def test_calculate_spherical_projection(self, mock_spherical_projection):
        self._run_projection_test(mock_spherical_projection, True)

    @mock.patch("instrumentview.Projections.cylindrical_projection.cylindrical_projection")
    def test_calculate_cylindrical_projection(self, mock_cylindrical_projection):
        self._run_projection_test(mock_cylindrical_projection, False)

    def _run_projection_test(self, mock_projection_constructor, is_spherical):
        mock_projection = mock.MagicMock()
        mock_projection.coordinate_for_detector.return_value = (1, 2)
        mock_projection_constructor.return_value = mock_projection
        points = self._model.calculate_projection(is_spherical, axis=[0, 1, 0])
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(point == [1, 2, 0] for point in points))
