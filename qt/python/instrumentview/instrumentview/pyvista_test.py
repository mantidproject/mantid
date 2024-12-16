from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QWidget, QPlainTextEdit
import pyvista as pv
from pyvistaqt import BackgroundPlotter
import numpy as np
from scipy.spatial.transform import Rotation
from pathlib import Path
import matplotlib.pyplot as plt
import instrumentview.Projections.spherical_projection as iv_spherical
import instrumentview.Projections.cylindrical_projection as iv_cylindrical
from mantid.simpleapi import LoadRaw, LoadNexus, LoadEventNexus


class DetectorPosition(np.ndarray):
    def __new__(cls, input_array):
        return np.asarray(input_array).view(cls)

    def __eq__(self, other):
        return np.allclose(self, other)


class MainWindow(QMainWindow):
    m_Sample_Position = None
    m_Source_Position = None
    m_detector_position_map = dict()
    m_detector_spectrum_fig = None
    m_detector_spectrum_axes = None
    m_workspace = None
    m_detector_info_text = None
    m_detector_counts = None
    m_detector_mesh = None
    m_detector_index_to_workspace_index = None
    m_invalid_index = -1
    m_detector_indices = []

    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)
        self.setWindowTitle("PyVista in Qt")

        # Create a central widget and set the layout
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # Create a PyVista plotter and add it to the layout
        self.plotter = BackgroundPlotter(show=False)
        layout.addWidget(self.plotter.app_window)
        self.projection_plotter = BackgroundPlotter(show=False)
        layout.addWidget(self.projection_plotter.app_window)

        pv.global_theme.color_cycler = "default"

        # self.plotWorkspace(Path(r"C:\Tutorial\SampleData-ISIS\SXD23767.raw"), False)
        # self.plotWorkspace(Path(r"C:\Tutorial\UsageData\CNCS_7860_event.nxs"), False, is_event=True)
        # self.plotWorkspace(Path(r"C:\Tutorial\SampleData-ISIS\INTER00013470.nxs"), True)
        self.plotWorkspace(Path(r"C:\Tutorial\SampleData-ISIS\MAR11060.raw"), True)
        # self.plotWorkspace(Path(r"C:\Temp\sans2d.nxs"), False)

        # Plot origin
        origin = pv.Sphere(radius=0.01, center=[0, 0, 0])
        self.plotter.add_mesh(origin, color="orange", pickable=False)

        self.plotter.enable_point_picking(show_message=False, callback=self.point_picked, use_picker=True)
        # self.plotter.enable_rectangle_picking(show_message=False, callback=self.rectangle_picked, use_picker=True)
        # self.plotter.enable_eye_dome_lighting()
        self.plotter.show_axes()
        self.plotter.camera.focal_point = self.m_Sample_Position

    def plotWorkspace(self, wsPath: Path, draw_detector_geometry: bool, is_event: bool = False) -> None:
        if is_event:
            ws = LoadEventNexus(str(wsPath), StoreInADS=False)
        elif wsPath.suffix == ".nxs":
            ws = LoadNexus(str(wsPath), StoreInADS=False)
        else:
            ws = LoadRaw(str(wsPath), StoreInADS=False)

        # ws = LoadEmptyInstrument(InstrumentName="GEM")

        self.m_workspace = ws
        detector_info = ws.detectorInfo()

        component_info = ws.componentInfo()
        self.m_Sample_Position = np.array(component_info.samplePosition())
        self.m_Source_Position = np.array(component_info.sourcePosition())
        monitor_indices = []
        monitor_points = []
        component_meshes = []
        component_mesh_colours = []

        self.m_detector_index_to_workspace_index = np.full(len(component_info), self.m_invalid_index, dtype=int)
        spectrum_info = ws.spectrumInfo()
        for workspace_index in range(ws.getNumberHistograms()):
            spectrum_definition = spectrum_info.getSpectrumDefinition(workspace_index)
            for i in range(len(spectrum_definition)):
                pair = spectrum_definition[i]
                self.m_detector_index_to_workspace_index[pair[0]] = workspace_index

        for component_index in range(component_info.root(), -1, -1):
            component_type = component_info.componentTypeName(component_index)
            match component_type:
                case "Infinite":
                    continue
                case "Grid":
                    continue
                case "Rectangular":
                    rectangular_bank_mesh = self.drawRectangularBank(component_info, component_index)
                    component_meshes.append(rectangular_bank_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case "OutlineComposite":
                    outline_composite_mesh = self.drawSingleDetector(component_info, component_index)
                    component_meshes.append(outline_composite_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case "Structured":
                    structured_mesh = self.drawStructuredBank(component_info, component_index)
                    component_meshes.append(structured_mesh)
                    component_mesh_colours.append((0.1, 0.1, 0.1, 0.2))
                    continue
                case _:
                    if not component_info.isDetector(component_index):
                        continue
                    if detector_info.isMonitor(component_index):
                        monitor_points.append(detector_info.position(component_index))
                        monitor_indices.append(component_index)
                    elif component_info.hasValidShape(component_index):
                        self.m_detector_indices.append(component_index)
                        if draw_detector_geometry:
                            component_meshes.append(self.drawSingleDetector(component_info, component_index))
                    else:
                        continue

        data_min = 0
        data_max = 0
        self.m_detector_counts = []
        integrated_spectra = ws.getIntegratedSpectra(0, 0, True)
        for det_index in self.m_detector_indices:
            workspace_index = int(self.m_detector_index_to_workspace_index[det_index])
            if workspace_index == self.m_invalid_index or det_index in monitor_indices:
                continue
            self.m_detector_counts.append(integrated_spectra[workspace_index])
            data_max = max(data_max, integrated_spectra[workspace_index])
            data_min = min(data_min, integrated_spectra[workspace_index])

        self.m_detector_position_map = {id: DetectorPosition(component_info.position(id)) for id in self.m_detector_indices}
        self.m_detector_mesh = self.createPolyDataMesh(list(self.m_detector_position_map.values()))
        self.m_detector_mesh["Integrated Counts"] = self.m_detector_counts

        self.plotter.add_mesh(
            self.m_detector_mesh, scalars="Integrated Counts", clim=[data_min, data_max], render_points_as_spheres=True, point_size=7
        )

        monitor_point_cloud = self.createPolyDataMesh(monitor_points)
        monitor_point_cloud["colours"] = self.generateSingleColour(monitor_points, 1, 0, 0, 1)

        cmap = plt.get_cmap("viridis")
        colours = [cmap(np.clip((d - data_min) / (data_max - data_min), 0, 1)) for d in self.m_detector_counts]

        if len(component_meshes) > 0:
            multiblock = pv.MultiBlock(component_meshes)
            multiblock.combine()
            self.plotter.add_mesh(multiblock, multi_colors=colours, pickable=False)

        self.plotter.add_mesh(monitor_point_cloud, scalars="colours", rgba=True, render_points_as_spheres=True, point_size=10)

        self.calculate_projection(data_min, data_max, is_spherical=True)
        # self.calculate_projection(data_min, data_max, is_spherical=False)

    def generateSingleColour(self, points, red: float, green: float, blue: float, alpha: float) -> np.ndarray:
        rgba = np.zeros((len(points), 4))
        rgba[:, 0] = red
        rgba[:, 1] = green
        rgba[:, 2] = blue
        rgba[:, 3] = alpha
        return rgba

    def drawRectangularBank(self, component_info, component_index: int) -> pv.PolyData:
        corner_indices = component_info.quadrilateralComponentCornerIndices(component_index)
        corner_positions = [np.array(component_info.position(corner_index)) for corner_index in corner_indices]
        scale = np.array(component_info.scaleFactor(component_index))
        corner_positions = corner_positions * scale
        # Number of points for each face, followed by the indices of the vertices
        faces = [4, 0, 1, 2, 3]
        return self.createPolyDataMesh(corner_positions, faces)

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

    def createPolyDataMesh(self, points, faces=None) -> pv.PolyData:
        mesh = pv.PolyData(points, faces)
        return mesh

    def show_plot_for_detectors(self, detector_indices):
        if self.m_detector_spectrum_fig is None:
            self.m_detector_spectrum_fig, self.m_detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
            self.m_detector_spectrum_fig.canvas.mpl_connect("close_event", self.on_figure_close)
            self.m_detector_spectrum_fig.canvas.manager.window.move(0, 0)

        self.m_detector_spectrum_axes.clear()
        try:
            _ = iter(detector_indices)
        except TypeError:
            self.plot_detector(detector_indices, self.m_detector_spectrum_axes)
        else:
            for d in detector_indices:
                self.plot_detector(d, self.m_detector_spectrum_axes)

        self.m_detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)
        self.m_detector_spectrum_fig.show()
        plt.draw()

    def plot_detector(self, detector_index, axes):
        workspace_index = self.workspace_index_from_detector_index(detector_index)
        axes.plot(self.m_workspace, label=self.m_workspace.name() + "Workspace Index " + str(workspace_index), wkspIndex=workspace_index)

    def show_info_text_for_detectors(self, detector_indices):
        if self.m_detector_info_text is None:
            self.m_detector_info_text = QPlainTextEdit()
            self.m_detector_info_text.setReadOnly(True)
            self.m_detector_info_text.move(self.geometry().bottomLeft().x() + 100, 0)

        try:
            _ = iter(detector_indices)
        except TypeError:
            info_string = self.get_detector_info_text(detector_indices)
        else:
            info_string = "\n\n".join(self.get_detector_info_text(d) for d in detector_indices)

        self.m_detector_info_text.setPlainText(info_string)
        self.m_detector_info_text.show()

    def workspace_index_from_detector_index(self, detector_index: int) -> int:
        return int(self.m_detector_index_to_workspace_index[detector_index])

    def point_picked(self, point, picker):
        if point is None:
            return
        point_index = picker.GetPointId()
        detector_index = self.m_detector_indices[point_index]
        self.show_plot_for_detectors(detector_index)
        self.show_info_text_for_detectors(detector_index)

    def rectangle_picked(self, rectangle):
        selected_points = rectangle.frustum_mesh.points
        points = set([self.m_detector_mesh.find_closest_point(p) for p in selected_points])
        self.show_plot_for_detectors(points)
        self.show_info_text_for_detectors(points)

    def on_figure_close(self, event):
        self.m_detector_spectrum_fig = None
        self.m_detector_spectrum_axes = None

    def get_detector_info_text(self, detector_index: int) -> str:
        workspace_index = self.workspace_index_from_detector_index(detector_index)
        component_info = self.m_workspace.componentInfo()
        detector_info_lines = ["Selected detector: " + component_info.name(detector_index)]
        detector_id = self.m_workspace.detectorInfo().detectorIDs()[detector_index]
        detector_info_lines.append("Detector ID: " + str(detector_id))
        detector_info_lines.append("Workspace index: " + str(workspace_index))
        position = component_info.position(detector_index)
        detector_info_lines.append("xyz: " + str(position.X()) + "," + str(position.Y()) + "," + str(position.Z()))
        spherical = position.getSpherical()
        detector_info_lines.append("rtp: " + str(spherical[0]) + "," + str(spherical[1]) + "," + str(spherical[2]))

        if component_info.hasParent(detector_index):
            parent = detector_index
            component_path = ""
            while component_info.hasParent(parent):
                parent = component_info.parent(parent)
                component_path = "/" + component_info.name(parent) + component_path
            detector_info_lines.append("Component path:" + component_path + "/" + component_info.name(detector_index))
            counts = self.m_detector_counts[detector_index]
            detector_info_lines.append("Pixel counts: " + str(counts))

        return "\n".join(s for s in detector_info_lines)

    def calculate_projection(self, data_min: float, data_max: float, is_spherical: bool):
        spherical = (
            iv_spherical.spherical_projection(self.m_workspace, self.m_detector_indices, np.array([1, 0, 0]))
            if is_spherical
            else iv_cylindrical.cylindrical_projection(self.m_workspace, self.m_detector_indices, np.array([1, 0, 0]))
        )
        projection_points = []
        for det_id in range(len(self.m_detector_indices)):
            x, y = spherical.coordinate_for_detector(det_id)
            projection_points.append([x, y, 0])

        projection_mesh = self.createPolyDataMesh(projection_points)
        projection_mesh["Integrated Counts"] = self.m_detector_counts
        self.projection_plotter.add_mesh(
            projection_mesh, scalars="Integrated Counts", clim=[data_min, data_max], render_points_as_spheres=True, point_size=7
        )
        self.projection_plotter.view_xy()
        self.projection_plotter.enable_image_style()
