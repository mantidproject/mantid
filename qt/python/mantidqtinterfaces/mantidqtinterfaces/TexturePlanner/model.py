# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from Engineering.texture.texture_helper import project_orientation, vec_string_to_norm_array

from mantidqtinterfaces.TexturePlanner.helpers.absorption import AbsorptionCalculator
from mantidqtinterfaces.TexturePlanner.helpers.detector_geometry import DetectorGeometry
from mantidqtinterfaces.TexturePlanner.helpers.exporter import OrientationExporter
from mantidqtinterfaces.TexturePlanner.helpers.instrument import InstrumentHelper
from mantidqtinterfaces.TexturePlanner.helpers.orientation_table import OrientationTable
from mantidqtinterfaces.TexturePlanner.helpers.plotter import TexturePlotter
from mantidqtinterfaces.TexturePlanner.helpers.workspace_manager import WorkspaceManager


class TexturePlannerModel(object):
    """Thin wrapper class mainly holding setting state and few cross-cutting methods and acting as a bridge between
    functionality contained in separate collaborators classes.
    """

    def __init__(self, instrument="ENGINX", projection="azimuthal"):
        # cross-cutting visual / display settings
        self.gon_colors = ("hotpink", "orange", "purple", "goldenrod", "plum", "saddlebrown")
        self.dir_cols = ("red", "green", "blue")
        self.ax_transform = np.eye(3)
        self.dir_names = ["D1", "D2", "D3"]
        self.projection = projection
        self.vis_settings = {"directions": True, "goniometers": True, "incident": True, "ks": True, "scattered": False}

        # currently-selected goniometer (controls which axis is highlighted in the plot)
        self.gonio_index = 0

        # output / plot toggles
        self.n_output_points = 1
        self.plot_transmission = False

        # euler_file_settings (used by orientation loader + exporter)
        self.orientation_kwargs = {"Axes": "YXY", "Senses": "-1,-1,-1"}

        # MonteCarloAbsorption settings (filled in once workspaces collaborator exists)
        self.mc_kwargs = None

        # collaborators
        self.workspaces = WorkspaceManager(self)
        self.mc_kwargs = {
            "InputWorkspace": self.workspaces.WS_MC_INPUT,
            "OutputWorkspace": self.workspaces.WS_MC_OUTPUT,
            "EventsPerPoint": 50,
            "MaxScatterPtAttempts": int(1e4),
            "SimulateScatteringPointIn": "SampleOnly",
            "ResimulateTracksForDifferentWavelengths": False,
        }
        self.orientations = OrientationTable(self)
        self.geometry = DetectorGeometry(self)
        self.absorption = AbsorptionCalculator(self)
        self.exporter = OrientationExporter(self)
        self.plotter = TexturePlotter(self)
        self.instrument = InstrumentHelper(self, instrument)
        # bootstrap instrument config now the helper is bound to the model (update_ws reads
        # the instrument name back through self.instrument, so it must be assigned first)
        self.instrument.update_instrument(instrument)

    @staticmethod
    def get_default_texture_directions():
        return ("RD", "ND", "TD"), ((1, 0, 0), (0, 1, 0), (0, 0, 1))

    # cross-cutting settings setters -----------------------------------
    def set_ax_transform(self, vec1, vec2, vec3):
        vec1, vec2, vec3 = vec_string_to_norm_array(vec1), vec_string_to_norm_array(vec2), vec_string_to_norm_array(vec3)
        self.ax_transform = np.concatenate((vec1[:, None], vec2[:, None], vec3[:, None]), axis=1)

    def set_dir_names(self, name1, name2, name3):
        self.dir_names = [name1, name2, name3]

    def set_gonio_index(self, index):
        self.gonio_index = index

    def update_gonio_index(self, num_gonios):
        max_ind = num_gonios - 1
        return min(self.gonio_index, max_ind)

    def set_plot_transmission(self, val):
        self.plot_transmission = val

    # projection orchestration -----------------------------------------
    def update_all_projected_data(self):
        for i in self.orientations.keys():
            self.update_projected_data(i)

    def update_projected_data(self, index):
        orientation = self.orientations[index]
        orientation.pf_points = project_orientation(orientation.R, self.geometry.detQs_lab, self.ax_transform, self.projection)
        if self.plot_transmission:
            self.absorption.calc_for_index(index)
