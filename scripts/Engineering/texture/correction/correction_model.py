from mantid.simpleapi import (
    LoadSampleShape,
    SetSample,
    CopySample,
    SetSampleMaterial,
    DefineGaugeVolume,
    ConvertUnits,
    Scale,
    MonteCarloAbsorption,
    EstimateDivergence,
    CloneWorkspace,
    SaveNexus,
    SetGoniometer,
    logger,
    CreateEmptyTableWorkspace,
    LoadEmptyInstrument,
    CreateSampleWorkspace,
    CreateSampleShape,
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from os import path, makedirs
from scipy import interpolate
from Engineering.EnggUtils import GROUP
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


class TextureCorrectionModel:
    def __init__(self):
        self.reference_ws = None

    def load_all_orientations(self, wss, txt_file, use_euler, euler_scheme, euler_sense):
        if self._validate_file(txt_file, ".txt"):
            with open(txt_file, "r") as f:
                goniometer_strings = [line.replace("\t", ",") for line in f.readlines()]
                goniometer_lists = [[float(x) for x in gs.split(",")] for gs in goniometer_strings]
            try:
                if not use_euler:
                    # if use euler angles not selected then assumes it is a scans output matrix
                    for iws, ws in enumerate(wss):
                        # NewSetGoniometer(ws, transformation_str=goniometer_strings[iws])
                        SetGoniometer(ws, GoniometerMatrix=goniometer_lists[iws][:9])
                else:
                    axis_dict = {"x": "1,0,0", "y": "0,1,0", "z": "0,0,1"}
                    rotation_sense = [int(x) for x in euler_sense.split(",")]
                    for iws, ws in enumerate(wss):
                        angles = goniometer_strings[iws].split(",")
                        kwargs = {}
                        for iang, angle in enumerate(angles):
                            sense = rotation_sense[iang]
                            kwargs[f"Axis{iang}"] = f"{angle},{axis_dict[(euler_scheme[iang]).lower()]},{sense}"
                        SetGoniometer(ws, **kwargs)
            except BaseException as e:
                logger.error(
                    f"{str(e)}. Failed to set goniometer, are your settings for `use_euler_angles` correct? Currently: {use_euler}"
                )

    def set_sample_info(self, ws, shape, material):
        if shape.endswith(".stl"):
            LoadSampleShape(InputWorkspace=ws, OutputWorkspace=ws, Filename=shape)
        else:
            SetSample(ws, Geometry={"Shape": "CSG", "Value": shape})
        SetSampleMaterial(ws, material)

    def copy_sample_info(self, ref_ws, wss, is_ref=False):
        # currently copy shape bakes the orientation matrix into the sample shape
        if not is_ref:
            # need to create a ws with an unorientated sample to copy over
            ws = ADS.retrieve(ref_ws)
            trans_mat = ws.getRun().getGoniometer().getR()

            _tmp_ws = CloneWorkspace(ws, OutputWorkspace="_tmp_ws")
            _tmp_ws.getRun().getGoniometer().setR(np.linalg.inv(trans_mat))

            CopySample(InputWorkspace=ws, OutputWorkspace=_tmp_ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)

            for ws in wss:
                CopySample(InputWorkspace=_tmp_ws, OutputWorkspace=ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)

            # remove the tmp ws
            ADS.remove("_tmp_ws")
        else:
            # if is_ref flag then we want to take this baked orientation forward
            for ws in wss:
                CopySample(InputWorkspace=ref_ws, OutputWorkspace=ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)

    def get_ws_info(self, ws_name, select=True):
        return {
            "shape": "Not set" if self._has_no_valid_shape(ws_name) else "set",
            "material": "Not set" if self._has_no_valid_material(ws_name) else self._get_material_name(ws_name),
            "orient": "default" if self._has_no_orientation_set(ws_name) else "set",
            "select": select,
        }

    def _has_no_orientation_set(self, ws_name):
        try:
            return np.all(ADS.retrieve(ws_name).run().getGoniometer().getR() == np.identity(3))
        except RuntimeError:
            return True

    def _is_valid_mesh(self, mesh):
        return len(mesh) > 3

    def _has_no_valid_shape(self, ws_name):
        no_shape = False
        try:
            no_shape = not self._is_valid_mesh(ADS.retrieve(ws_name).sample().getShape().getMesh())
        except RuntimeError:
            no_shape = True
        return no_shape

    def _get_material_name(self, ws_name):
        return ADS.retrieve(ws_name).sample().getMaterial().name()

    def _has_no_valid_material(self, ws_name):
        return self._get_material_name(ws_name) == ""

    def define_gauge_volume(self, ws, preset, custom):
        gauge_str = self.get_gauge_vol_str(preset, custom)
        if gauge_str:
            DefineGaugeVolume(ws, gauge_str)

    def get_gauge_vol_str(self, preset, custom):
        if preset == "4mmCube":
            gauge_str = """
        <cuboid id='some-gv'> \
        <height val='0.04'  /> \
        <width val='0.04' />  \
        <depth  val='0.04' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-gv' /> \\ """
        else:
            try:
                gauge_str = self._read_xml(custom)
            except RuntimeError:
                gauge_str = None
        return gauge_str

    def get_xml_mesh(self, xml):
        tmp_ws = CreateSampleWorkspace()
        CreateSampleShape(tmp_ws, xml)
        mesh = tmp_ws.sample().getShape().getMesh()
        ADS.remove("tmp_ws")
        return mesh

    def _read_xml(self, file):
        out = None
        if self._validate_file(file, ".xml"):
            with open(file, "r") as f:
                out = f.read()
        return out

    def _validate_file(self, file, ext):
        valid = False
        if file:
            root, f_ext = path.splitext(file)
            if f_ext == ext:
                valid = True
        return valid

    def calc_absorption(self, ws, mc_param_str):
        method_dict = self._param_str_to_dict(mc_param_str)
        temp_ws = ConvertUnits(ws, Target="Wavelength")
        Scale(InputWorkspace=temp_ws, OutputWorkspace=temp_ws, Factor=1, Operation="Add")
        method_dict["OutputWorkspace"] = "_abs_corr"
        MonteCarloAbsorption(temp_ws, **method_dict)

    def _param_str_to_dict(self, param_string):
        out_dict = {}
        for params in param_string.split(","):
            k, v = params.split(":")
            out_dict[k] = self._parse_param_values(v)
        return out_dict

    def _parse_param_values(self, string):
        l_string = string.lower()
        if l_string == "true":
            return True
        elif l_string == "false":
            return False
        elif string.isnumeric():
            return int(string)
        else:
            return string

    def calc_divergence(self, ws, horz, vert, det_horz):
        EstimateDivergence(ws, vert, horz, det_horz, OutputWorkspace="_div_corr")

    def apply_corrections(
        self, ws, out_ws, calibration_group, root_dir, abs_corr=1.0, div_corr=1.0, rb_num=None, remove_ws_after_processing=False
    ):
        ws = ADS.retrieve(ws)
        temp_ws = ConvertUnits(ws, Target="dSpacing")
        if isinstance(abs_corr, str):
            abs_ws = ConvertUnits(ADS.retrieve(abs_corr), Target="dSpacing")
            temp_ws = temp_ws / abs_ws
            ADS.remove("abs_ws")
        else:
            temp_ws = temp_ws / abs_corr
        if isinstance(div_corr, str):
            div_ws = ConvertUnits(ADS.retrieve(div_corr), Target="dSpacing")
            temp_ws = temp_ws / div_ws
            ADS.remove("div_ws")
        else:
            temp_ws = temp_ws / div_corr
        CloneWorkspace(temp_ws, OutputWorkspace=out_ws)
        self._save_corrected_files(out_ws, root_dir, "AbsorptionCorrection", rb_num, calibration_group)
        if remove_ws_after_processing:
            # remove output ws from ADS to free up memory
            ADS.remove(out_ws)
            ADS.remove("temp_ws")
            if isinstance(abs_corr, str):
                ADS.remove(abs_corr)
            if isinstance(div_corr, str):
                ADS.remove(div_corr)

    def _save_corrected_files(self, ws, root_dir, dir_name, rb_num, calibration_group):
        save_dirs = [path.join(root_dir, dir_name)]
        if rb_num:
            save_dirs.append(path.join(root_dir, "User", rb_num, dir_name))
            if calibration_group == GROUP.TEXTURE20 or calibration_group == GROUP.TEXTURE30:
                save_dirs.pop(0)  # only save to RB directory to limit number files saved
        for save_dir in save_dirs:
            if not path.exists(save_dir):
                makedirs(save_dir)
            SaveNexus(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".nxs"))

    def read_attenuation_coefficient_at_value(self, ws, val, unit):
        conv_ws = ConvertUnits(ADS.retrieve(ws), Target=unit)
        coefs = []
        xbins = conv_ws.readX(0)
        xdat = np.convolve(xbins, np.ones(2), "valid") / 2  # this gets the bin centres
        for r in range(conv_ws.getNumberHistograms()):
            ydat = conv_ws.readY(r)
            f = interpolate.interp1d(xdat, ydat)
            interp_val = f([val])[0]
            coefs.append(interp_val)
        ADS.remove("conv_ws")
        return coefs

    def get_atten_table_name(self, ws_str, eval_val, unit):
        ws = ADS.retrieve(ws_str)
        run_num = str(ws.getRun().getProperty("run_number").value)
        instr = ws.getInstrument().getName()
        return f"{instr}_{run_num}_attenuation_coefficient_{eval_val}_{unit}"

    def write_atten_val_table(self, ws, vals, eval_val, unit, rb_num, calibration, root_dir):
        out_ws = self.get_atten_table_name(ws, eval_val, unit)
        table = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
        table.addColumn("float", "I")
        for r in range(ADS.retrieve(ws).getNumberHistograms()):
            table.addRow(
                [
                    vals[r],
                ]
            )
        self._save_corrected_files(out_ws, root_dir, "AttenuationTables", rb_num, calibration.group)

    def create_reference_ws(self, rb_num, instr="ENGINX"):
        self.set_reference_ws(f"{rb_num}_reference_workspace")
        LoadEmptyInstrument(InstrumentName=instr, OutputWorkspace=self.reference_ws)

    def save_reference_file(self, rb_num, calibration, root_dir):
        if self.reference_ws and ADS.doesExist(self.reference_ws):
            self._save_corrected_files(self.reference_ws, root_dir, "ReferenceWorkspaces", rb_num, calibration.group)

    def set_reference_ws(self, ws_name):
        self.reference_ws = ws_name

    def get_reference_info(self):
        material = "Not set"
        shape_enabled = False
        if self.reference_ws:
            material = self.get_ws_info(self.reference_ws).get("material", "Not set")
            shape_enabled = not self._has_no_valid_shape(self.reference_ws)
        return self.reference_ws, shape_enabled, material

    def plot_sample_directions(self, fig, ws_name, ax_transform, ax_labels):
        ax = fig.axes[0]
        if not ws_name:
            ws_name = self.reference_ws
            # if we are looking at the reference, we want to rotate the sample independently of our definition axes
            rotation_matrix = np.eye(3)
            ws = ADS.retrieve(ws_name)
        else:
            ws = ADS.retrieve(ws_name)
            # if not looking at the reference, we want our definition axes to rotate with the sample
            rotation_matrix = ws.getRun().getGoniometer().getR()
        sample_mesh = ws.sample().getShape().getMesh()
        # apply the rotation matrix to the axes
        rotated_ax_transform = rotation_matrix @ ax_transform
        # transform the mesh vertices into new axes frame
        rot_vert = np.asarray([rotated_ax_transform.T @ sm[0, :] @ rotated_ax_transform for sm in sample_mesh]).T
        # find the furthest vertex projected along each axis
        arrow_lens = np.dot(rotated_ax_transform, rot_vert).max(axis=1) * 1.2
        rd = rotated_ax_transform[:, 0] * arrow_lens[0]
        nd = rotated_ax_transform[:, 1] * arrow_lens[1]
        td = rotated_ax_transform[:, 2] * arrow_lens[2]
        ax.quiver(0, 0, 0, *rd, color="red", length=arrow_lens[0], normalize=True, arrow_length_ratio=0.05)
        ax.quiver(0, 0, 0, *nd, color="green", length=arrow_lens[1], normalize=True, arrow_length_ratio=0.05)
        ax.quiver(0, 0, 0, *td, color="blue", length=arrow_lens[2], normalize=True, arrow_length_ratio=0.05)
        ax.text(*rd, ax_labels[0])
        ax.text(*nd, ax_labels[1])
        ax.text(*td, ax_labels[2])

    def plot_gauge_vol(self, preset, custom, fig):
        gauge_str = self.get_gauge_vol_str(preset, custom)
        if gauge_str:
            mesh = self.get_xml_mesh(gauge_str)
            if self._is_valid_mesh(mesh):
                axes = fig.gca()
                mesh_polygon = Poly3DCollection(mesh, facecolors="cyan", edgecolors="black", linewidths=0.1, alpha=0.25)
                axes.add_collection3d(mesh_polygon)


# temporary methods until SetGoniometer PR is merged
def NewSetGoniometer(
    ws: str,
    transformation_str: str = "",
    trans_scale: float = 1.0,
    axis0: str = "",
    axis1: str = "",
    axis2: str = "",
    axis3: str = "",
    axis4: str = "",
    axis5: str = "",
):
    if isinstance(ws, str):
        try:
            ws = ADS.retrieve(ws)
        except KeyError:
            logger.error(f"{ws} not found")
    if transformation_str:
        # info = ws.componentInfo()
        or_vals = transformation_str.split(",")
        run_mat = np.asarray(or_vals[:9], dtype=float).reshape((3, 3))
        # trans_vec = np.asarray(or_vals[9:], dtype=float) * trans_scale
        SetGoniometer(ws, Axis0=get_rot_vec_and_angle(run_mat))
        # info.setPosition(info.sample(), V3D(*trans_vec))
    else:
        SetGoniometer(ws, Axis0=axis0, Axis1=axis1, Axis2=axis2, Axis3=axis3, Axis4=axis4, Axis5=axis5)


def get_rot_vec_and_angle(mat: np.ndarray) -> str:
    from scipy.spatial.transform import Rotation

    if np.square(mat - np.eye(3, 3)).sum() != 0:
        rv = Rotation.from_matrix(mat).as_rotvec()
        ang = np.linalg.norm(rv)
        nv = rv / ang
        return f"{np.rad2deg(ang)},{nv[0]},{nv[1]},{nv[2]},1"
    else:
        return "0,0,0,1,1"
