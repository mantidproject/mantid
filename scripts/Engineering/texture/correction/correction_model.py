from mantid.simpleapi import (
    LoadSampleShape,
    SetSample,
    CopySample,
    SetSampleMaterial,
    DefineGaugeVolume,
    ConvertUnits,
    MonteCarloAbsorption,
    EstimateDivergence,
    CloneWorkspace,
    SaveNexus,
    SetGoniometer,
    logger,
    CreateEmptyTableWorkspace,
    LoadEmptyInstrument,
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from os import path, makedirs
from scipy import interpolate
from Engineering.EnggUtils import GROUP
from Engineering.common.texture_sample_viewer import has_no_valid_shape, plot_sample_directions, plot_gauge_vol


class TextureCorrectionModel:
    def __init__(self):
        self.reference_ws = None

    def load_all_orientations(self, wss, txt_file, use_euler, euler_scheme=None, euler_sense=None):
        if self._validate_file(txt_file, ".txt"):
            with open(txt_file, "r") as f:
                goniometer_strings = [line.strip().replace("\t", ",") for line in f.readlines()]
                goniometer_lists = [[float(x) for x in gs.split(",")] for gs in goniometer_strings]
            try:
                if not use_euler:
                    # if use euler angles not selected then assumes it is a scans output matrix
                    for iws, ws in enumerate(wss):
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
        <height val='0.004'  /> \
        <width val='0.004' />  \
        <depth  val='0.004' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='some-gv' /> \\ """
        elif preset == "No Gauge Volume":
            gauge_str = None
        else:
            try:
                gauge_str = self._read_xml(custom)
            except RuntimeError:
                gauge_str = None
        return gauge_str

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
        # Scale(InputWorkspace=temp_ws, OutputWorkspace=temp_ws, Factor=1, Operation="Add")
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
        table.addColumn("float", "mu")
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

    def plot_gauge_vol(self, preset, custom_shape, fig):
        gauge_vol_str = self.get_gauge_vol_str(preset, custom_shape)
        plot_gauge_vol(gauge_vol_str, fig)

    def plot_sample_directions(self, fig, ws_name, ax_transform, ax_labels):
        plot_sample_directions(fig, ws_name, ax_transform, ax_labels, self.reference_ws)

    def _has_no_valid_shape(self, ws_name):
        has_no_valid_shape(ws_name)
