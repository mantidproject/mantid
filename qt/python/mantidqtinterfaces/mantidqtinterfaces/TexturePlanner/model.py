# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from Engineering.common.calibration_info import CalibrationInfo
from Engineering.common.instrument_config import get_instr_config, SUPPORTED_INSTRUMENTS
from Engineering.texture.texture_helper import project_orientation, vec_string_to_norm_array

from mantidqtinterfaces.TexturePlanner.absorption import AbsorptionCalculator
from mantidqtinterfaces.TexturePlanner.detector_geometry import DetectorGeometry
from mantidqtinterfaces.TexturePlanner.exporter import OrientationExporter
from mantidqtinterfaces.TexturePlanner.orientation_table import OrientationTable
from mantidqtinterfaces.TexturePlanner.plotter import TexturePlotter
from mantidqtinterfaces.TexturePlanner.workspace_manager import WorkspaceManager


class TexturePlannerModel(object):
    """Thin orchestrator wiring together the workspace, orientation, geometry,
    absorption, plotter, and exporter collaborators. Owns cross-cutting
    settings (projection, ax_transform, dir_names, mc_kwargs, vis_settings,
    direction colours, sense maps).
    """

    # Per-instrument detector grouping presets. Add a new instrument by adding a row here.
    _SUPPORTED_GROUPS_BY_INSTRUMENT = {
        "ENGINX": ("Texture20", "Texture30", "banks"),
        "IMAT": ("Module1", "Module4", "Row1", "Row4", "banks"),
    }
    _DEFAULT_SUPPORTED_GROUPS = ("banks",)

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
        self.plot_attenuation = False

        # euler_file_settings (used by orientation loader + exporter)
        self.orientation_kwargs = {"Axes": "YXY", "Senses": "-1,-1,-1"}

        # MonteCarloAbsorption settings (filled in once workspaces collaborator exists)
        self.mc_kwargs = None

        # instrument config
        self.instr = instrument
        self.calib_info = None
        self.config = None
        self.group = None
        self.supported_groups = ("Texture20", "Texture30", "banks")

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

        # init func calls
        self.update_instrument(self.instr)

    # instrument config -------------------------------------------------
    def update_instrument(self, instrument):
        self.instr = instrument
        self.config = get_instr_config(self.instr)
        self.workspaces.update_ws()
        self.update_calib_info()
        self.update_supported_groups()

    @staticmethod
    def get_supported_instruments():
        return SUPPORTED_INSTRUMENTS

    def set_group(self, group_str):
        self.group = self.config.group(group_str)
        self.update_calib_info()

    def update_calib_info(self):
        self.calib_info = CalibrationInfo(instrument=self.instr, group=self.group)

    def update_supported_groups(self):
        self.supported_groups = self._SUPPORTED_GROUPS_BY_INSTRUMENT.get(self.instr, self._DEFAULT_SUPPORTED_GROUPS)

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

    def set_plot_attenuation(self, val):
        self.plot_attenuation = val

    # projection orchestration -----------------------------------------
    def update_all_projected_data(self):
        for i in self.orientations.keys():
            self.update_projected_data(i)

    def update_projected_data(self, index):
        orientation = self.orientations[index]
        orientation.pf_points = project_orientation(orientation.R, self.geometry.detQs_lab, self.ax_transform, self.projection)
        if self.plot_attenuation:
            self.absorption.calc_for_index(index)
