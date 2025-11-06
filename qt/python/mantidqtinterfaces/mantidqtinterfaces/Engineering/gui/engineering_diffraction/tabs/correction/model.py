# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import os
from Engineering.texture.correction.correction_model import TextureCorrectionModel
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import Load


class CorrectionModel(TextureCorrectionModel):
    @staticmethod
    def load_files(filenames):
        wss = []
        for path in filenames:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            wss.append(ws_name)
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
                continue
            try:
                Load(Filename=path, OutputWorkspace=ws_name)
            except Exception as e:
                logger.warning(f"Failed to load {path}: {e}")
        return wss

    def load_ref(self, path):
        if not path:
            return
        ws_name = os.path.splitext(os.path.basename(path))[0]
        if ADS.doesExist(ws_name):
            logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
            self.set_reference_ws(ws_name)
            return
        try:
            Load(Filename=path, OutputWorkspace=ws_name)
            self.set_reference_ws(ws_name)
        except Exception as e:
            logger.warning(f"Failed to load {path}: {e}")

    @staticmethod
    def get_out_ws_names(wss):
        return [f"Corrected_{ws}" for ws in wss]

    def get_atten_args(self, inc_atten, eval_point, eval_unit):
        if not inc_atten:
            return {}, False
        msg, eval_point = self.try_convert_float(eval_point, "Attenuation Evaluation Point")
        if msg != "":
            logger.error(msg)
            return {}, True
        return {"atten_val": eval_point, "atten_units": eval_unit}, False

    def get_div_args(self, inc_div, div_hoz, div_vert, det_hoz):
        if not inc_div:
            return {}, False
        div_hoz_msg, div_hoz = self.try_convert_float(div_hoz, "Horizontal Divergence")
        div_vert_msg, div_vert = self.try_convert_float(div_vert, "Vertical Divergence")
        det_hoz_msg, det_hoz = self.try_convert_float(det_hoz, "Detector Divergence")
        msg = div_hoz_msg + div_vert_msg + det_hoz_msg
        if msg != "":
            logger.error(msg)
            return {}, True
        return {"hoz": div_hoz, "vert": div_vert, "det_hoz": det_hoz}, False

    @staticmethod
    def try_convert_float(val, param):
        try:
            return "", float(val)
        except ValueError:
            return f"{param} must be interpretable as a float. \n", 0.0
