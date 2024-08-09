# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab model of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map import DNSScMap
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict


class DNSElasticSCPlotModel(DNSObsModel):
    """
    Model for DNS plot calculations.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self._single_crystal_map = None

        self._data = ObjectDict()
        self._data.x = None
        self._data.y = None
        self._data.z = None
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

    def get_interpolated_triangulation(self, interpolate, axis_type, switch):
        mesh_name = axis_type + "_mesh"
        self._single_crystal_map.triangulate(mesh_name=mesh_name, switch=switch)
        self._single_crystal_map.mask_triangles(mesh_name=mesh_name)
        triangulator_refiner, z_refiner = self._single_crystal_map.interpolate_triangulation(interpolate)
        self._data.triang = triangulator_refiner
        self._data.z_triang = z_refiner
        # this is important to get the limits
        x, y, z = getattr(self._single_crystal_map, mesh_name)
        self._data.x = x
        self._data.y = y
        self._data.z = z
        return triangulator_refiner, z_refiner

    def get_axis_labels(self, axis_type, crystal_axes, switch=False):
        hkl1 = self._single_crystal_map.hkl1
        hkl2 = self._single_crystal_map.hkl2
        axis_labels = {"hkl": [f"[{hkl1}] (r.l.u.)", f"[{hkl2}] (r.l.u.)"]}
        labels = axis_labels[axis_type]
        return labels
