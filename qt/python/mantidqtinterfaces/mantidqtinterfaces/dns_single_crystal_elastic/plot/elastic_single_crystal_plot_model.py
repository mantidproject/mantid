# ruff: noqa: E741  # Ambiguous variable name
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab model of DNS reduction GUI.
"""

import numpy as np
import mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers as helper
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map import DNSScMap
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers import get_hkl_intensity_from_cursor


class DNSElasticSCPlotModel(DNSObsModel):
    """
    Model for DNS plot calculations. It converts data from (omega, 2theta)
    space into (q_x, q_y) and (n_x, n_y). Also, creates hull of DNS data
    to filter points.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self._single_crystal_map = None

        self._data = ObjectDict()
        self._data.x = None
        self._data.y = None
        self._data.z = None
        # x_, y_, and z_lims are used for storing full data (default) lims for plotting
        self._data.x_lims = None
        self._data.y_lims = None
        self._data.z_lims = None
        self._data.z_min = None
        self._data.z_max = None
        self._data.pz_min = None
        self._data.triang = None
        self._data.z_triang = None

    def create_single_crystal_map(self, data_array, options, initial_values=None):
        two_theta = data_array["two_theta_array"]
        omega = data_array["omega_array"]
        z_mesh = data_array["intensity"]
        error = data_array["error"]
        parameter = {
            "wavelength": options["wavelength"],
            "dx": options["dx"],
            "dy": options["dy"],
            "hkl1": options["hkl1"],
            "hkl2": options["hkl2"],
            "omega_offset": options["omega_offset"],
        }
        if initial_values is not None:
            parameter.update(initial_values)
        self._single_crystal_map = DNSScMap(parameter=parameter, two_theta=two_theta, omega=omega, z_mesh=z_mesh, error_mesh=error)
        return self._single_crystal_map

    def has_data(self):
        return self._data.x is not None

    def get_projections(self, xlim, ylim):
        limits = np.append(xlim, ylim)
        x, y, z = helper.filter_flattened_meshes(self._data.x, self._data.y, self._data.z, limits)
        x_projection = helper.get_projection(x, z)
        y_projection = helper.get_projection(y, z)
        return x_projection, y_projection

    def generate_triangulation_mesh(self, interpolate, axis_type, switch):
        mesh_name = axis_type + "_mesh"
        self._single_crystal_map.triangulate(mesh_name=mesh_name, switch=switch)
        self._single_crystal_map.mask_triangles(mesh_name=mesh_name)
        triangulator_refiner, z_refiner = self._single_crystal_map.interpolate_triangulation(interpolate)
        self._data.triang = triangulator_refiner
        self._data.z_triang = z_refiner
        # this is important to get the limits
        x, y, z = getattr(self._single_crystal_map, mesh_name)
        x, y, z = self.switch_axis(x, y, z, switch)
        self.set_mesh_data(x, y, z)
        return triangulator_refiner, z_refiner

    def generate_quad_mesh(self, interpolate, axis_type, switch):
        new_mesh_name = axis_type + "_mesh_interpolated"
        old_mesh_name = axis_type + "_mesh"
        if interpolate:
            self._single_crystal_map.interpolate_quad_mesh(interpolate)
            x, y, z = getattr(self._single_crystal_map, new_mesh_name)
        else:
            x, y, z = getattr(self._single_crystal_map, old_mesh_name)
        x, y, z = self.switch_axis(x, y, z, switch)
        self.set_mesh_data(x, y, z)
        return x, y, z

    def generate_scatter_mesh(self, axis_type, switch):
        x, y, z = getattr(self._single_crystal_map, axis_type + "_mesh")
        x, y, z = self.switch_axis(x, y, z, switch)
        self.set_mesh_data(x, y, z)
        return x, y, z

    def get_xy_dy_ratio(self):
        return self._single_crystal_map.dx / self._single_crystal_map.dy

    def get_aspect_ratio(self, plot_settings_dict):
        if plot_settings_dict["fix_aspect"]:
            if plot_settings_dict["type"] == "hkl":
                ratio = self.get_xy_dy_ratio()
                return ratio
            return 1
        return "auto"

    def get_axis_labels(self, axis_type, crystal_axes, switch=False):
        if crystal_axes:
            return self._single_crystal_map.get_crystal_axis_names()
        hkl1 = self._single_crystal_map.hkl1
        hkl2 = self._single_crystal_map.hkl2
        axis_labels = {
            "angular": ["2\u03b8 (deg)", "\u03c9 (deg)"],
            "qxqy": [r"$q_{x} \ (\AA^{-1})$", r"$q_{y} \ (\AA^{-1})$"],
            "hkl": [f"[{hkl1}] (r.l.u.)", f"[{hkl2}] (r.l.u.)"],
        }
        labels = axis_labels[axis_type]
        if switch:
            labels.reverse()
        return labels

    def get_changing_hkl_components(self):
        return self._single_crystal_map.get_changing_hkl_components()

    def get_format_coord(self, plot_settings_dict):
        # adds z and hkl label to cursor position
        def format_coord(x, y):
            mesh_name = plot_settings_dict["type"] + "_mesh"
            border_path = self._single_crystal_map.get_dns_map_border(mesh_name)
            h, k, l, z, error = get_hkl_intensity_from_cursor(self._single_crystal_map, plot_settings_dict, x, y)
            # ensures empty hover in the region outside the data boundary
            if border_path.contains_point((x, y)):
                return f"x={x:2.3f}, y={y:2.3f}, hkl=({h:2.2f}, {k:2.2f}, {l:2.2f}), Intensity={z:6.4f}±{error:6.4f}"
            return f"x={x:2.3f}, y={y:2.3f}, hkl=({h:2.2f}, {k:2.2f}, {l:2.2f})"

        return format_coord

    def get_data_z_min_max(self, xlim=None, ylim=None):
        return helper.get_z_min_max(self._data.z, xlim, ylim, self._data.x, self._data.y)

    def get_data_xy_lim(self, switch):
        limits = [[min(self._data.x.flatten()), max(self._data.x.flatten())], [min(self._data.y.flatten()), max(self._data.y.flatten())]]
        if switch:
            limits.reverse()
        return limits

    @staticmethod
    def switch_axis(x, y, z, switch):
        if switch:  # switch x and y axes
            nx = np.transpose(y)
            ny = np.transpose(x)
            nz = np.transpose(z)
            return nx, ny, nz
        return x, y, z

    def get_omega_offset(self):
        return self._single_crystal_map["omega_offset"]

    def get_dx_dy(self):
        return self._single_crystal_map["dx"], self._single_crystal_map["dy"]

    def prepare_data_for_saving(self):
        x = self._data.x.flatten()
        y = self._data.y.flatten()
        z = self._data.z.flatten()
        data_combined = np.array(list(zip(x, y, z)))
        return data_combined

    def set_mesh_data(self, x, y, z):
        self._data.x = x
        self._data.y = y
        self._data.z = z
        self.save_default_data_lims()

    def save_default_data_lims(self):
        x_lims, y_lims = self.get_data_xy_lim(switch=False)
        z_min, z_max, pos_z_min = self.get_data_z_min_max()
        # add 5% padding to comply with default plotting settings
        x0, x1 = x_lims[0], x_lims[1]
        y0, y1 = y_lims[0], y_lims[1]
        dx = (x1 - x0) * 0.05
        dy = (y1 - y0) * 0.05
        self._data.x_lims = [x0 - dx, x1 + dx]
        self._data.y_lims = [y0 - dy, y1 + dy]
        self._data.z_lims = [z_min, z_max]

    def get_default_data_lims(self):
        return self._data.x_lims, self._data.y_lims, self._data.z_lims
