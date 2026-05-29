# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from scipy.spatial.transform import Rotation

from mantid.simpleapi import (
    CloneWorkspace,
    CopySample,
    CreateSimulationWorkspace,
    DeleteLog,
    LoadEmptyInstrument,
    LoadSampleShape,
    RotateSampleShape,
    SetSampleMaterial,
    SetSampleShape,
    TranslateSampleShape,
)
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import logger
from Engineering.common.xml_shapes import get_cube_xml
from Engineering.texture.texture_helper import define_gauge_volume, get_gauge_vol_str, get_scattering_centre


class WorkspaceManager:
    """Owns the workspaces (data, ungrouped, mesh, mesh-neutral, instrument)
    and the operations that mutate sample shape / material / gauge volume on
    them. Reads instrument name from the parent model via a back-reference.
    """

    # Workspace names used by the planner. All start with "__" so the ADS treats them as hidden.
    WS_DATA = "__texture_planning_ws"
    WS_INSTR = "__texture_planning_instr"
    WS_UNGROUPED = "__texture_planning_ws_ungrouped"
    WS_MESH_RAW = "__texture_planning_raw_sample_mesh"
    WS_MESH_NEUTRAL = "__texture_planning_neutral_sample_mesh"
    WS_REFERENCE = "__texture_planning_reference_ws"
    WS_MC_INPUT = "__mc_ws"
    WS_MC_OUTPUT = "__abs_ws"
    WS_TMP = "__tmp_ws"

    def __init__(self, model):
        self._model = model
        self.wsname = self.WS_DATA
        self.instr_wsname = self.WS_INSTR
        self.ungrouped_wsname = self.WS_UNGROUPED
        self.ws = None
        self.ungrouped_ws = None
        self.mesh_ws = None
        self.updated_mesh_ws = None
        self.instr_ws = None
        self.init_R = Rotation.identity()
        self.offset = (0, 0, 0)
        self.gauge_volume_str = None
        # stl_settings
        self.stl_kwargs = {"Scale": "cm", "XDegrees": 0, "YDegrees": 0, "ZDegrees": "0", "TranslationVector": "0,0,0"}
        # material + attenuation-point settings; the material is applied here, the
        # point/unit are read by AbsorptionCalculator
        self.attenuation_kwargs = {"point": 1.5, "unit": "dSpacing", "material": "Fe"}

    @property
    def instr(self):
        return self._model.instr

    @property
    def scattering_centre(self):
        return get_scattering_centre(self.ws)

    def update_ws(self):
        self.instr_ws = LoadEmptyInstrument(InstrumentName=self.instr, OutputWorkspace=self.instr_wsname)
        if self.ws:
            ws = self._create_new_ws_with_copied_sample(self.wsname, self.ws, clone=True)
            mesh_ws = self._create_new_ws_with_copied_sample(self.WS_MESH_RAW, self.mesh_ws, clone=True)
            updated_mesh_ws = self._create_new_ws_with_copied_sample(self.WS_MESH_NEUTRAL, self.updated_mesh_ws, clone=True)
        else:
            ws = CreateSimulationWorkspace(Instrument=self.instr, BinParams="0,0.1,5", OutputWorkspace=self.wsname, UnitX="dSpacing")
            for ispec in range(ws.getNumberHistograms()):
                ws.setY(ispec, np.ones_like(ws.readY(ispec)))
            mesh_ws = CloneWorkspace(InputWorkspace=ws, OutputWorkspace=self.WS_MESH_RAW)
            updated_mesh_ws = CloneWorkspace(InputWorkspace=ws, OutputWorkspace=self.WS_MESH_NEUTRAL)
            SetSampleShape(ws, get_cube_xml("default_cube", 0.01))
            SetSampleShape(mesh_ws, get_cube_xml("default_cube", 0.01))
            SetSampleShape(updated_mesh_ws, get_cube_xml("default_cube", 0.01))
        self.ws = ws
        self.mesh_ws = mesh_ws
        self.updated_mesh_ws = updated_mesh_ws
        self.ungrouped_ws = CloneWorkspace(InputWorkspace=self.ws, OutputWorkspace=self.ungrouped_wsname)
        self.set_material()
        if self.gauge_volume_str:
            define_gauge_volume(self.ws, self.gauge_volume_str)

    def set_material(self):
        SetSampleMaterial(self.ws, self.attenuation_kwargs["material"])
        SetSampleMaterial(self.mesh_ws, self.attenuation_kwargs["material"])
        SetSampleMaterial(self.updated_mesh_ws, self.attenuation_kwargs["material"])
        if self.ungrouped_ws is not None:
            SetSampleMaterial(self.ungrouped_ws, self.attenuation_kwargs["material"])

    def set_material_string(self, material):
        prev_material = self.attenuation_kwargs["material"]
        try:
            self.attenuation_kwargs["material"] = material
            self.set_material()
        except Exception:
            logger.error("Invalid Material 'ChemicalFormula' - see SetSampleMaterial docs for more info")
            self.attenuation_kwargs["material"] = prev_material
            self.ws = ADS.retrieve(self.wsname)
            self.ungrouped_ws = ADS.retrieve(self.ungrouped_wsname)
            self.mesh_ws = ADS.retrieve(self.WS_MESH_RAW)
            self.updated_mesh_ws = ADS.retrieve(self.WS_MESH_NEUTRAL)
            try:
                self.set_material()
            except Exception:
                pass

    def load_stl(self, stl_file):
        LoadSampleShape(InputWorkspace=self.ws, Filename=stl_file, OutputWorkspace=self.ws, **self.stl_kwargs)
        LoadSampleShape(InputWorkspace=self.mesh_ws, Filename=stl_file, OutputWorkspace=self.mesh_ws, **self.stl_kwargs)
        LoadSampleShape(InputWorkspace=self.updated_mesh_ws, Filename=stl_file, OutputWorkspace=self.updated_mesh_ws, **self.stl_kwargs)
        self.set_material()

    def load_xml(self, xml_file):
        with open(xml_file, "r") as f:
            xml_string = f.read()
        SetSampleShape(self.ws, xml_string)
        SetSampleShape(self.mesh_ws, xml_string)
        SetSampleShape(self.updated_mesh_ws, xml_string)
        self.set_material()

    @staticmethod
    def translate_shape(ws, x_pos, y_pos, z_pos):
        if not (x_pos == 0.0 and y_pos == 0.0 and z_pos == 0.0):
            TranslateSampleShape(InputWorkspace=ws, TranslationVector=f"{x_pos},{y_pos},{z_pos}")

    def update_initial_shape(self, x_rot, y_rot, z_rot, x_pos, y_pos, z_pos):
        _tmp_ws = self._create_new_ws_with_copied_sample(self.WS_TMP, self.mesh_ws)
        self.offset = (x_pos, y_pos, z_pos)
        self.translate_shape(_tmp_ws, *self.offset)

        try:
            CopySample(InputWorkspace=_tmp_ws, OutputWorkspace=self.wsname, CopyName=False, CopyEnvironment=False, CopyLattice=False)
            CopySample(
                InputWorkspace=_tmp_ws, OutputWorkspace=self.updated_mesh_ws, CopyName=False, CopyEnvironment=False, CopyLattice=False
            )

            if x_rot == 0.0 and y_rot == 0.0 and z_rot == 0.0:
                self.init_R = Rotation.identity()
                return None

            self.init_R = Rotation.from_euler("xyz", (x_rot, y_rot, z_rot), degrees=True)
            rotvec = self.init_R.as_rotvec(degrees=True)
            ang = np.linalg.norm(rotvec)
            if ang == 0:
                return None
            vec = rotvec / ang

            # RotateSampleShape folds the workspace's current goniometer R into the new shape's
            # XML (newSampleShapeRot = init_R * oldRotation). The plotter sets a non-identity R
            # on self.ws on every redraw, so without this reset we would silently bake the current
            # orientation into the sample shape and self.ws / self.updated_mesh_ws would diverge.
            self.ws.run().getGoniometer().setR(np.eye(3))
            RotateSampleShape(self.wsname, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")
            RotateSampleShape(self.updated_mesh_ws, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")
        finally:
            ADS.remove(self.WS_TMP)

    def set_gauge_volume_str(self, preset, custom):
        self.gauge_volume_str = get_gauge_vol_str(preset, custom)
        if self.gauge_volume_str:
            define_gauge_volume(self.ws, self.gauge_volume_str)
        elif self.ws.run().hasProperty("GaugeVolume"):
            DeleteLog(Workspace=self.ws, Name="GaugeVolume")

    def _create_new_ws_with_copied_sample(self, new_wsname, sample_to_copy, clone=False):
        if clone:
            shape_ws = CloneWorkspace(InputWorkspace=sample_to_copy, OutputWorkspace="__shape_ws")
        else:
            shape_ws = sample_to_copy
        new_ws = CreateSimulationWorkspace(Instrument=self.instr, BinParams="0,0.1,5", OutputWorkspace=new_wsname, UnitX="dSpacing")
        CopySample(InputWorkspace=shape_ws, OutputWorkspace=new_wsname, CopyName=False, CopyEnvironment=False, CopyLattice=False)
        return new_ws
