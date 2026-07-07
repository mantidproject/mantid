# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence

from Engineering.common.instrument_config import INSTRUMENT_GROUP
from Engineering.BaseEngInstrument import BaseEngInstrument


class EnginX(BaseEngInstrument):
    def __init__(
        self,
        vanadium_run: str,
        focus_runs: Sequence[str],
        save_dir: str,
        full_inst_calib_path: str,
        prm_path: str | None = None,
        ceria_run: str | None = None,
        group: INSTRUMENT_GROUP | None = None,
        groupingfile_path: str | None = None,
        spectrum_num: str | None = None,
    ) -> None:
        super().__init__(
            vanadium_run=vanadium_run,
            focus_runs=focus_runs,
            save_dir=save_dir,
            full_inst_calib_path=full_inst_calib_path,
            prm_path=prm_path,
            ceria_run=ceria_run,
            group=group,
            groupingfile_path=groupingfile_path,
            spectrum_num=spectrum_num,
            instrument="ENGINX",
        )
