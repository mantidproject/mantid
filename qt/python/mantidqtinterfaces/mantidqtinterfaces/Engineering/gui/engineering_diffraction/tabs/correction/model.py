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
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from os import path, makedirs


class TextureCorrectionModel:
    def load_all_orientations(self, wss, txt_file):
        print(txt_file, self._validate_file(txt_file, ".txt"))
        if self._validate_file(txt_file, ".txt"):
            with open(txt_file, "r") as f:
                goniometer_strings = [line.replace("\t", ",") for line in f.readlines()]
            for iws, ws in enumerate(wss):
                NewSetGoniometer(ws, transformation_str=goniometer_strings[iws])

    def set_sample_info(self, ws, shape, material):
        if shape.endswith(".stl"):
            LoadSampleShape(InputWorkspace=ws, OutputWorkspace=ws, Filename=shape)
        else:
            SetSample(ws, Geometry={"Shape": "CSG", "Value": shape})
        SetSampleMaterial(ws, material)

    def copy_sample_info(self, ref_ws, wss):
        # currently copy shape bakes the orientation matrix into the sample shape
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

    def _has_no_valid_shape(self, ws_name):
        no_shape = False
        try:
            no_shape = len(ADS.retrieve(ws_name).sample().getShape().getMesh()) < 3
        except RuntimeError:
            no_shape = True
        return no_shape

    def _get_material_name(self, ws_name):
        return ADS.retrieve(ws_name).sample().getMaterial().name()

    def _has_no_valid_material(self, ws_name):
        return self._get_material_name(ws_name) == ""

    def define_gauge_volume(self, ws, preset, custom):
        if preset == "4mmCube":
            gauge_str = """
<cuboid id='some-gv'> \
<height val='4.0'  /> \
<width val='4.0' />  \
<depth  val='4.0' />  \
<centre x='0.0' y='0.0' z='0.0'  />  \
</cuboid>  \
<algebra val='some-gv' /> \\ """
        else:
            try:
                gauge_str = self._read_xml(custom)
            except RuntimeError:
                gauge_str = None
        if gauge_str:
            DefineGaugeVolume(ws, gauge_str)

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

    def calc_absorption(self, ws):
        mc_param_str = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "monte_carlo_params")
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

    def apply_corrections(self, ws, out_ws, abs_corr=1.0, div_corr=1.0, save_dir=None):
        ws = ADS.retrieve(ws)
        temp_ws = ConvertUnits(ws, Target="dSpacing")
        if isinstance(abs_corr, str):
            abs_ws = ConvertUnits(ADS.retrieve(abs_corr), Target="dSpacing")
            temp_ws = temp_ws / abs_ws
        else:
            temp_ws = temp_ws / abs_corr
        if isinstance(div_corr, str):
            div_ws = ConvertUnits(ADS.retrieve(div_corr), Target="dSpacing")
            temp_ws = temp_ws / div_ws
        else:
            temp_ws = temp_ws / div_corr
        CloneWorkspace(temp_ws, OutputWorkspace=out_ws)
        self._save_corrected_files(out_ws)

    def _save_corrected_files(self, ws):
        save_dir = output_settings.get_output_path()
        corr_dirs = [path.join(save_dir, "AbsorptionCorrected")]
        for corr_dir in corr_dirs:
            if not path.exists(corr_dir):
                makedirs(corr_dir)
            SaveNexus(InputWorkspace=ws, Filename=path.join(corr_dir, ws + ".nxs"))


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
