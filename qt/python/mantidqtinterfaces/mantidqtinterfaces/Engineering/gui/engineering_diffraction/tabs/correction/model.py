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
            else:
                try:
                    Load(Filename=path, OutputWorkspace=ws_name)
                except Exception as e:
                    logger.warning(f"Failed to load {path}: {e}")
                    continue
        return wss

    def load_ref(self, path):
        if path:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
                self.set_reference_ws(ws_name)
            else:
                try:
                    Load(Filename=path, OutputWorkspace=ws_name)
                    self.set_reference_ws(ws_name)
                except Exception as e:
                    logger.warning(f"Failed to load {path}: {e}")

    @staticmethod
    def get_out_ws_names(wss):
        return [f"Corrected_{ws}" for ws in wss]
