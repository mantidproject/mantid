# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from datetime import datetime
from pathlib import Path
from tempfile import NamedTemporaryFile

from mantid.kernel.environment import is_linux
from mantid.kernel import ConfigService, Logger

import base64

if is_linux():
    import lz4.frame
import re
import subprocess
import zlib


CORE_DUMP_RECENCY_LIMIT = 30
log = Logger("errorreports (pystack analysis)")


def retrieve_thread_traces_from_coredump_file(workbench_pid: str) -> bytes:
    # Locate the core dumps dir
    core_dumps_path = None
    try:
        core_dumps_path = _get_core_dumps_dir()
    except ValueError as e:
        log.error(str(e))
        return b""

    # Get most recent dump file, check it's python (can you check it's from workbench?)
    core_file = _get_most_recent_core_dump_file(core_dumps_path, workbench_pid)
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


def _get_most_recent_core_dump_file(core_dumps_dir: Path, workbench_pid: str) -> Path | None:
    files = core_dumps_dir.iterdir()
    files_sorted_by_latest = sorted([file for file in files], key=lambda file: file.stat().st_ctime, reverse=True)
    if files_sorted_by_latest:
        for latest_core_dump_file in files_sorted_by_latest:
            # test it's recent enough
            age = datetime.now() - datetime.fromtimestamp(latest_core_dump_file.stat().st_ctime)
            if age.seconds < CORE_DUMP_RECENCY_LIMIT:
                log.notice(f"Found recent file {latest_core_dump_file.as_posix()}")
                if _is_lz4_file(latest_core_dump_file):
                    latest_core_dump_file = _decompress_lz4_file(latest_core_dump_file)
                    log.notice(f"Decompressed lz4 core file to {latest_core_dump_file.as_posix()}")
                # test it's the correct process.
                if _check_core_file_is_the_workbench_process(latest_core_dump_file, workbench_pid):
                    log.notice(f"{latest_core_dump_file.as_posix()} identified as a mantid workbench core dump")
                    return latest_core_dump_file
                else:
                    log.notice(f"{latest_core_dump_file.as_posix()} not itdentified as a mantid workbench core dump")
            else:
                log.notice(
                    f"Could not find recent enough ( < {CORE_DUMP_RECENCY_LIMIT} "
                    "seconds old) valid core dump file in {core_dumps_dir.as_posix()}"
                )
                return None
    log.notice(f"No valid files found in {core_dumps_dir.as_posix()}")
    return None


def _check_core_file_is_the_workbench_process(core_dump_file: Path, workbench_pid: str) -> bool:
    args = ["pystack", "core", core_dump_file.as_posix()]
    process = subprocess.run(args, capture_output=True, text=True)
    if process.stderr:
        log.error(f"Pystack executable check failed: {process.stderr}")
        return False
    stdout = process.stdout
    search_result = re.search(r"pid: (\d+) ppid: (\d+) ", stdout)
    if search_result is not None:
        # Since the process id comes from Popen with shell=True, it might be the pid of the parent shell
        # Seems to be inconsistent between distributions.
        return workbench_pid in (search_result.group(1), search_result.group(2))
    return False


def _get_output_from_pystack(core_dump_file: Path) -> str:
    args = ["pystack", "core", core_dump_file.as_posix(), "--native-all"]
    process = subprocess.run(args, capture_output=True, text=True)
    if process.stderr:
        log.error(f"Error when running Pystack: {process.stderr}")
    return process.stdout


def _decompress_lz4_file(lz4_core_dump_file: Path) -> Path:
    with NamedTemporaryFile(delete=False) as tmp_decompressed_core_file:
        with lz4.frame.open(lz4_core_dump_file.as_posix(), "r") as lz4_fp:
            tmp_decompressed_core_file.write(lz4_fp.read())
    return Path(tmp_decompressed_core_file.name)


def _is_lz4_file(core_dump_file: Path) -> bool:
    lz4_magic_number = b"\x04\x22\x4d\x18"
    with open(core_dump_file, "rb") as f:
        return f.read(4) == lz4_magic_number
