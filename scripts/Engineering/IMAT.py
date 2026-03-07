# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence, Optional

from enum import Enum
from Engineering.BaseEngInstrument import BaseEngInstrument


class IMAT(BaseEngInstrument):
    def __init__(
        self,
        vanadium_run: str,
        focus_runs: Sequence[str],
        save_dir: str,
        full_inst_calib_path: str,
        prm_path: Optional[str] = None,
        ceria_run: Optional[str] = None,
        group: Optional[Enum] = None,
        groupingfile_path: Optional[str] = None,
        spectrum_num: Optional[str] = None,
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
            instrument="IMAT",
        )
