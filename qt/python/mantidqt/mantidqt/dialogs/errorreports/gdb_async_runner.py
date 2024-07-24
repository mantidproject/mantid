# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import resource
import subprocess
import tempfile
from datetime import datetime
from pathlib import Path

from mantid.kernel import ConfigService, Logger
from mantidqt.utils.async_qt_adaptor import IQtAsync, qt_async_task


class GDBAsync(IQtAsync):
    def __init__(self, parent_presenter: "ErrorReporterPresenter"):
        super().__init__()
        self._parent_presenter = parent_presenter
        self.log = Logger("errorreports (gdb recovery)")

    def finished_cb_slot(self) -> None:
        self._parent_presenter.async_completion_call_back()

    @qt_async_task
    def run_gdb_recovery(self):
        self._parent_presenter._traceback = self._recover_trace_from_core_dump()

    def _recover_trace_from_core_dump(self):
        if not self._core_dumps_enabled():
            self.log.notice("Core dumps not enabled (enable with 'ulimit -c unlimited'); exiting")
            return ""
        core_dump_dir = ConfigService.getString("errorreports.core_dumps")
        if not core_dump_dir:
            with open("/proc/sys/kernel/core_pattern") as fp:
                core_dump_dir = fp.readline().split()[0][1:]
        if not os.path.exists(core_dump_dir) or not os.path.isdir(core_dump_dir):
            return ""
        self.log.notice(f"Found core dump directory {core_dump_dir}")
        latest_core_dump = self._latest_core_dump(core_dump_dir)
        if latest_core_dump is None:
            return ""
        self.log.notice(f"Found latest core dump file {latest_core_dump} ({datetime.fromtimestamp(latest_core_dump.stat().st_ctime)})")
        output = ""
        if latest_core_dump.suffix == ".lz4":
            output = self._decompress_lz4_then_run_gdb(latest_core_dump)
        else:
            output = self._run_gdb(latest_core_dump.as_posix())
        if not output:
            return ""
        self.log.notice("Trimming gdb output to extract back trace...")
        return self._trim_core_dump_file(output)

    def _core_dumps_enabled(self):
        size = resource.getrlimit(resource.RLIMIT_CORE)[1]
        return size != 0

    def _decompress_lz4_then_run_gdb(self, latest_core_dump: Path):
        tmp_core_copy_fp = tempfile.NamedTemporaryFile()
        lz4_command = ["lz4", "-d", latest_core_dump.as_posix(), tmp_core_copy_fp.name]
        self.log.notice(f"Running {' '.join(lz4_command)} ...")
        result = subprocess.run(lz4_command)
        output = ""
        if result.returncode == 0:
            self.log.notice(f"Decompressed core file to {tmp_core_copy_fp.name}")
            output = self._run_gdb(tmp_core_copy_fp.name)
        else:
            self.log.notice(f"lz4 returned non-zero exit code:\n{result.stderr}")
        tmp_core_copy_fp.close()
        return output

    def _run_gdb(self, core_file: str):
        commands_fp = tempfile.NamedTemporaryFile()
        with open(commands_fp.name, "w") as fp:
            fp.write("bt\nq\n")

        gdb_command = ["gdb", "-x", commands_fp.name, f"{os.environ['CONDA_PREFIX']}/bin/python", core_file]
        self.log.notice(f"Running {' '.join(gdb_command)} ...")
        result = subprocess.run(gdb_command, capture_output=True, text=True)
        commands_fp.close()
        if result.returncode == 0:
            self.log.notice("gdb ran successfully")
            return result.stdout
        self.log.notice(f"gdb returned non-zero exit code:\n{result.stderr}")
        return ""

    def _latest_core_dump(self, dir: str):
        CORE_DUMP_RECENCY_LIMIT = 30
        files = Path(dir).iterdir()
        sorted_files = sorted([file for file in files], key=lambda file: file.stat().st_ctime)
        if sorted_files:
            latest_core_dump = sorted_files[-1]
            difference = datetime.now() - datetime.fromtimestamp(latest_core_dump.stat().st_ctime)
            if difference.seconds < CORE_DUMP_RECENCY_LIMIT:
                return Path(dir, sorted_files[-1])
        self.log.notice(f"Did not find a core dump from within the last {CORE_DUMP_RECENCY_LIMIT} seconds.")
        return None

    @staticmethod
    def _trim_core_dump_file(content: str):
        lines = content.split("\n")
        trace_begins_index = next((n for n, line in enumerate(lines) if line.startswith("#")), None)
        if trace_begins_index is not None:
            return "\n".join(lines[trace_begins_index - 1 :])
        return ""
