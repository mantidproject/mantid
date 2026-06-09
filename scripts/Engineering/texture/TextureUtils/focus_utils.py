# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import logger, Load
from Engineering.EnginX import EnginX
from Engineering.IMAT import IMAT
from mantid.api import AnalysisDataService as ADS
from typing import Sequence
from Engineering.common.instrument_config import get_instr_config

from Engineering.texture.TextureUtils.io import mk


def run_focus_script(
    wss: Sequence[str],
    focus_dir: str,
    van_run: str,
    ceria_run: str,
    full_instr_calib: str,
    grouping: str | None = None,
    prm_path: str | None = None,
    spectrum_num: str | None = None,
    groupingfile_path: str | None = None,
) -> None:
    """
    Focus data for use in a texture analysis pipeline. Currently only ENGIN-X is supported,
    but TextureInstrument class should grow to include others.

    wss: Sequence of workspaces to be focused, can be paths to files or ws names
    focus_dir: directory of where the focused data should be saved
    van_run: the run number/ file path of the vanadium calibration run
    ceria_run: the run number/ file path of the latest ceria calibration run at time of experiment
    full_instr_calib: path to the full instrument calibration file (can be found in settings of Engineering Diffraction Interface)
    grouping: key for desired detector grouping, if standard, otherwise use the prm path
    prm_path: optional path to the grouping prm file (produced during calibration), if using a standard detector grouping,
              just use the grouping argument
    spectrum_num: optional string of spectra numbers if desired to define custom grouping by specifying the spectra
    groupingfile_path: optional path to a grouping ".cal" or ".xml" file, alternative to prm_path
    """
    instrument = _get_instrument_from_ws_list(wss)
    config = get_instr_config(instrument)
    group = config.group(grouping) if grouping else None
    match instrument:
        case "IMAT":
            TextureInstrument = IMAT
        case "ENGINX" | "ENGIN-X":
            TextureInstrument = EnginX
        case _:
            raise ValueError(f"Unsupported instrument '{instrument}' for texture focusing. Supported instruments are ENGINX and IMAT.")
    model = TextureInstrument(
        vanadium_run=van_run,
        ceria_run=ceria_run,
        focus_runs=wss,
        save_dir=focus_dir,
        prm_path=prm_path,
        full_inst_calib_path=full_instr_calib,
        group=group,
        spectrum_num=spectrum_num,
        groupingfile_path=groupingfile_path,
    )

    mk(focus_dir)
    model.main()


def _get_instrument_from_ws_list(wss: Sequence[str]) -> str | None:
    instruments = set()
    for ws_str in wss:
        if ADS.doesExist(ws_str):
            ws = ADS.retrieve(ws_str)
        else:
            try:
                ws = Load(Filename=ws_str)
            except:
                logger.error(f"Could not find or load '{ws_str}'")
                return None
        instruments.add(ws.getInstrument().getName())
    instruments = list(instruments)
    if len(instruments) == 1:
        return instruments[0]
    else:
        logger.error("Workspaces provided have multiple different instruments attached: " + ", ".join(instruments))
        return None
