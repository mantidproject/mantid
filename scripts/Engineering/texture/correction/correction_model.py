# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    LoadSampleShape,
    SetSample,
    CopySample,
    SetSampleMaterial,
    DefineGaugeVolume,
    ConvertUnits,
    MonteCarloAbsorption,
    CloneWorkspace,
    SaveNexus,
    logger,
    CreateEmptyTableWorkspace,
    LoadEmptyInstrument,
    CreateDetectorTable,
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from os import path, makedirs
from scipy import interpolate
from Engineering.common.texture_sample_viewer import has_valid_shape
from typing import Optional, Sequence, Union, Tuple
from mantid.dataobjects import Workspace2D
from Engineering.common.calibration_info import CalibrationInfo
from Engineering.texture.texture_helper import get_gauge_vol_str, load_all_orientations


class TextureCorrectionModel:
    def __init__(self):
        self.reference_ws = None
        self.include_abs = False
        self.include_div = False
        self.include_atten = False
        self.rb_num = None
        self.calibration = None
        self.remove_ws_after_processing = False
        self.corrected_files = []

    # ~~~~~ Correction Functions ~~~~~~~~~~~~~

    def calc_absorption(self, ws: str, mc_param_str: str) -> None:
        method_dict = self._param_str_to_dict(mc_param_str) if mc_param_str else {}  # allow no kwargs to be given
        temp_ws = ConvertUnits(ws, Target="Wavelength")
        method_dict["OutputWorkspace"] = "_abs_corr"
        MonteCarloAbsorption(temp_ws, **method_dict)

    def calc_divergence(self, ws_name: str, horz: float, vert: float, det_horz: float) -> None:
        # estimate of divergence taken from divergence component of equation 3 in
        # J. Appl. Cryst. (2014). 47, 1337–1354 doi:10.1107/S1600576714012710

        ws = ADS.retrieve(ws_name)
        num_spec = ws.getNumberHistograms()
        thetas = self.get_thetas(ws)

        scale = vert * np.sqrt(horz**2 + det_horz**2)
        div = scale * (np.sin(thetas) ** 2)
        div = np.nan_to_num(div, nan=1.0)

        _div_corr = CloneWorkspace(InputWorkspace=ws, OutputWorkspace="_div_corr")
        y_shape = ws.readY(0).shape
        for i in range(num_spec):
            _div_corr.setY(i, np.full(y_shape, div[i]))

    @staticmethod
    def get_thetas(ws):
        spec_info = ws.spectrumInfo()
        num_spec = ws.getNumberHistograms()
        # no 2theta for monitors, we set them as nans and then replace these with 1 at the end of the calc
        # essentially resulting in no correction for monitors when ws/div_ws
        det_tab = CreateDetectorTable(ws)
        good_spectra = np.asarray(det_tab.column("Monitor")) == "no"
        return np.array([spec_info.twoTheta(i) if good_spectra[i] else np.nan for i in range(num_spec)])

    def calc_all_corrections(
        self,
        wss: Sequence[Workspace2D],
        out_wss: Sequence[str],
        root_dir: Optional[str] = None,
        abs_args: Optional[dict] = None,
        atten_args: Optional[dict] = None,
        div_args: Optional[dict] = None,
    ):
        corrected_file_paths = []
        for i, ws in enumerate(wss):
            abs_corr = 1.0
            div_corr = 1.0
            if self.include_div:
                logger.notice("Cannot correct monitors for beam divergence, monitor spectra will be ignored")
            if self.include_abs:
                self.define_gauge_volume(ws, abs_args["gauge_vol_preset"], abs_args["gauge_vol_file"])
                self.calc_absorption(ws, abs_args["mc_param_str"])
                abs_corr = "_abs_corr"
                if self.include_atten:
                    val, units = atten_args["atten_val"], atten_args["atten_units"]
                    atten_vals = self.read_attenuation_coefficient_at_value(abs_corr, val, units)
                    self.write_atten_val_table(
                        ws,
                        atten_vals,
                        val,
                        units,
                        self.rb_num,
                        self.calibration,
                        root_dir,
                    )

            if self.include_div:
                self.calc_divergence(ws, div_args["hoz"], div_args["vert"], div_args["det_hoz"])
                div_corr = "_div_corr"

            corrected_file_path = self.apply_corrections(ws, out_wss[i], root_dir, abs_corr, div_corr)
            corrected_file_paths.append(corrected_file_path)
        self.corrected_files = corrected_file_paths

    def apply_corrections(
        self,
        ws: str,
        out_ws: str,
        root_dir: str,
        abs_corr: Union[float, str] = 1.0,
        div_corr: Union[float, str] = 1.0,
    ) -> str:
        ws = ADS.retrieve(ws)
        temp_ws = ConvertUnits(ws, Target="dSpacing", StoreInADS=False)

        # if an absorption correction calculation has been run expect this to be "_abs_corr" (the output workspace)
        # otherwise it will just be 1.0
        if isinstance(abs_corr, str):
            abs_ws = ConvertUnits(ADS.retrieve(abs_corr), Target="dSpacing", StoreInADS=False)
            temp_ws = temp_ws / abs_ws
            ADS.remove("abs_ws")
        else:
            temp_ws = temp_ws / abs_corr

        # if a divergence correction calculation has been run expect this to be "_div_corr" (the output workspace)
        # otherwise it will just be 1.0
        if isinstance(div_corr, str):
            div_ws = ADS.retrieve(div_corr)
            # modify the intensity counts directly as the units of div_ws are arbitrary
            for i in range(temp_ws.getNumberHistograms()):
                temp_ws.setY(i, temp_ws.readY(i) / div_ws.readY(i))
            ADS.remove("div_ws")
        else:
            temp_ws = temp_ws / div_corr

        # save files
        CloneWorkspace(temp_ws, OutputWorkspace=out_ws, StoreInADS=True)
        is_texture = self.calibration.get_group() in self.calibration.config.texture_groups if self.calibration else False
        save_filepath = self._save_corrected_files(out_ws, root_dir, "AbsorptionCorrection", self.rb_num, is_texture)

        # optionally remove extra files
        if self.remove_ws_after_processing:
            logger.notice("removing saved and temporary workspaces from ADS")
            # remove output ws from ADS to free up memory
            ADS.remove(out_ws)
            if isinstance(abs_corr, str):
                ADS.remove(abs_corr)
            if isinstance(div_corr, str):
                ADS.remove(div_corr)

        return save_filepath

    # ~~~~~ General Utility Functions ~~~~~~~~~~~~~

    @staticmethod
    def _save_corrected_files(ws: str, root_dir: str, dir_name: str, rb_num: Optional[str], is_texture: bool) -> str:
        filepath = ""
        save_dirs = [path.join(root_dir, dir_name)]
        if rb_num:
            save_dirs.append(path.join(root_dir, "User", rb_num, dir_name))
            if is_texture:
                save_dirs.pop(0)  # only save to RB directory to limit number files saved
        for save_dir in save_dirs:
            if not path.exists(save_dir):
                makedirs(save_dir)
            filepath = path.join(save_dir, ws + ".nxs")
            SaveNexus(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".nxs"))
        return filepath

    def get_corrected_files(self):
        return self.corrected_files

    def set_include_abs(self, inc: bool):
        self.include_abs = inc

    def set_include_atten(self, inc: bool):
        self.include_atten = inc

    def set_include_div(self, inc: bool):
        self.include_div = inc

    def set_rb_num(self, rb: str):
        self.rb_num = rb

    def set_calibration(self, calib: CalibrationInfo):
        self.calibration = calib

    def set_remove_after_processing(self, flag: bool):
        self.remove_ws_after_processing = flag

    def get_alg_preset_values(self):
        if self.reference_ws:
            return {"InputWorkspace": self.reference_ws}
        current_workspaces = ADS.getObjectNames()
        if len(current_workspaces) > 0:
            return {"InputWorkspace": current_workspaces[0]}
        return {}

    # ~~~~~ Sample Definition and Orientation Functions ~~~~~~~~~~~~~

    def load_all_orientations(
        self,
        wss: Sequence[Workspace2D],
        txt_file: str,
        use_euler: bool,
        euler_scheme: Optional[str] = None,
        euler_sense: Optional[str] = None,
    ) -> None:
        load_all_orientations(wss, txt_file, use_euler, euler_scheme, euler_sense)

    @staticmethod
    def set_sample_info(ws: Workspace2D, shape: str, material: str) -> None:
        if shape.endswith(".stl"):
            LoadSampleShape(InputWorkspace=ws, OutputWorkspace=ws, Filename=shape)
        else:
            SetSample(ws, Geometry={"Shape": "CSG", "Value": shape})
        SetSampleMaterial(ws, material)

    @staticmethod
    def copy_sample_info(ref_ws_name: str, wss: Sequence[Workspace2D], is_ref: bool = False) -> None:
        # currently copy shape bakes the orientation matrix into the sample shape
        if not ADS.doesExist(ref_ws_name):
            logger.error(f"Workspace: {ref_ws_name} could not be found to copy the sample - has it been deleted?")
            return None  # saves wrapping the rest of the function in an else block
        if not is_ref:
            # need to create a ws with an unorientated sample to copy over
            ref_ws = ADS.retrieve(ref_ws_name)
            trans_mat = ref_ws.getRun().getGoniometer().getR()

            _tmp_ws = CloneWorkspace(ref_ws, OutputWorkspace="_tmp_ws")
            _tmp_ws.getRun().getGoniometer().setR(np.linalg.inv(trans_mat))

            CopySample(InputWorkspace=ref_ws, OutputWorkspace=_tmp_ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)

            for ws in wss:
                CopySample(InputWorkspace=_tmp_ws, OutputWorkspace=ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)
            ADS.remove("_tmp_ws")
        else:
            # if is_ref flag then we want to take this baked orientation forward
            for ws in wss:
                CopySample(InputWorkspace=ref_ws_name, OutputWorkspace=ws, CopyName=False, CopyEnvironment=False, CopyLattice=False)

    def get_ws_info(self, ws_name: str, select: bool = True) -> dict:
        return {
            "shape": "Not set" if self._has_no_valid_shape(ws_name) else "set",
            "material": "Not set" if self._has_no_valid_material(ws_name) else self._get_material_name(ws_name),
            "orient": "default" if self._has_no_orientation_set(ws_name) else "set",
            "select": select,
        }

    @staticmethod
    def _has_no_orientation_set(ws_name) -> bool:
        try:
            return np.all(ADS.retrieve(ws_name).run().getGoniometer().getR() == np.identity(3))
        except RuntimeError:
            return True

    @staticmethod
    def _get_material_name(ws_name: str) -> str:
        return ADS.retrieve(ws_name).sample().getMaterial().name()

    def _has_no_valid_material(self, ws_name: str) -> bool:
        return self._get_material_name(ws_name) == ""

    # ~~~~~ Gauge Volume Functions ~~~~~~~~~~~~~

    @staticmethod
    def define_gauge_volume(ws: Workspace2D, preset: str, custom: Optional[str]) -> None:
        gauge_str = get_gauge_vol_str(preset, custom)
        if gauge_str:
            DefineGaugeVolume(ws, gauge_str)

    # ~~~~~ `Parameter Dictionary as String` Functions ~~~~~~~~~~~~~

    def _param_str_to_dict(self, param_string: str) -> dict:
        out_dict = {}
        for params in param_string.split(","):
            k, v = params.split(":")
            out_dict[k] = self._parse_param_values(v)
        return out_dict

    @staticmethod
    def _parse_param_values(string) -> Union[bool, int, str]:
        l_string = string.lower()
        if l_string == "true":
            return True
        elif l_string == "false":
            return False
        elif string.isnumeric():
            return int(string)
        else:
            return string

    # ~~~~~ Attenuation Table Functions ~~~~~~~~~~~~~

    @staticmethod
    def read_attenuation_coefficient_at_value(ws: str, val: float, unit: str) -> Sequence[float]:
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

    @staticmethod
    def get_atten_table_name(ws_str: str, eval_val: float, unit: str) -> str:
        ws = ADS.retrieve(ws_str)
        run_num = str(ws.getRun().getProperty("run_number").value)
        instr = ws.getInstrument().getName()
        return f"{instr}_{run_num}_attenuation_coefficient_{eval_val}_{unit}"

    def write_atten_val_table(
        self,
        ws: str,
        vals: Sequence[float],
        eval_val: float,
        unit: str,
        rb_num: Optional[str],
        calibration: Optional[CalibrationInfo],
        root_dir: str,
    ) -> None:
        out_ws = self.get_atten_table_name(ws, eval_val, unit)
        table = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
        table.addColumn("float", "mu")
        for r in range(ADS.retrieve(ws).getNumberHistograms()):
            table.addRow(
                [
                    vals[r],
                ]
            )
        is_texture = self.calibration.get_group() in self.calibration.config.texture_groups if self.calibration else False
        self._save_corrected_files(out_ws, root_dir, "AttenuationTables", rb_num, is_texture)

    # ~~~~~ Reference Workspace Functions ~~~~~~~~~~~~~

    def create_reference_ws(self, rb_num: str, instr: str = "ENGINX") -> None:
        self.set_reference_ws(f"{rb_num}_reference_workspace")
        LoadEmptyInstrument(InstrumentName=instr, OutputWorkspace=self.reference_ws)

    def save_reference_file(self, rb_num: str, current_calib: Optional[CalibrationInfo], root_dir: Optional[str]) -> None:
        is_texture = current_calib.get_group() in current_calib.config.texture_groups if current_calib else False
        if self.reference_ws and ADS.doesExist(self.reference_ws):
            self._save_corrected_files(self.reference_ws, root_dir, "ReferenceWorkspaces", rb_num, is_texture)

    def set_reference_ws(self, ws_name: str) -> None:
        self.reference_ws = ws_name

    def get_reference_ws(self):
        return self.reference_ws

    def get_reference_info(self) -> Tuple[str, bool, str]:
        material = "Not set"
        shape_enabled = False
        if self.reference_ws:
            material = self.get_ws_info(self.reference_ws).get("material", "Not set")
            shape_enabled = not self._has_no_valid_shape(self.reference_ws)
        return self.reference_ws, shape_enabled, material

    # ~~~~~ Plotting Functions ~~~~~~~~~~~~~

    @staticmethod
    def _has_no_valid_shape(ws_name: str) -> bool:
        return not has_valid_shape(ws_name)
