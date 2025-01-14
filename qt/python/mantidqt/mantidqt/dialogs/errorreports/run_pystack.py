# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from datetime import datetime
from pathlib import Path
import base64
import re
import subprocess
import zlib

from mantid.kernel import ConfigService, Logger


CORE_DUMP_RECENCY_LIMIT = 30
log = Logger("errorreports (pystack analysis)")


def retrieve_thread_traces_from_coredump_file() -> bytes:
    # Locate the core dumps dir
    core_dumps_path = None
    try:
        core_dumps_path = _get_core_dumps_dir()
    except ValueError as e:
        log.error(str(e))
        return b""

    # Get most recent dump file, check it's python (can you check it's from workbench?)
    core_file = _get_most_recent_core_dump_file(core_dumps_path)
    if core_file is None:
        return b""

    # Run file through pystack and capture output
    pystack_output = _get_output_from_pystack(core_file)

    # Compress output and return
    compressed_bytes = zlib.compress(pystack_output.encode("utf-8"))
    return base64.standard_b64encode(compressed_bytes)


def _get_core_dumps_dir() -> Path:
    core_dumps_str = ConfigService.getString("errorreports.core_dumps")
    if not core_dumps_str:
        raise ValueError("errorreports.core_dumps not set")
    core_dumps_path = Path(core_dumps_str)
    if not core_dumps_path.exists():
        raise ValueError(f"errorreports.core_dumps value ({core_dumps_str}) does not exist")
    elif not core_dumps_path.is_dir():
        raise ValueError(f"errorreports.core_dumps value ({core_dumps_str}) is not a directory")
    return core_dumps_path


def _get_most_recent_core_dump_file(core_dumps_dir: Path) -> Path | None:
    files = core_dumps_dir.iterdir()
    files_sorted_by_latest = sorted([file for file in files], key=lambda file: file.stat().st_ctime, reverse=True)
    if files_sorted_by_latest:
        for latest_core_dump_file in files_sorted_by_latest:
            # test it's recent enough
            age = datetime.now() - datetime.fromtimestamp(latest_core_dump_file.stat().st_ctime)
            if age.seconds < CORE_DUMP_RECENCY_LIMIT:
                # test it's the correct process.
                if _check_core_file_is_the_workbench_process(latest_core_dump_file):
                    file_path = Path(core_dumps_dir, latest_core_dump_file)
                    log.notice(f"Found most recent workbench core dump: {file_path.as_posix()}")
                    return file_path
            else:
                log.notice(
                    f"Could not find recent enough ( < {CORE_DUMP_RECENCY_LIMIT} seconds old) core dump file in {core_dumps_dir.as_posix()}"
                )
                return None
    log.notice(f"No files found in {core_dumps_dir.as_posix()}")
    return None


def _check_core_file_is_the_workbench_process(core_dump_file: Path) -> bool:
    args = ["pystack", "core", core_dump_file.as_posix()]
    process = subprocess.run(args, capture_output=True, text=True)
    stdout = process.stdout
    search_result = re.search(r"^executable: python arguments: python.*mantid/qt/applications/workbench.*$", stdout, re.MULTILINE)
    return search_result is not None


def _get_output_from_pystack(core_dump_file: Path) -> str:
    args = ["pystack", "core", core_dump_file.as_posix(), "--native-all"]
    process = subprocess.run(args, capture_output=True, text=True)
    return process.stdout
