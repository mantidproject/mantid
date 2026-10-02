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
        log.warning(str(e))
        return b""

    pystack_output = _get_pystack_output_for_most_recent_core_dump(core_dumps_path, workbench_pid)
    if not pystack_output:
        return b""

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


def _get_pystack_output_for_most_recent_core_dump(core_dumps_dir: Path, workbench_pid: str) -> str:
    files_sorted_by_latest = sorted(core_dumps_dir.iterdir(), key=lambda file: file.stat().st_ctime, reverse=True)
    for core_dump_file in files_sorted_by_latest:
        # test it's recent enough
        age = datetime.now() - datetime.fromtimestamp(core_dump_file.stat().st_ctime)
        if age.total_seconds() >= CORE_DUMP_RECENCY_LIMIT:
            break
        log.notice(f"Found recent file {core_dump_file.as_posix()}")
        pystack_output = _get_pystack_output_if_workbench_process(core_dump_file, workbench_pid)
        if pystack_output:
            return pystack_output
    log.notice(
        f"Could not find recent enough ( < {CORE_DUMP_RECENCY_LIMIT} seconds old) "
        f"mantid workbench core dump file in {core_dumps_dir.as_posix()}"
    )
    return ""


def _get_pystack_output_if_workbench_process(core_dump_file: Path, workbench_pid: str) -> str:
    decompressed_core_dump_file = None
    if _is_lz4_file(core_dump_file):
        decompressed_core_dump_file = _decompress_lz4_file(core_dump_file)
        log.notice(f"Decompressed lz4 core file to {decompressed_core_dump_file.as_posix()}")
    try:
        pystack_output = _run_pystack(decompressed_core_dump_file or core_dump_file)
    finally:
        # The decompressed core file can be hundreds of MB, so don't leave it behind
        if decompressed_core_dump_file is not None:
            decompressed_core_dump_file.unlink(missing_ok=True)

    # test it's the correct process.
    if _is_workbench_process(pystack_output, workbench_pid):
        log.notice(f"{core_dump_file.as_posix()} identified as a mantid workbench core dump")
        return pystack_output
    log.notice(f"{core_dump_file.as_posix()} not identified as a mantid workbench core dump")
    return ""


def _run_pystack(core_dump_file: Path) -> str:
    # pystack can fail to locate the Python interpreter state, e.g. when a second copy of libpython is loaded
    # alongside the statically linked conda python. It still reports the core file information and, with
    # --native-all, the native stack of every thread, so warnings on stderr are not treated as a failure.
    args = ["pystack", "core", core_dump_file.as_posix(), "--native-all"]
    process = subprocess.run(args, capture_output=True, text=True)
    if process.stderr:
        log.warning(f"Pystack reported problems when analysing {core_dump_file.as_posix()}: {process.stderr}")
    return process.stdout


def _is_workbench_process(pystack_output: str, workbench_pid: str) -> bool:
    search_result = re.search(r"pid: (\d+) ppid: (\d+) ", pystack_output)
    if search_result is None:
        return False
    # The parent pid is also accepted in case workbench was launched through an intermediate shell
    return workbench_pid in (search_result.group(1), search_result.group(2))


def _decompress_lz4_file(lz4_core_dump_file: Path) -> Path:
    with NamedTemporaryFile(delete=False) as tmp_decompressed_core_file:
        with lz4.frame.open(lz4_core_dump_file.as_posix(), "r") as lz4_fp:
            tmp_decompressed_core_file.write(lz4_fp.read())
    return Path(tmp_decompressed_core_file.name)


def _is_lz4_file(core_dump_file: Path) -> bool:
    lz4_magic_number = b"\x04\x22\x4d\x18"
    with open(core_dump_file, "rb") as f:
        return f.read(4) == lz4_magic_number
