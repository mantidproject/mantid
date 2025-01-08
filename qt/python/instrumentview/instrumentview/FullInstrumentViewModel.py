# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.DetectorInfo import DetectorInfo
import instrumentview.Projections.spherical_projection as iv_spherical
import instrumentview.Projections.cylindrical_projection as iv_cylindrical
import numpy as np
import math
import pyvista as pv
from scipy.spatial.transform import Rotation


class DetectorPosition(np.ndarray):
    def __new__(cls, input_array):
        return np.asarray(input_array).view(cls)

    def __eq__(self, other):
        return np.allclose(self, other)


class FullInstrumentViewModel:
    _sample_position = np.array([0, 0, 0])
    _source_position = np.array([0, 0, 0])
    _invalid_index = -1
    _data_min = 0.0
    _data_max = 0.0

    def __init__(self, workspace, draw_detector_geometry: bool):
        self._workspace = workspace
        self._detector_info = workspace.detectorInfo()

        self._component_info = workspace.componentInfo()
        self._sample_position = np.array(self._component_info.samplePosition())
        self._source_position = np.array(self._component_info.sourcePosition())

        self._detector_indices = []
        self._monitor_positions = []
        self._monitor_indices = []
        component_meshes = []
        component_mesh_colours = []

        self._detector_index_to_workspace_index = np.full(len(self._component_info), self._invalid_index, dtype=int)
        spectrum_info = workspace.spectrumInfo()
        self._bin_min = math.inf
        self._bin_max = -math.inf
        for workspace_index in range(workspace.getNumberHistograms()):
            x_data = self._workspace.dataX(workspace_index)
            self._union_with_current_bin_min_max(x_data[0])
            self._union_with_current_bin_min_max(x_data[-1])
            spectrum_definition = spectrum_info.getSpectrumDefinition(workspace_index)
            for i in range(len(spectrum_definition)):
                pair = spectrum_definition[i]
                self._detector_index_to_workspace_index[pair[0]] = workspace_index

        for component_index in range(self._component_info.root(), -1, -1):
            component_type = self._component_info.componentTypeName(component_index)
            match component_type:
                case "Infinite":
                    continue
                case "Grid":
                    continue
                case "Rectangular":
                    rectangular_bank_mesh = self.drawRectangularBank(self._component_info, component_index)
                    component_meshes.append(rectangular_bank_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case "OutlineComposite":
                    outline_composite_mesh = self.drawSingleDetector(self._component_info, component_index)
                    component_meshes.append(outline_composite_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case "Structured":
                    structured_mesh = self.drawStructuredBank(self._component_info, component_index)
                    component_meshes.append(structured_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case _:
                    if not self._component_info.isDetector(component_index):
                        continue
                    if self._detector_info.isMonitor(component_index):
                        self._monitor_positions.append(self._detector_info.position(component_index))
                        self._monitor_indices.append(component_index)
                    elif self._component_info.hasValidShape(component_index):
                        self._detector_indices.append(component_index)
                        if draw_detector_geometry:
                            component_meshes.append(self.drawSingleDetector(self._component_info, component_index))
                    else:
                        continue

        self._detector_counts = []
        self.update_time_of_flight_range(self._bin_min, self._bin_max, True)
        self._detector_position_map = {id: DetectorPosition(self._component_info.position(id)) for id in self._detector_indices}

    def _union_with_current_bin_min_max(self, bin_edge) -> None:
        if not math.isinf(bin_edge):
            if bin_edge < self._bin_min:
                self._bin_min = bin_edge
            elif bin_edge > self._bin_max:
                self._bin_max = bin_edge

    def update_time_of_flight_range(self, tof_min: float, tof_max: float, entire_range=False) -> None:
        integrated_spectra = self._workspace.getIntegratedSpectra(tof_min, tof_max, entire_range)
        self._detector_counts.clear()
        for det_index in self._detector_indices:
            workspace_index = int(self._detector_index_to_workspace_index[det_index])
            if workspace_index == self._invalid_index or det_index in self._monitor_indices:
                continue
            self._detector_counts.append(integrated_spectra[workspace_index])
            self._data_max = max(self._data_max, integrated_spectra[workspace_index])
            self._data_min = min(self._data_min, integrated_spectra[workspace_index])

    def workspace(self):
        return self._workspace

    def workspace_index_from_detector_index(self, detector_index: int) -> int:
        return int(self._detector_index_to_workspace_index[detector_index])

    def sample_position(self) -> np.ndarray:
        return self._sample_position

    def detector_positions(self) -> list:
        return list(self._detector_position_map.values())

    def detector_counts(self) -> list:
        return self._detector_counts

    def detector_index(self, i: int) -> int:
        return self._detector_indices[i]

    def data_limits(self) -> list:
        return [self._data_min, self._data_max]

    def bin_limits(self) -> list:
        return [self._bin_min, self._bin_max]

    def monitor_positions(self) -> list:
        return self._monitor_positions

    def drawRectangularBank(self, component_info, component_index: int) -> pv.PolyData:
        corner_indices = component_info.quadrilateralComponentCornerIndices(component_index)
        corner_positions = [np.array(component_info.position(corner_index)) for corner_index in corner_indices]
        scale = np.array(component_info.scaleFactor(component_index))
        corner_positions = corner_positions * scale
        # Number of points for each face, followed by the indices of the vertices
        faces = [4, 0, 1, 2, 3]
        return pv.PolyData(corner_positions, faces)

    def drawStructuredBank(self, component_info, component_index: int) -> pv.PolyData:
        bank_corner_indices = component_info.quadrilateralComponentCornerIndices(component_index)
        bank_corner_positions = [np.array(component_info.position(corner_index)) for corner_index in bank_corner_indices]
        shape = component_info.shape(component_index)
        shape_points = [np.array(point) for point in shape.getGeometryPoints()]
        position = np.array(shape_points[0])
        position[2] = bank_corner_positions[1][2]  # Bottom-left Z

        columns = component_info.children(component_index)
        column_width = len(columns) * 3
        base_column_index = component_info(columns[0])[0]
        base_shape = component_info.shape(base_column_index)
        base_points = base_shape.getGeometryPoints()
        base_position = np.array(base_points[0])

        vertices = []
        # faces = []

        for index in range(0, column_width, 3):
            column_index = int(index / 3)
            column = component_info.children(columns[column_index])
            for column_child_index in len(column):
                y = column[column_child_index]
                child_shape = component_info.shape(y)
                child_position = np.array(component_info.position(y))
                child_rotation = component_info.rotation(y).getAngleAxis()
                rotation = Rotation.from_rotvec(
                    child_rotation[0] * np.array([child_rotation[1], child_rotation[2], child_rotation[3]]), degrees=True
                )
                child_shape_points = child_shape.getGeometryPoints()
                child_shape_points = [np.array(rotation.apply(p)) + position - base_position + child_position for p in child_shape_points]
                vertices.append(child_shape_points)

        return pv.PolyData(vertices)

    def drawSingleDetector(self, component_info, component_index: int):
        detector_position = np.array(component_info.position(component_index))
        mantid_rotation = component_info.rotation(component_index).getAngleAxis()
        rotation = Rotation.from_rotvec(
            mantid_rotation[0] * np.array([mantid_rotation[1], mantid_rotation[2], mantid_rotation[3]]), degrees=True
        )
        scale = np.array(component_info.scaleFactor(component_index))
        shape = component_info.shape(component_index)
        shape_type = shape.getGeometryShape()
        shape_points = [np.array(point) for point in shape.getGeometryPoints()]
        shape_dimensions = shape.getGeometryDimensions()
        match shape_type:
            case "SPHERE":
                centre = shape_points[0] + detector_position
                centre = rotation.apply(centre)
                centre = np.multiply(centre, scale)
                radius = shape_dimensions[1]
                return pv.Sphere(radius, centre)
            case "CUBOID":
                vec0 = shape_points[0] + detector_position
                vec1 = shape_points[1] - shape_points[0]
                vec2 = shape_points[2] - shape_points[0]
                vec3 = shape_points[3] - shape_points[0]
                hex_points = [
                    vec0,
                    vec0 + vec3,
                    vec0 + vec3 + vec1,
                    vec0 + vec1,
                    vec0 + vec1,
                    vec0 + vec2 + vec3,
                    vec0 + vec2 + vec3 + vec1,
                    vec0 + vec1 + vec2,
                ]
                hex_points = [vertex * scale for vertex in hex_points]
                connectivity = np.array([8, 0, 1, 2, 3, 4, 5, 6, 7])
                return pv.UnstructuredGrid(connectivity, [pv.CellType.HEXAHEDRON], hex_points)
            case "HEXAHEDRON":
                hex_points = [(point + detector_position) * scale for point in shape_points]
                connectivity = np.array([8, 0, 1, 2, 3, 4, 5, 6, 7])
                return pv.UnstructuredGrid(connectivity, [pv.CellType.HEXAHEDRON], hex_points)
            case "CONE" | "CYLINDER":
                centre = shape_points[0] + detector_position
                cylinder_axis = shape_points[1]
                radius = shape_dimensions[1]
                height = shape_dimensions[2]
                centre = centre + 0.5 * height * cylinder_axis
                cylinder_rotation = Rotation.concatenate([Rotation.align_vectors([0, 0, 1], cylinder_axis)[0], rotation])
                cylinder_axis = cylinder_rotation.apply(cylinder_axis)[1]
                centre = np.multiply(centre, scale)
                cylinder_axis = np.multiply(cylinder_axis, scale)
                if shape_type == "CYLINDER":
                    return pv.Cylinder(centre, cylinder_axis, radius, height, 20)
                return pv.Cone(centre, cylinder_axis, height, radius)
            case _:
                mesh = shape.getMesh()
                if len(mesh) == 0:
                    return None
                mesh_points = np.array([rotation.apply(p) + detector_position for p in mesh.reshape(-1, 3)])
                faces = np.hstack((3 * np.ones((len(mesh), 1)), np.arange(mesh_points.shape[0]).reshape(-1, 3))).astype(int)
                return pv.PolyData(mesh_points, faces)

    def get_detector_info_text(self, detector_index: int) -> DetectorInfo:
        workspace_index = self.workspace_index_from_detector_index(detector_index)
        name = self._component_info.name(detector_index)
        detector_id = self._detector_info.detectorIDs()[detector_index]
        xyz_position = self._component_info.position(detector_index)
        spherical_position = xyz_position.getSpherical()

        component_path = ""
        pixel_counts = 0
        if self._component_info.hasParent(detector_index):
            parent = detector_index
            component_path = ""
            while self._component_info.hasParent(parent):
                parent = self._component_info.parent(parent)
                component_path = "/" + self._component_info.name(parent) + component_path
            pixel_counts = self.detector_counts()[detector_index]

        return DetectorInfo(
            name, detector_id, workspace_index, np.array(xyz_position), np.array(spherical_position), component_path, int(pixel_counts)
        )

    def calculate_projection(self, is_spherical: bool, axis: np.ndarray) -> list[float]:
        projection = (
            iv_spherical.spherical_projection(self._workspace, self._detector_indices, axis)
            if is_spherical
            else iv_cylindrical.cylindrical_projection(self._workspace, self._detector_indices, axis)
        )
        projection_points = []
        for det_id in range(len(self._detector_indices)):
            x, y = projection.coordinate_for_detector(det_id)
            projection_points.append([x, y, 0])
        return projection_points
