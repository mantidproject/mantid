# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
import subprocess
import sys
import os
import pathlib
from mantid.kernel import ConfigService, Logger

log = Logger("MPIEXEC")


class ExecuteMPITest(systemtesting.MantidSystemTest):
    def skipTests(self):
        return os.environ.get("MPI_ENABLED") != "TRUE"

    def requiredFiles(self):
        return ["PG3_9829_event.nxs"]

    def runTest(self):
        config = ConfigService.Instance()
        dsd = config.getDataSearchDirs()

        current_dir = pathlib.Path(__file__).resolve().parent
        test_file = current_dir / "MPIAlgorithmsTest.py"

        cmd = ["mpiexec", "-n", "4", sys.executable, test_file, ",".join(dsd)]
        result = subprocess.run(cmd, capture_output=True, text=True)

        log.information(f"TEST OUTPUT:\n{result.stdout}")
        assert result.returncode == 0, f"{result.returncode} MPI tests failed"
