# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from uuid import uuid4

import numpy as np
from scipy.spatial.transform import Rotation

from mantid.simpleapi import (
    CloneWorkspace,
    CopySample,
    CreateSimulationWorkspace,
    DeleteLog,
    LoadSampleShape,
    RotateSampleShape,
    SetSampleMaterial,
    SetSampleShape,
    TranslateSampleShape,
)
from mantid.api import AnalysisDataService as ADS
from Engineering.common.xml_shapes import get_cube_xml
from Engineering.texture.texture_helper import define_gauge_volume, get_gauge_vol_str, get_scattering_centre


class WorkspaceManager:
    """Owns the workspaces (data, ungrouped, mesh, mesh-neutral, instrument)
    and the operations that mutate sample shape / material / gauge volume on
    them. Reads instrument name from the parent model via a back-reference.
    """

    # Workspace names used by the planner. All start with "__" so the ADS treats them as hidden.
    # Each holds a distinct view of the sample needed by a specific consumer:
    #   WS_DATA          live workspace driving plots/exports: holds the translated shape with init_R
    #                    baked into its XML goniometer tag, and carries the current orientation R on
    #                    its run goniometer. Read by get_scattering_centre, GroupDetectors, etc.
    #   WS_UNGROUPED     pre-group clone of WS_DATA; detector_geometry re-runs GroupDetectors against
    #                    this whenever the group pattern changes (can't ungroup an already-grouped ws).
    #   WS_MESH_RAW      pristine sample (no init_R, no translation, identity goniometer). Source of
    #                    truth for absorption (which composes R*init_R itself) and for rebuilding
    #                    WS_DATA in update_initial_shape.
    #   WS_MESH_NEUTRAL  init_R baked in, no translation, identity goniometer. Used by the plotter
    #                    (which applies R itself to the mesh vertices) and by the reference-ws export.
    #   WS_REFERENCE     transient name for output_as_reference_workspace; created and removed inside
    #                    the export call, never persists across calls.
    #   WS_MC_INPUT      per-orientation MonteCarloAbsorption input. Rebuilt each calc_for_index with
    #                    R*init_R baked into the shape and identity goniometer.
    #   WS_MC_OUTPUT     MonteCarloAbsorption output (transmission factors).
    #   WS_TMP           transient name for update_initial_shape's working copy; removed in finally.
    #   WS_MATERIAL      dedicated holder for the ground-truth sample material. Its shape is never
    #                    reloaded, so it survives shape changes and lets set_material re-apply the
    #                    full material (formula + number/mass density) via CopySample rather than a
    #                    lossy formula-only round-trip through SetSampleMaterial.
    WS_DATA = "__texture_planning_ws"
    WS_UNGROUPED = "__texture_planning_ws_ungrouped"
    WS_MESH_RAW = "__texture_planning_raw_sample_mesh"
    WS_MESH_NEUTRAL = "__texture_planning_neutral_sample_mesh"
    WS_MATERIAL = "__texture_planning_material"
    WS_REFERENCE = "__texture_planning_reference_ws"
    WS_MC_INPUT = "__mc_ws"
    WS_MC_OUTPUT = "__abs_ws"
    WS_TMP = "__tmp_ws"
    _SHAPE_TMP = "__shape_ws"

    # The class constants above are base names. Each instance shadows them with per-instance copies
    # carrying a unique suffix (see __init__) so that several planner windows open at once don't
    # collide on the ADS. Listed here once so __init__ can suffix them and cleanup() can remove them.
    _OWNED_WS_NAME_ATTRS = (
        "WS_DATA",
        "WS_UNGROUPED",
        "WS_MESH_RAW",
        "WS_MESH_NEUTRAL",
        "WS_MATERIAL",
        "WS_REFERENCE",
        "WS_MC_INPUT",
        "WS_MC_OUTPUT",
        "WS_TMP",
        "_SHAPE_TMP",
    )

    DEFAULT_MATERIAL = "Fe"

    def __init__(self, model):
        self._model = model
        # Several planner windows can be open simultaneously, so suffix every workspace name with a
        # token unique to this instance. All consumers read these names off the instance (e.g.
        # self.workspaces.WS_MC_INPUT, wsm.wsname), so shadowing the class constants here is
        # transparent to them - no consumer needs to know about the suffix.
        self._suffix = f"_{uuid4().hex[:8]}"
        for attr in self._OWNED_WS_NAME_ATTRS:
            setattr(self, attr, getattr(self, attr) + self._suffix)
        self.wsname = self.WS_DATA
        self.ungrouped_wsname = self.WS_UNGROUPED
        self.ws = None  # holds the translated shape with init_R
        # baked in and current orientation R on its run goniometer
        self.ungrouped_ws = None  # ungrouped clone of self.ws for changing groups quickly
        self.mesh_ws = None  # pristine sample (no init_R, no translation, identity goniometer)
        # reference for absorption and for rebuilding WS_DATA when initial shape is updated.
        self.updated_mesh_ws = None  # sample with init_R baked in, no translation, identity goniometer
        # Used by the plotter and by the reference-ws export.
        self.material_ws = None  # ground-truth material holder; never has its shape reloaded
        self.init_R = Rotation.identity()
        self.offset = (0, 0, 0)
        self.gauge_volume_str = None
        # stl_settings
        self.stl_kwargs = {"Scale": "cm", "XDegrees": 0, "YDegrees": 0, "ZDegrees": "0", "TranslationVector": "0,0,0"}
        # attenuation-point settings read by AbsorptionCalculator. The material itself lives on
        # material_ws (the ground truth), not here.
        self.attenuation_kwargs = {"point": 1.5, "unit": "dSpacing"}

    def cleanup(self):
        """Remove every workspace this instance owns from the ADS. Called when the planner window
        closes so the (hidden) workspaces don't accumulate across open/close cycles now that their
        names are unique per instance."""
        for attr in self._OWNED_WS_NAME_ATTRS:
            wsname = getattr(self, attr)
            if ADS.doesExist(wsname):
                ADS.remove(wsname)

    @property
    def instr(self):
        return self._model.instrument.get_instrument()

    @property
    def scattering_centre(self):
        return get_scattering_centre(self.ws)

    def update_ws(self):
        if self.ws:
            # rebuilding for a new instrument: _update_existing_wss copies the sample (including
            # whatever material the user set via the SetSampleMaterial dialog) onto the new
            # workspaces, so we must not reset the material here
            self._update_existing_wss()
        else:
            # brand-new workspaces have no material; seed the default so absorption works out of the box
            self._init_wss()
            self.set_material()
        self.ungrouped_ws = CloneWorkspace(InputWorkspace=self.ws, OutputWorkspace=self.ungrouped_wsname)
        if self.gauge_volume_str:
            define_gauge_volume(self.ws, self.gauge_volume_str)

    def _update_existing_wss(self):
        # ws and updated_mesh_ws carry the user's initial shape rotation (init_R); it must survive the
        # instrument switch, but a plain CopySample drops it for a CSG shape (see
        # copy_sample_preserving_initial_rotation), so preserve it explicitly. mesh_ws is the pristine,
        # un-rotated sample and is copied as-is.
        ws = self._create_new_ws_with_copied_sample(self.wsname, self.ws, clone=True, preserve_initial_rotation=True)
        mesh_ws = self._create_new_ws_with_copied_sample(self.WS_MESH_RAW, self.mesh_ws, clone=True)
        updated_mesh_ws = self._create_new_ws_with_copied_sample(
            self.WS_MESH_NEUTRAL, self.updated_mesh_ws, clone=True, preserve_initial_rotation=True
        )
        self.ws = ws
        self.mesh_ws = mesh_ws
        self.updated_mesh_ws = updated_mesh_ws

    def _init_wss(self):
        ws = CreateSimulationWorkspace(Instrument=self.instr, BinParams="0,0.1,5", OutputWorkspace=self.wsname, UnitX="dSpacing")
        for ispec in range(ws.getNumberHistograms()):
            ws.setY(ispec, np.ones_like(ws.readY(ispec)))
        mesh_ws = CloneWorkspace(InputWorkspace=ws, OutputWorkspace=self.WS_MESH_RAW)
        updated_mesh_ws = CloneWorkspace(InputWorkspace=ws, OutputWorkspace=self.WS_MESH_NEUTRAL)
        self.ws = ws
        self.mesh_ws = mesh_ws
        self.updated_mesh_ws = updated_mesh_ws
        self._set_default_shape_on_wss()
        # ground-truth material holder, seeded with the default. Cloned after the default shape is
        # set so it owns a shape to hang the material on. set_material copies from here, so the
        # default is applied to the other wss in the brand-new branch of update_ws.
        self.material_ws = CloneWorkspace(InputWorkspace=self.mesh_ws, OutputWorkspace=self.WS_MATERIAL)
        SetSampleMaterial(self.material_ws, self.DEFAULT_MATERIAL)

    def _set_default_shape_on_wss(self):
        SetSampleShape(self.ws, get_cube_xml("default_cube", 0.01))
        SetSampleShape(self.mesh_ws, get_cube_xml("default_cube", 0.01))
        SetSampleShape(self.updated_mesh_ws, get_cube_xml("default_cube", 0.01))

    @staticmethod
    def _copy_material(source, target):
        # copy only the material (not shape/orientation/etc.), preserving its full definition
        # including the number/mass density
        CopySample(
            InputWorkspace=source,
            OutputWorkspace=target,
            CopyName=False,
            CopyMaterial=True,
            CopyShape=False,
            CopyEnvironment=False,
            CopyLattice=False,
        )

    def set_material(self):
        """Apply the ground-truth material (material_ws) onto the shape workspaces"""
        targets = [self.ws, self.mesh_ws, self.updated_mesh_ws]
        if self.ungrouped_ws is not None:
            targets.append(self.ungrouped_ws)
        for target in targets:
            self._copy_material(self.material_ws, target)

    def get_material_name(self):
        """Chemical formula / name of the ground-truth material. Returns "" when no material is set."""
        try:
            return self.material_ws.sample().getMaterial().name()
        except (AttributeError, RuntimeError):
            return ""

    def propagate_material(self):
        """Share the material the user just set via the SetSampleMaterial dialog (which only writes to
        WS_MESH_RAW) with the other workspaces. The raw mesh ws becomes the new ground truth, so it is
        captured onto material_ws as well, ensuring later shape reloads re-apply this exact material."""
        self._copy_material(self.mesh_ws, self.material_ws)
        targets = [self.ws, self.updated_mesh_ws]
        if self.ungrouped_ws is not None:
            targets.append(self.ungrouped_ws)
        for target in targets:
            self._copy_material(self.mesh_ws, target)

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
            # CopySample bakes the destination workspace's current goniometer R into the new
            # shape's XML (see CopySample::copyParameters -> addGoniometerTag), and the plotter
            # leaves a non-identity R on self.ws on every redraw.
            #
            # Reset to identity *before* the CopySamples so the shape we hand on carries only init_R
            self.ws.run().getGoniometer().setR(np.eye(3))
            CopySample(InputWorkspace=_tmp_ws, OutputWorkspace=self.wsname, CopyName=False, CopyEnvironment=False, CopyLattice=False)
            CopySample(
                InputWorkspace=_tmp_ws, OutputWorkspace=self.updated_mesh_ws, CopyName=False, CopyEnvironment=False, CopyLattice=False
            )

            if self._all_rots_zero(x_rot, y_rot, z_rot):
                self.init_R = Rotation.identity()

            else:
                self.init_R = Rotation.from_euler("xyz", (x_rot, y_rot, z_rot), degrees=True)
                self.rotate_samples_by_initial_goniometer()
        finally:
            ADS.remove(self.WS_TMP)

    @staticmethod
    def _all_rots_zero(x_rot, y_rot, z_rot):
        return np.isclose(x_rot, 0) and np.isclose(y_rot, 0) and np.isclose(z_rot, 0)

    def rotate_samples_by_initial_goniometer(self):
        rot_vec = self.init_R.as_rotvec(degrees=True)
        ang = np.linalg.norm(rot_vec)
        if ang == 0:
            return None
        vec = rot_vec / ang

        RotateSampleShape(self.wsname, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")
        RotateSampleShape(self.updated_mesh_ws, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")

    def set_gauge_volume_str(self, preset, custom):
        self.gauge_volume_str = get_gauge_vol_str(preset, custom)
        if self.gauge_volume_str:
            define_gauge_volume(self.ws, self.gauge_volume_str)
        elif self.ws.run().hasProperty("GaugeVolume"):
            DeleteLog(Workspace=self.ws, Name="GaugeVolume")

    def _create_new_ws_with_copied_sample(self, new_wsname, sample_to_copy, clone=False, preserve_initial_rotation=False):
        if clone:
            shape_ws = CloneWorkspace(InputWorkspace=sample_to_copy, OutputWorkspace=self._SHAPE_TMP)
        else:
            shape_ws = sample_to_copy
        try:
            new_ws = CreateSimulationWorkspace(Instrument=self.instr, BinParams="0,0.1,5", OutputWorkspace=new_wsname, UnitX="dSpacing")
            if preserve_initial_rotation:
                self.copy_sample_preserving_initial_rotation(shape_ws, new_ws)
            else:
                CopySample(InputWorkspace=shape_ws, OutputWorkspace=new_wsname, CopyName=False, CopyEnvironment=False, CopyLattice=False)
        finally:
            if clone and ADS.doesExist(self._SHAPE_TMP):
                ADS.remove(self._SHAPE_TMP)
        return new_ws

    @staticmethod
    def _shape_is_mesh(ws):
        # a loaded STL becomes a MeshObject; the default cube and a loaded CSG xml are CSGObjects.
        return type(ws.sample().getShape()).__name__ == "MeshObject"

    def copy_sample_preserving_initial_rotation(self, source_ws, dest_ws):
        """CopySample source_ws's sample onto dest_ws while keeping source_ws's baked-in initial
        shape rotation (init_R).

        CopySample re-bakes the *destination* workspace's run goniometer into the copied shape: for a
        CSG shape it overwrites the <goniometer> tag that holds init_R (so a copy into an
        identity-goniometer workspace silently strips init_R), while for a MeshObject it rotates the
        vertices that already hold init_R. So the destination must carry init_R for a CSG shape, and
        stay at identity for a mesh (otherwise init_R would be applied a second time). The destination
        goniometer is only a vehicle for re-baking init_R into the shape, so it is restored to
        identity afterwards - init_R must live in the shape, never in the run goniometer."""
        gonio_R = np.eye(3) if self._shape_is_mesh(source_ws) else self.init_R.as_matrix()
        dest_ws.run().getGoniometer().setR(gonio_R)
        CopySample(InputWorkspace=source_ws, OutputWorkspace=dest_ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)
        dest_ws.run().getGoniometer().setR(np.eye(3))
