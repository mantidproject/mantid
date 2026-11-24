# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Check that MPI can start on this machine before the MPI unit tests run.

On a misconfigured host (e.g. a container whose hostname does not resolve) MPI start-up hangs rather than failing,
so each check runs in a subprocess with a short timeout and the reason is printed for whoever looks after the machine.

Usage: check_mpi_environment.py MPIEXEC NUMPROC_FLAG
"""

import socket
import subprocess
import sys
from pathlib import Path

TIMEOUT = 5  # seconds; MPI starts in well under a second on a healthy machine


def run_check(description: str, cmd: list[str]) -> bool:
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        print(f"FAIL: {description}: timed out after {TIMEOUT}s")
        return False
    if result.returncode != 0:
        print(f"FAIL: {description}: exit code {result.returncode}")
        print(result.stdout + result.stderr)
        return False
    print(f"OK:   {description}")
    return True


def print_diagnostics() -> None:
    print(f"\nhostname: {socket.gethostname()}")
    print("network interfaces: " + ", ".join(name for _, name in socket.if_nameindex()))
    hosts = Path("/etc/hosts")
    if hosts.exists():
        print(f"{hosts}:\n{hosts.read_text()}")


def main(mpiexec: str, numproc_flag: str) -> int:
    hostname = socket.gethostname()
    # Resolving in a subprocess as a lookup that goes to an unreachable DNS server can hang
    checks = [
        (
            f"resolve hostname '{hostname}'",
            [sys.executable, "-c", f"import socket; print(socket.getaddrinfo({hostname!r}, None)[0][4][0])"],
        ),
        ("start MPI in a process", [sys.executable, "-c", "from mpi4py import MPI"]),
        ("start MPI with mpiexec", [mpiexec, numproc_flag, "1", sys.executable, "-c", "from mpi4py import MPI"]),
    ]
    # Run every check so the log shows everything that is wrong
    passed = [run_check(description, cmd) for description, cmd in checks]
    if all(passed):
        return 0
    print_diagnostics()
    print("MPI cannot start on this machine, so the MPI unit tests will not be run")
    return 1


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1], sys.argv[2]))
